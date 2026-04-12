#include <math.h>
#include <stdio.h>
#include <string.h>

#include "pd_api.h"

// Screen
#define WIDTH 400
#define HEIGHT 240
#define MARGIN 4

// Benchmark config
#define BATCH_SIZE 10
#define BTN_COOLDOWN 8
#define SPAWNER_SPEED 8.0f
#define CRANK_SCALE (400.0f / 360.0f)
#define SPAWNER_Y 16

// Physics (per-frame, matches Lua)
#define GRAVITY 0.5f
#define DAMPING 0.85f
#define MIN_SPEED 0.5f

// FPS stepping
#define FPS_MIN 10
#define FPS_MAX 50
#define FPS_STEP 10

// Pre-allocated bunny capacity; doubles on overflow
#define INITIAL_CAPACITY 4096

typedef struct {
  float x, y, vx, vy;
} Bunny;

typedef struct {
  Bunny *bunnies;
  int count;
  int capacity;
  float spawnerX;
  int cooldown;
  int fpsTarget;
  LCDBitmap *bitmap;
  int bitmapW;
  int bitmapH;
  LCDFont *font;
  uint32_t rng;
} GameState;

static PlaydateAPI *pd = NULL;
static GameState *g_state = NULL;

static int update(void *userdata);

// Fast LCG — same constants as the Rust reference
static inline float rng_next(GameState *s) {
  s->rng = s->rng * 1664525u + 1013904223u;
  return (float)(s->rng >> 8) *
         (1.0f / 16777216.0f); // >> 8 gives 24 bits; / 2^24
}

static void spawn_batch(GameState *s) {
  int new_count = s->count + BATCH_SIZE;
  if (new_count > s->capacity) {
    while (s->capacity < new_count)
      s->capacity *= 2;
    s->bunnies =
        pd->system->realloc(s->bunnies, sizeof(Bunny) * (size_t)s->capacity);
  }
  float sx = s->spawnerX;
  float sy = (float)SPAWNER_Y;
  for (int i = 0; i < BATCH_SIZE; i++) {
    float vx = (rng_next(s) - 0.5f) * 10.0f;
    float vy = (rng_next(s) - 0.8f) * 10.0f;
    s->bunnies[s->count++] = (Bunny){sx, sy, vx, vy};
  }
}

static void despawn_batch(GameState *s) {
  s->count -= BATCH_SIZE;
  if (s->count < 0)
    s->count = 0;
}

