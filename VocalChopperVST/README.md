# Vocal Chopper VST3

A professional vocal editing plugin for FL Studio (and any VST3 host).

## Features

- **Chop** — click waveform to add cut points, auto-detect transients, beat-divide
- **Pads** — 4×4 MPC-style pad grid, MIDI-triggerable, maps to FL FPC layout by default
- **Fades** — per-region fade-in/out with Linear, Expo, S-Curve, and Log shapes
- **Align** — beat grid timeline, offset detection, nudge and snap-to-beat
- **Clean** — auto noise gate strips silence and background hiss between vocal phrases
- **Stretch** — per-region pitch shift (±24 semitones) and time stretch (0.25×–4×)
- Preset save/load with 6 factory presets
- Undo/redo (40 steps)
- Live YIN pitch detection in toolbar
- FFT spectrum overlay
- Keyboard shortcuts (Space=play, Cmd+Z=undo, Cmd+S=save preset)
- Resizable window (600×440 → 1200×800)

---

## Download the compiled plugin

Every push to `main` automatically builds the `.vst3` for **Windows, macOS, and Linux**.

1. Go to the **Actions** tab above
2. Click the latest green workflow run
3. Scroll to **Artifacts** at the bottom
4. Download **VocalChopper-Windows-VST3** (or macOS / Linux)

---

## Install in FL Studio (Windows)

1. Download `VocalChopper-Windows-VST3` from Actions (see above)
2. Unzip it — you'll get a folder called `VocalChopper.vst3`
3. Copy that folder to:
   ```
   C:\Program Files\Common Files\VST3\
   ```
4. Open FL Studio → Options → Manage plugins → **Find plugins**
5. Search **Vocal Chopper** in the plugin browser
6. Drag it onto a mixer channel

---

## Install on macOS

1. Download `VocalChopper-macOS-VST3`
2. Copy `VocalChopper.vst3` to:
   ```
   /Library/Audio/Plug-Ins/VST3/
   ```
3. Rescan plugins in your DAW

---

## Build from source yourself

**Requirements:** CMake 3.22+, Visual Studio 2022 (Windows) or Xcode (Mac), Git

```bash
git clone https://github.com/YOUR_USERNAME/VocalChopper.git
cd VocalChopper

# Windows
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release

# macOS
cmake -B build -G Xcode -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
cmake --build build --config Release

# Linux
cmake -B build
cmake --build build --config Release
```

The `.vst3` will appear at `build/VocalChopper_artefacts/Release/VST3/`

JUCE is downloaded automatically (~500 MB, first build only). Build time: ~15 min.

---

## Keyboard shortcuts

| Key | Action |
|-----|--------|
| `Space` | Play / Stop |
| `Cmd/Ctrl + Z` | Undo |
| `Cmd/Ctrl + Shift + Z` | Redo |
| `Cmd/Ctrl + S` | Save preset |
| `Cmd/Ctrl + ←` | Nudge alignment −10ms |
| `Cmd/Ctrl + →` | Nudge alignment +10ms |
| `Delete` | Clear chop points (waveform focused) |

---

## License

MIT — free to use, modify, and distribute.
