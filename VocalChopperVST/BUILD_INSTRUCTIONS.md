# Vocal Chopper VST3 — Build Instructions

## What you need installed

| Tool | Version | Download |
|------|---------|----------|
| CMake | 3.22+ | https://cmake.org/download/ |
| Clang or MSVC | Any modern | (Windows: Visual Studio 2022 Community) |
| Git | Any | https://git-scm.com |

JUCE is downloaded automatically by CMake (no separate install needed).

---

## Windows (FL Studio is Windows-first)

```bash
# 1. Clone / unzip this project somewhere, e.g.:
cd C:\dev\VocalChopperVST

# 2. Configure (downloads JUCE ~500 MB first time)
cmake -B build -G "Visual Studio 17 2022" -A x64

# 3. Build Release
cmake --build build --config Release

# 4. The .vst3 file will be at:
#    build\VocalChopper_artefacts\Release\VST3\VocalChopper.vst3

# 5. Copy to FL Studio's VST folder:
#    Default: C:\Program Files\Common Files\VST3\
#    Or wherever you pointed FL Studio in Options > File Settings > VST plugins

# 6. In FL Studio: Options > Manage plugins > Scan  (or rescan)
#    Search for "Vocal Chopper" in the plugin browser
```

## macOS

```bash
cd ~/dev/VocalChopperVST
cmake -B build -G Xcode
cmake --build build --config Release

# .vst3 lands at:
# build/VocalChopper_artefacts/Release/VST3/VocalChopper.vst3

# Copy to: /Library/Audio/Plug-Ins/VST3/
# Then rescan in FL Studio (macOS)
```

## Linux

```bash
sudo apt install libasound2-dev libx11-dev libxinerama-dev \
                 libxext-dev libfreetype-dev libwebkit2gtk-4.0-dev

cd ~/dev/VocalChopperVST
cmake -B build
cmake --build build --config Release
# .vst3 at: build/VocalChopper_artefacts/Release/VST3/
```

---

## Loading in FL Studio

1. Open FL Studio
2. **Options → File Settings → VST plugins** — make sure your VST3 folder is listed
3. **Plugin database (left panel)** → right-click → "Find plugins"
4. Search "Vocal Chopper" → drag onto a mixer channel or effects chain
5. The plugin opens as an effects panel inside FL Studio

---

## How to use the plugin

### Chop tab
- **Load Vocal** button or drag a .wav/.aif file onto the waveform
- **Left-click** the waveform to add a chop point at that position
- **Right-click** to remove the nearest chop point
- **Auto-detect transients** — energy-delta algorithm finds natural cut points
- **Divide by beat** — splits evenly based on the snap setting (1/4, 1/8, 1/16)
- **Copy chop guide** — copies timestamped instructions for Edison to clipboard

### Fades tab
- Two fade curve editors (fade in / fade out)
- **Click** the curve canvas to cycle through: Linear → Exponential → S-Curve → Logarithmic
- **Drag horizontally** on the canvas to adjust duration
- Duration slider also controls the fade length
- "Apply fades on playback" toggles whether fades are applied during internal preview

### Align tab
- Set your BPM in the toolbar (or it auto-reads from the FL Studio host)
- Click **Analyze offset** to detect how far your vocal's first onset is from the beat grid
- **Nudge** buttons shift the analysis by ±10ms so you can fine-tune
- **Copy FL instructions** copies a plain-text fix guide to clipboard

### Playback
- **Play** previews the vocal (with fades applied if enabled)
- **Stop** stops playback
- The white playhead line moves through the waveform

---

## Extending the plugin

The codebase is organized so each concern is isolated:

| File | Responsibility |
|------|---------------|
| `ChopEngine` | All chop point logic, transient detection, region management |
| `FadeDesigner` | Fade curve math and the visual canvas component |
| `AlignmentAnalyzer` | Beat grid math, onset detection, FL instruction generation |
| `WaveformDisplay` | Waveform rendering, zoom, pan, chop marker drawing |
| `PluginProcessor` | Audio playback, parameter state, host sync |
| `PluginEditor` | Main GUI, tab layout, wiring everything together |
| `LookAndFeel` | Dark theme colors and custom control rendering |

**To add pitch detection**: add a new tab in `PluginEditor`, use `juce::dsp::FFT` in a helper class.  
**To add MIDI output** (trigger chops from pads): enable MIDI output in CMakeLists and emit `MidiMessage::noteOn` from `processBlock`.  
**To connect to FL's automation**: all sliders are registered in `params` (`AudioProcessorValueTreeState`) and show up as automatable parameters automatically.

---

## v2 — What's new

### Pad tab (MPC-style 4×4 grid)
- 16 pads auto-mapped to chop regions (C2 upward = R1, R2, ... matching FL FPC layout)
- Left-click a pad to preview that region instantly
- Right-click a pad → MIDI learn → play any note on your controller to remap it
- Pads flash on trigger (from mouse OR incoming MIDI)
- Pad shows region number, duration, and mapped MIDI note

### Live pitch detection (toolbar)
- YIN algorithm runs in real time during playback
- Current pitch shown in toolbar as note name + Hz (e.g. "A4  440Hz")
- Confidence threshold 0.65 — only displays if reliably detected

### Per-region pitch analysis (Chop tab)
- Click "Analyze pitch" after adding chop points
- Runs YIN across every region, shows root note + Hz + MIDI number in the region list
- Chop guide export now includes pitch info per region

### Interactive alignment timeline (Align tab)
- Visual beat grid with bar/beat/16th markers
- Your vocal regions shown as draggable blocks on the grid
- Orange arrow shows measured offset with ms label
- Green "snap target" line shows nearest beat
- Drag any region left/right to nudge its timing
- Scroll with mouse wheel, zoom with Ctrl+scroll

### MIDI triggering
- Any MIDI note-on hitting the mapped note fires the corresponding region
- Velocity scales output gain
- Default mapping matches FL FPC (C2 = pad 1)
- Full MIDI learn via right-click on any pad

### Module summary
| File | Role |
|------|------|
| PitchDetector | YIN algorithm, live + per-region batch analysis |
| MidiChopTrigger | MIDI→region mapping, MIDI learn, velocity |
| PadGrid | 4×4 visual pad grid, flash animation, learn UI |
| AlignmentTimeline | Interactive beat grid, draggable regions, offset arrow |
