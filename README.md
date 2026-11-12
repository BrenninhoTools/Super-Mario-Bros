# Super-Mario-Bros

A native port of Super Mario Bros. (NES) written in C++ for Android, iOS, Windows, macOS and Linux.

The game is not emulated. The original 6502 code from the disassembly in `asm/smbdis.asm` is statically translated into C++ at build time by `tools/smb2cpp.py`, and runs on a native platform layer built on SDL2 (video, audio, input and touch controls).

## ROM

No Nintendo assets are included. You need your own dump of one of these cartridges:

- **Super Mario Bros. (World)** (`.nes`, iNES header optional)
- **Super Mario Bros. / Duck Hunt (USA)** (mapper 66 multicart): only the Super Mario Bros. banks are extracted, and the multicart reset stub is restored to the original code

The program checks the PRG data and only accepts these versions.

The ROM can also be inside a `.zip` file: the archive is scanned and the first entry that is a supported ROM is used (`.nes` files are tried first, folders and `__MACOSX` entries are skipped). Stored and Deflate compression are supported; password-protected archives are not.

- **Windows / Linux:** drag and drop the ROM (`.nes` or `.zip`) onto the window on first launch, or place it next to the executable as `baserom.nes`, or pass its path as the first argument. It is copied to the user data folder so it only needs to be done once.
- **macOS:** drag the ROM onto the window or onto the app icon. The app is ad-hoc signed, so the first launch needs right-click → Open.
- **Android:** the file picker opens on first launch. Select the ROM and it is stored in the app's private storage.
- **iOS:** the file picker opens on first launch, or copy the ROM as `baserom.nes` into the app's folder in the Files app (On My iPhone → Super Mario Bros.). The IPA is unsigned, so install it with AltStore, SideStore or TrollStore.

## Controls

| NES      | Keyboard              | Gamepad        |
|----------|-----------------------|----------------|
| D-Pad    | Arrows / WASD         | D-Pad / Left stick |
| A (jump) | X / K / Space         | A / Y          |
| B (run)  | Z / J / Left Shift    | B / X          |
| Start    | Enter                 | Start          |
| Select   | Right Shift / Tab     | Back           |

Extra keys: `F11` or `Alt+Enter` fullscreen window, `F4` switch between filling the screen and the original aspect ratio, `F2` reset, `P` pause, `F3` performance overlay, `Esc` quit.

The game fills the whole screen by default. The button in the top-right corner (shown when the mouse moves on desktop, always on mobile) switches back to the original aspect ratio, and the choice is saved.

On Android and iOS, NES-style touch controls are shown, with an analog stick for movement (8 directions, the finger that grabs it keeps control even outside the base). **A** jumps and **B** runs or throws fireballs while held. The yellow **C** button is a run toggle: tap it once and B stays held (a green ring and `RUN` appear), tap it again to release. While running with C, tapping B still throws a fireball. In fill mode the controls become translucent so the game stays visible. Using a gamepad hides the touch controls, and touching the screen shows them again.

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

### macOS

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
cmake --build build --parallel
```

The app bundle is `build/SuperMarioBros.app`.

### iOS

```
cmake -S . -B build-ios -G Xcode -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_ARCHITECTURES=arm64
cmake --build build-ios --config Release -- -sdk iphoneos CODE_SIGNING_ALLOWED=NO
```

Or open `build-ios/SuperMarioBros.xcodeproj` in Xcode to sign and run it on a device.

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

## Performance

- **Frame pacing:** when the display runs at a multiple of 60 Hz (60, 120, 240, including 59.94), the game advances exactly one frame every 1 or 2 vsyncs, so there are no doubled or skipped frames. Other refresh rates (90, 144 Hz) or a display without working vsync fall back to a precise timer.
- **Audio sync:** instead of dropping audio when the queue drifts, the output rate is nudged by up to ±0.5% to keep a small, constant buffer. This removes crackles and keeps latency low (about 2 frames on desktop, 3 on mobile).
- **Renderer:** the background is decoded 8 pixels at a time with lookup tables, the palette is cached per scanline and lines without sprites take a fast path. Output is pixel-identical to the reference renderer, and a full frame (game logic, video and audio) takes about 0.25 ms on a desktop CPU.
- **Android:** the app asks for a 120 or 60 Hz display mode so frame pacing can lock, enables sustained performance mode when the device supports it, and keeps the screen on.

Press `F3` on desktop to see game FPS, display FPS, frame time, pacing mode and audio queue.

## Credits

- Disassembly by doppelganger (`asm/smbdis.asm`).
- [SDL2](https://github.com/libsdl-org/SDL) for the platform layer.
- Super Mario Bros. is © Nintendo. This project is not affiliated with Nintendo.
