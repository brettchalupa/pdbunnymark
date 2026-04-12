local gfx <const> = playdate.graphics

local WIDTH <const> = 400
local HEIGHT <const> = 240
local MARGIN <const> = 4

-- Benchmark config
local BATCH_SIZE <const> = 10
local BTN_COOLDOWN <const> = 8        -- frames between held-button repeats
local SPAWNER_SPEED <const> = 8       -- px/frame
local CRANK_SCALE <const> = 400 / 360 -- px per degree; one full rotation = screen width
local SPAWNER_Y <const> = 16          -- fixed vertical track near top of screen

-- Physics (per-frame units; behavior scales with refresh rate)
local GRAVITY <const> = 0.5
local DAMPING <const> = 0.85
local MIN_SPEED <const> = 0.5

-- FPS target stepping
local FPS_MIN <const> = 10
local FPS_MAX <const> = 50
local FPS_STEP <const> = 10
local fpsTarget = 30

-- Setup
gfx.setFont(gfx.font.new("/System/Fonts/Asheville-Sans-14-Bold.pft"))
gfx.setLineWidth(2)
playdate.display.setRefreshRate(fpsTarget)

local bunnyImg <const> = gfx.image.new("bunny.png")
local bunnyW, bunnyH = bunnyImg:getSize()
local boundR = WIDTH - bunnyW
local boundB = HEIGHT - bunnyH

-- State
local bunnies = {}
local bunnyCount = 0
local spawnerX = WIDTH // 2
local cooldown = 0

local function spawnBatch()
  local x, y = spawnerX, SPAWNER_Y
  for _ = 1, BATCH_SIZE do
    bunnyCount += 1
    bunnies[bunnyCount] = {
      x = x,
      y = y,
      vx = (math.random() - 0.5) * 10,
      vy = (math.random() - 0.8) * 10,
    }
  end
end

local function despawnBatch()
  local n = bunnyCount - BATCH_SIZE
  if n < 0 then n = 0 end
  for i = bunnyCount, n + 1, -1 do
    bunnies[i] = nil
  end
  bunnyCount = n
end

spawnBatch()

function playdate.update()
  local pd = playdate

  -- Left/Right + crank: slide spawner along horizontal track
  local dx = pd.getCrankChange() * CRANK_SCALE
  if pd.buttonIsPressed(pd.kButtonLeft) then
    dx -= SPAWNER_SPEED
  elseif pd.buttonIsPressed(pd.kButtonRight) then
    dx += SPAWNER_SPEED
  end
  if dx ~= 0 then
    spawnerX += dx
    if spawnerX < 0 then spawnerX = 0 end
    if spawnerX > WIDTH then spawnerX = WIDTH end
  end

  -- Up/Down: step FPS target (one step per press, 10-50)
  if pd.buttonJustPressed(pd.kButtonUp) then
    if fpsTarget < FPS_MAX then
      fpsTarget += FPS_STEP
      pd.display.setRefreshRate(fpsTarget)
    end
  elseif pd.buttonJustPressed(pd.kButtonDown) then
    if fpsTarget > FPS_MIN then
      fpsTarget -= FPS_STEP
      pd.display.setRefreshRate(fpsTarget)
    end
  end

  -- A/B: spawn or remove bunnies, repeats while held
  if cooldown > 0 then
    cooldown -= 1
  elseif pd.buttonIsPressed(pd.kButtonA) then
    spawnBatch()
    cooldown = BTN_COOLDOWN
  elseif pd.buttonIsPressed(pd.kButtonB) then
    despawnBatch()
    cooldown = BTN_COOLDOWN
  end

  gfx.clear()

  -- Localize everything touched in the hot loop
  local img = bunnyImg
  local g = GRAVITY
  local dmp = DAMPING
  local ms = MIN_SPEED
  local br = boundR
  local bb = boundB
  local bs = bunnies
  local n = bunnyCount
  local abs = math.abs
  local floor = math.floor
  local rnd = math.random

  -- Single-pass physics + draw
  for i = 1, n do
    local b = bs[i]
    local vx = b.vx
    local vy = b.vy + g
    local x = b.x + vx
    local y = b.y + vy

    if x < 0 then
      x = 0
      vx = abs(vx) * dmp
      if vx < ms then vx = ms end
    elseif x > br then
      x = br
      vx = -(abs(vx) * dmp)
      if vx > -ms then vx = -ms end
    end

    if y < 0 then
      y = 0
      vy = abs(vy) * dmp
      if vy < ms then vy = ms end
    elseif y > bb then
      y = bb
      vy = -(abs(vy) * dmp)
      if rnd() < 0.5 then
        vy -= 3 + rnd() * 4
      end
    end

    b.vx = vx
    b.vy = vy
    b.x = x
    b.y = y
    img:draw(floor(x), floor(y))
  end

  -- Spawner crosshair on the horizontal track
  local sx = spawnerX
  gfx.drawLine(sx - 5, SPAWNER_Y, sx + 5, SPAWNER_Y)
  gfx.drawLine(sx, SPAWNER_Y - 5, sx, SPAWNER_Y + 5)

  -- HUD: actual FPS top-right, stats bottom-left
  pd.drawFPS(WIDTH - MARGIN - 14, MARGIN)
  gfx.drawText("Bunnies: " .. n, MARGIN, HEIGHT - MARGIN - 14)
  gfx.drawText("Lua", 180, HEIGHT - MARGIN - 14)
  gfx.drawText("Target: " .. fpsTarget .. " FPS", WIDTH - 120, HEIGHT - MARGIN - 14)
end
