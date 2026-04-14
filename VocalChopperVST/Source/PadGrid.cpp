#include "PadGrid.h"

const juce::Colour PadGrid::kPadColours[] = {
    juce::Colour(0xff534AB7), juce::Colour(0xff0F6E56), juce::Colour(0xff993C1D),
    juce::Colour(0xff993556), juce::Colour(0xff185FA5), juce::Colour(0xff3B6D11),
    juce::Colour(0xff854F0B), juce::Colour(0xffA32D2D), juce::Colour(0xff534AB7),
    juce::Colour(0xff0F6E56), juce::Colour(0xff993C1D), juce::Colour(0xff993556),
    juce::Colour(0xff185FA5), juce::Colour(0xff3B6D11), juce::Colour(0xff854F0B),
    juce::Colour(0xffA32D2D)
};

PadGrid::PadGrid(MidiChopTrigger& t, ChopEngine& c)
    : trigger_(t), chopEngine_(c)
{
    for (int i = 0; i < kPads; ++i)
    {
        pads_[(size_t)i].baseColour = kPadColours[i % kPads];
        pads_[(size_t)i].topLabel   = "-";
        pads_[(size_t)i].botLabel   = MidiChopTrigger::midiNoteToName(36 + i);
    }
    startTimerHz(30);
}

PadGrid::~PadGrid() { stopTimer(); }

void PadGrid::refreshFromRegions()
{
    const auto& regions  = chopEngine_.getRegions();
    const auto& mappings = trigger_.getMappings();

    for (int i = 0; i < kPads; ++i)
    {
        auto& pad = pads_[(size_t)i];
        pad.active = false;
        pad.topLabel = "-";

        if (i < (int)mappings.size())
        {
            int ri = mappings[(size_t)i].regionIdx;
            pad.botLabel = MidiChopTrigger::midiNoteToName(mappings[(size_t)i].midiNote);

            if (ri < (int)regions.size())
            {
                pad.active   = true;
                pad.topLabel = "R" + juce::String(regions[(size_t)ri].id);
                double len = regions[(size_t)ri].endSec - regions[(size_t)ri].startSec;
                pad.botLabel += "  " + juce::String(len * 1000.0, 0) + "ms";
                pad.baseColour = regions[(size_t)ri].colour.darker(0.4f);
            }
        }
    }
    repaint();
}

void PadGrid::flashPad(int regionIdx)
{
    const auto& mappings = trigger_.getMappings();
    for (int i = 0; i < (int)mappings.size() && i < kPads; ++i)
    {
        if (mappings[(size_t)i].regionIdx == regionIdx)
        {
            pads_[(size_t)i].flashing     = true;
            pads_[(size_t)i].flashFrames  = 8;
            pads_[(size_t)i].flashIntensity = 1.f;
        }
    }
}

void PadGrid::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff0d0d1a));
    for (int i = 0; i < kPads; ++i)
        drawPad(g, i);
}

juce::Rectangle<int> PadGrid::getPadBounds(int idx) const
{
    int col = idx % kCols;
    int row = (kRows - 1) - (idx / kCols);   // Bottom row = low notes
    int padW = getWidth()  / kCols;
    int padH = getHeight() / kRows;
    return { col * padW + 2, row * padH + 2, padW - 4, padH - 4 };
}

void PadGrid::drawPad(juce::Graphics& g, int idx)
{
    auto& pad = pads_[(size_t)idx];
    auto  b   = getPadBounds(idx).toFloat();

    // Background
    juce::Colour base = pad.active ? pad.baseColour : juce::Colour(0xff1a1a2e);
    if (pad.flashing)
        base = base.interpolatedWith(juce::Colours::white, pad.flashIntensity * 0.6f);

    g.setColour(base);
    g.fillRoundedRectangle(b, 5.f);

    // Border
    bool isLearning = (learningPad_ == idx);
    g.setColour(isLearning ? juce::Colours::yellow
                           : (pad.active ? base.brighter(0.3f)
                                         : juce::Colour(0xff2a2a4a)));
    g.drawRoundedRectangle(b, 5.f, isLearning ? 2.f : 0.5f);

    if (isLearning)
    {
        g.setColour(juce::Colours::yellow.withAlpha(0.8f));
        g.setFont(10.f);
        g.drawText("LEARN", b, juce::Justification::centred);
        return;
    }

    // Labels
    g.setColour(pad.active ? juce::Colours::white.withAlpha(0.9f)
                           : juce::Colour(0xff3a3a5c));
    g.setFont(juce::Font(11.f, juce::Font::bold));
    g.drawText(pad.topLabel, b.reduced(3.f).removeFromTop(b.getHeight() * 0.5f),
               juce::Justification::centred);

    g.setColour(pad.active ? base.brighter(0.6f) : juce::Colour(0xff2a2a4a));
    g.setFont(9.f);
    g.drawText(pad.botLabel, b.reduced(3.f).removeFromBottom(b.getHeight() * 0.45f),
               juce::Justification::centred);
}

void PadGrid::resized() {}

void PadGrid::mouseDown(const juce::MouseEvent& e)
{
    for (int i = 0; i < kPads; ++i)
    {
        if (getPadBounds(i).contains(e.getPosition()))
        {
            if (e.mods.isRightButtonDown())
            {
                // Right-click: MIDI learn
                learningPad_ = i;
                trigger_.startLearning(i);
                trigger_.onLearnComplete = [this](int /*slot*/, int /*note*/) {
                    learningPad_ = -1;
                    refreshFromRegions();
                    repaint();
                };
                repaint();
                if (onLearnRequested) onLearnRequested(i);
            }
            else if (pads_[(size_t)i].active)
            {
                // Left-click: trigger region
                flashPad(trigger_.getMappings()[(size_t)i].regionIdx);
                if (onPadClicked)
                    onPadClicked(trigger_.getMappings()[(size_t)i].regionIdx);
            }
            return;
        }
    }
}

void PadGrid::timerCallback()
{
    bool needsRepaint = false;
    for (auto& pad : pads_)
    {
        if (pad.flashing)
        {
            pad.flashFrames--;
            pad.flashIntensity = (float)pad.flashFrames / 8.f;
            if (pad.flashFrames <= 0) pad.flashing = false;
            needsRepaint = true;
        }
    }
    if (needsRepaint) repaint();
}