static int update(void *userdata) {
  GameState *s = (GameState *)userdata;

  // --- Input ---
  PDButtons cur, pushed, released;
  pd->system->getButtonState(&cur, &pushed, &released);

  // Left/Right + crank: slide spawner along horizontal track
  float dx = pd->system->getCrankChange() * CRANK_SCALE;
  if (cur & kButtonLeft)
    dx -= SPAWNER_SPEED;
  else if (cur & kButtonRight)
    dx += SPAWNER_SPEED;
  if (dx != 0.0f) {
    s->spawnerX += dx;
    if (s->spawnerX < 0.0f)
      s->spawnerX = 0.0f;
    if (s->spawnerX > (float)WIDTH)
      s->spawnerX = (float)WIDTH;
  }

  // Up/Down: step FPS target (one step per press, 10-50)
  if (pushed & kButtonUp) {
    if (s->fpsTarget < FPS_MAX) {
      s->fpsTarget += FPS_STEP;
      pd->display->setRefreshRate(s->fpsTarget);
    }
  } else if (pushed & kButtonDown) {
    if (s->fpsTarget > FPS_MIN) {
      s->fpsTarget -= FPS_STEP;
      pd->display->setRefreshRate(s->fpsTarget);
    }
  }

  // A/B: spawn or despawn bunnies, repeats while held
  if (s->cooldown > 0) {
    s->cooldown--;
  } else if (cur & kButtonA) {
    spawn_batch(s);
    s->cooldown = BTN_COOLDOWN;
  } else if (cur & kButtonB) {
    despawn_batch(s);
    s->cooldown = BTN_COOLDOWN;
  }

  // --- Clear ---
  pd->graphics->clear(kColorWhite);

  // --- Physics + Draw (single pass) ---
  float bound_r = (float)(WIDTH - s->bitmapW);
  float bound_b = (float)(HEIGHT - s->bitmapH);
  LCDBitmap *bmp = s->bitmap;
  Bunny *bunnies = s->bunnies;
  int n = s->count;

  for (int i = 0; i < n; i++) {
    Bunny *b = &bunnies[i];
    float vx = b->vx;
    float vy = b->vy + GRAVITY;
    float x = b->x + vx;
    float y = b->y + vy;

    if (x < 0.0f) {
      x = 0.0f;
      vx = fabsf(vx) * DAMPING;
      if (vx < MIN_SPEED)
        vx = MIN_SPEED;
    } else if (x > bound_r) {
      x = bound_r;
      vx = -(fabsf(vx) * DAMPING);
      if (vx > -MIN_SPEED)
        vx = -MIN_SPEED;
    }

    if (y < 0.0f) {
      y = 0.0f;
      vy = fabsf(vy) * DAMPING;
      if (vy < MIN_SPEED)
        vy = MIN_SPEED;
    } else if (y > bound_b) {
      y = bound_b;
      vy = -(fabsf(vy) * DAMPING);
      // Random floor kick — 50% chance of extra upward impulse (matches Lua)
      if (rng_next(s) < 0.5f) {
        vy -= 3.0f + rng_next(s) * 4.0f;
      }
    }

    b->vx = vx;
    b->vy = vy;
    b->x = x;
    b->y = y;
    pd->graphics->drawBitmap(bmp, (int)x, (int)y, kBitmapUnflipped);
  }

  // --- Spawner crosshair ---
  int spx = (int)s->spawnerX;
  pd->graphics->drawLine(spx - 5, SPAWNER_Y, spx + 5, SPAWNER_Y, 2,
                         kColorBlack);
  pd->graphics->drawLine(spx, SPAWNER_Y - 5, spx, SPAWNER_Y + 5, 2,
                         kColorBlack);

  // --- HUD: actual FPS top-right, stats bottom ---
  pd->system->drawFPS(WIDTH - MARGIN - 14, MARGIN);

  char buf[32];
  snprintf(buf, sizeof(buf), "Bunnies: %d", n);
  pd->graphics->drawText(buf, strlen(buf), kASCIIEncoding, MARGIN,
                         HEIGHT - MARGIN - 14);
  pd->graphics->drawText("C", 1, kASCIIEncoding, 180, HEIGHT - MARGIN - 14);
  snprintf(buf, sizeof(buf), "Target: %d FPS", s->fpsTarget);
  pd->graphics->drawText(buf, strlen(buf), kASCIIEncoding, WIDTH - 120,
                         HEIGHT - MARGIN - 14);

  return 1;
}

#ifdef _WINDLL
__declspec(dllexport)
#endif
int eventHandler(PlaydateAPI *playdate, PDSystemEvent event, uint32_t arg) {
  (void)arg;
  pd = playdate;

  if (event == kEventInit) {
    g_state = pd->system->realloc(NULL, sizeof(GameState));

    g_state->capacity = INITIAL_CAPACITY;
    g_state->bunnies =
        pd->system->realloc(NULL, sizeof(Bunny) * INITIAL_CAPACITY);
    g_state->count = 0;
    g_state->spawnerX = (float)(WIDTH / 2);
    g_state->cooldown = 0;
    g_state->fpsTarget = 30;
    g_state->rng = pd->system->getCurrentTimeMilliseconds();

    const char *err = NULL;

    g_state->bitmap = pd->graphics->loadBitmap("bunny", &err);
    pd->graphics->getBitmapData(g_state->bitmap, &g_state->bitmapW,
                                &g_state->bitmapH, NULL, NULL, NULL);

    g_state->font = pd->graphics->loadFont(
        "/System/Fonts/Asheville-Sans-14-Bold.pft", &err);
    pd->graphics->setFont(g_state->font);

    pd->display->setRefreshRate(g_state->fpsTarget);

    spawn_batch(g_state);

    pd->system->setUpdateCallback(update, g_state);
  }

  if (event == kEventTerminate) {
    if (g_state) {
      pd->system->realloc(g_state->bunnies, 0);
      pd->system->realloc(g_state, 0);
      g_state = NULL;
    }
  }

  return 0;
}
