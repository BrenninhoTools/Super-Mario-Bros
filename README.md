# Super-Mario-Bros

A native port of Super Mario Bros. (NES) written in C++ for Android, Windows and Linux.

The game is not emulated. The original 6502 code from the disassembly in `asm/smbdis.asm` is statically translated into C++ at build time by `tools/smb2cpp.py`, and runs on a native platform layer built on SDL2 (video, audio, input and touch controls).

## ROM

No Nintendo assets are included. You need your own dump of one of these cartridges:

- **Super Mario Bros. (World)** (`.nes`, iNES header optional)
- **Super Mario Bros. / Duck Hunt (USA)** (mapper 66 multicart): only the Super Mario Bros. banks are extracted, and the multicart reset stub is restored to the original code

The program checks the PRG data and only accepts these versions.

- **Windows / Linux:** drag and drop the ROM onto the window on first launch, or place it next to the executable as `baserom.nes`, or pass its path as the first argument. It is copied to the user data folder so it only needs to be done once.
- **Android:** the file picker opens on first launch. Select the ROM and it is stored in the app's private storage.

## Controls

| NES      | Keyboard              | Gamepad        |
|----------|-----------------------|----------------|
| D-Pad    | Arrows / WASD         | D-Pad / Left stick |
| A (jump) | X / K / Space         | A / Y          |
| B (run)  | Z / J / Left Shift    | B / X          |
| Start    | Enter                 | Start          |
| Select   | Right Shift / Tab     | Back           |

Extra keys: `F11` or `Alt+Enter` fullscreen, `F2` reset, `P` pause, `Esc` quit.

On Android, NES-style touch controls are shown, with an analog stick for movement (8 directions, the finger that grabs it keeps control even outside the base). The **B** button is a run toggle: tap it once and B stays held (a yellow ring and `RUN` appear), tap it again to release. Each time it turns on it also throws a fireball when Mario has the fire flower. Using a gamepad hides the touch controls, and touching the screen shows them again.

## Building

Clone with submodules:

```
git clone --recursive https://github.com/BrenninhoTools/Super-Mario-Bros
```

Requirements: CMake 3.16+, a C++17 compiler and Python 3.

### Windows / Linux

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

On Linux, install the usual SDL2 build dependencies (X11/Wayland, ALSA/PulseAudio). Use `-DSMB_SYSTEM_SDL=ON` to link against a system SDL2 instead of `external/SDL`.

### Android

Requires the Android SDK, NDK `27.2.12479018` and CMake `3.22.1`.

```
cd android
./gradlew assembleRelease
```

The APK is written to `android/app/build/outputs/apk/release/`.

## How it works

1. `tools/smb2cpp.py` assembles `smbdis.asm` to get the exact address of every instruction and checks that the result matches the original program layout and vectors.
2. Every instruction becomes a block of C++ with a label at its original address. Branches and jumps become `goto`, `jsr`/`rts` use the real 6502 stack and a dispatch table, and indirect jumps through `JumpEngine` dispatch over every pointer table target.
3. The generated `Cpu::execute` runs the reset code once and the NMI handler once per frame, exactly like the console does.
4. Data tables and graphics are read from the user's ROM. The PPU renders scanline by scanline with the sprite 0 split for the status bar, and the APU synthesizes both pulse channels, triangle and noise.

## Credits

- Disassembly by doppelganger (`asm/smbdis.asm`).
- [SDL2](https://github.com/libsdl-org/SDL) for the platform layer.
- Super Mario Bros. is © Nintendo. This project is not affiliated with Nintendo.
