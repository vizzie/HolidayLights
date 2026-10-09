# Simulator

Runs the real animation code (`effects.cpp`, `pixel_map.cpp`, `seasons.cpp`, unmodified) in a desktop window instead of on the Yun, via `sim_main.cpp` and SDL2.

## Setup (one-time)

```bash
brew install sdl2
```

## Build and run

The quickest way builds and launches in one step, from any directory:

```bash
tools/run_sim.sh
```

Or do the two steps yourself:

```bash
~/.platformio/penv/bin/pio run -e native
.pio/build/native/program
```

The first command compiles the sim (rerun it after any code change). The second opens a window that shows every LED as a dot at its real (x, y) position, running at the firmware's ~30ms frame rate.

Run the binary directly from your own terminal, not through `pio run -t exec` — that routes stdio through PlatformIO's own subprocess handling instead of a real terminal/window session and doesn't work right.

The full `~/.platformio/penv/bin/pio` path is needed because the PlatformIO VSCode extension doesn't put `pio` on your shell's PATH. You can also build from the PlatformIO sidebar by picking the `native` environment, then run the binary from a terminal as above.

## Controls

- Any key: advance to the next effect in the active season's rotation (the current effect's name is shown in the window title)
- Close the window, or Ctrl-C: quit

## Previewing a different date

Edit `simMonth`/`simDay` near the top of `sim_main.cpp` and rebuild. The date picks the season, which sets both the palette and which effects are in the rotation (see the `SEASONS` table in `seasons.cpp`). A date outside every season, like the default 9/15, uses the Default season, which has every effect.

Unlike the firmware, the sim doesn't switch effects on a timer; it stays on one effect until you press a key.

## Troubleshooting

- **`SDL.h` not found / linker can't find `-lSDL2`:** the `native` env in `platformio.ini` expects Homebrew's Apple Silicon paths (`/opt/homebrew`). On an Intel Mac, change those two paths to `/usr/local/include/SDL2` and `/usr/local/lib`.
- **`pio test -e native` errors:** there are no unit tests yet (`test/` only has a README), so this is expected.
