# Simulator

Runs the real animation code (`effects.cpp`, `pixel_map.cpp`, `seasons.cpp`, unmodified) in a desktop window instead of on the Yun, via `sim_main.cpp` and SDL2.

## Setup (one-time)

```bash
brew install sdl2
```

## Build and run

```bash
~/.platformio/penv/bin/pio run -e native
.pio/build/native/program
```

Run the binary directly from your own terminal, not through `pio run -t exec` — that routes stdio through PlatformIO's own subprocess handling instead of a real terminal/window session and doesn't work right.

## Controls

- Any key: advance to the next effect in the active season's rotation
- Close the window, or Ctrl-C: quit

## Previewing a different date

Edit `simMonth`/`simDay` near the top of `sim_main.cpp` and rebuild.
