cmake_minimum_required(VERSION 3.22)
project(VocalChopper VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Download JUCE if not present ──────────────────────────────────────────────
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        7.0.9
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

# ── Plugin target ─────────────────────────────────────────────────────────────
juce_add_plugin(VocalChopper
    COMPANY_NAME            "YourStudio"
    PLUGIN_MANUFACTURER_CODE Ystd
    PLUGIN_CODE             VCh1
    FORMATS                 VST3 Standalone
    PRODUCT_NAME            "Vocal Chopper"
    IS_SYNTH                FALSE
    NEEDS_MIDI_INPUT        TRUE
    NEEDS_MIDI_OUTPUT       FALSE
    IS_MIDI_EFFECT          FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    VST3_CATEGORIES         "Fx" "Tools"
)

juce_generate_juce_header(VocalChopper)

# ── Sources ───────────────────────────────────────────────────────────────────
target_sources(VocalChopper PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/WaveformDisplay.cpp
    Source/WaveformDisplay.h
    Source/ChopEngine.cpp
    Source/ChopEngine.h
    Source/FadeDesigner.cpp
    Source/FadeDesigner.h
    Source/AlignmentAnalyzer.cpp
    Source/AlignmentAnalyzer.h
    Source/AlignmentTimeline.cpp
    Source/AlignmentTimeline.h
    Source/MidiChopTrigger.cpp
    Source/MidiChopTrigger.h
    Source/PitchDetector.cpp
    Source/PitchDetector.h
    Source/PadGrid.cpp
    Source/PadGrid.h
    Source/NoiseGate.cpp
    Source/NoiseGate.h
    Source/CleanTab.cpp
    Source/CleanTab.h
    Source/StretchEngine.cpp
    Source/StretchEngine.h
    Source/StretchTab.cpp
    Source/StretchTab.h
    Source/SpectrumAnalyzer.cpp
    Source/SpectrumAnalyzer.h
    Source/UndoHistory.h
    Source/PresetManager.h
    Source/LookAndFeel.h
    Source/UIComponents.h
)

# ── JUCE modules ──────────────────────────────────────────────────────────────
target_compile_definitions(VocalChopper PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(VocalChopper PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_audio_formats
    juce::juce_dsp
    juce::juce_gui_basics
    juce::juce_gui_extra
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
