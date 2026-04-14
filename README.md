# PDBunnymark

Benchmark utility for Playdate comparing Lua and C performance via the classic
bunnymark test: spawn as many bouncing sprites as possible while holding a
stable target framerate.

[📹 Watch the video!](https://www.youtube.com/watch?v=eKmgUG1JFGE)

[Read about it on the Playdate dev forum](https://devforum.play.date/t/how-much-faster-is-c-than-lua-for-playdate-development/25381)

Install and use `just` for running dev commands. The `just` commands assume
you're on macOS. See the `justfile` for available commands.

Reference and inspiration:

- [OpenFL's BunnyMark](https://github.com/openfl/openfl-samples/tree/master/demos/BunnyMark)
- [Tetra's Bunnymark](https://github.com/17cupsofcoffee/tetra/blob/d769782c5ca30b4d960e924a3d3808473ba7359f/examples/bunnymark.rs)

## Controls

| Input        | Action                                              |
| ------------ | --------------------------------------------------- |
| A (hold)     | Spawn +10 bunnies                                   |
| B (hold)     | Remove -10 bunnies                                  |
| Left / Right | Move spawn point                                    |
| Crank        | Move spawn point (one full rotation = screen width) |
| Up / Down    | Step target FPS (10–50 in increments of 10)         |

## Results

**On device — highest stable bunny count at target framerate**

|           | 30 FPS | 50 FPS |
| --------- | ------ | ------ |
| **Lua**   | 240    | 120    |
| **C**     | 810    | 430    |
| **Ratio** | 3.4×   | 3.6×   |

**KEY TAKEAWAY:** C is about **~3.5×** faster than Lua (for this benchmark).

By comparison, on my M2 MacBook Pro, I get about 55,000 bunnies in Tetra (Rust +
SDL3) before frames start to drop.

### Caveats

The results can of course vary and this is just a simplistic benchmark. **This
workload is draw-heavy** and **hardware-specific.** These numbers are for
running on device. The Playdate Simulator results are very different. This is
why we test on device!

## Performance optimizations

Both versions apply the same high-level strategies, then diverge in the
language-specific details.

## Learn to make games for Playdate

I did this little benchmark exercise as part of research for
[Make Games for Playdate with Lua](https://leanpub.com/playdatebook), a book I
wrote about game programming for Playdate. If you're curious about how to make
games for Playdate, check it out. There are free sample chapters too.

## Unlicense

This program—its source and its assets—are released into the public domain.
