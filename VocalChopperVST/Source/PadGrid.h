#pragma once
#include <JuceHeader.h>
#include "MidiChopTrigger.h"
#include "ChopEngine.h"

// 4x4 pad grid — each pad fires a chop region.
// Pads flash on trigger and show MIDI note + region info.
class PadGrid : public juce::Component, public juce::Timer
{
public:
    PadGrid(MidiChopTrigger& trigger, ChopEngine& chop);
    ~PadGrid() override;

    // Call this when regions change to rebuild pad labels
    void refreshFromRegions();

    // Call from audio thread (via async) to flash a pad
    void flashPad(int regionIdx);

    // Callbacks
    std::function<void(int regionIdx)> onPadClicked;
    std::function<void(int padSlot)>   onLearnRequested;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;
    void timerCallback() override;

private:
    MidiChopTrigger& trigger_;
    ChopEngine&      chopEngine_;

    static constexpr int kCols = 4;
    static constexpr int kRows = 4;
    static constexpr int kPads = kCols * kRows;

    struct PadState
    {
        bool  active     = false;
        bool  flashing   = false;
        int   flashFrames = 0;
        float flashIntensity = 0.f;
        juce::String topLabel;
        juce::String botLabel;
        juce::Colour baseColour;
    };

    std::array<PadState, kPads> pads_;
    int learningPad_ = -1;

    juce::Rectangle<int> getPadBounds(int idx) const;
    void drawPad(juce::Graphics& g, int idx);

    static const juce::Colour kPadColours[];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PadGrid)
};
