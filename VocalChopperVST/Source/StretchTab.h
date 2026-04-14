#pragma once
#include <JuceHeader.h>
#include "StretchEngine.h"
#include "ChopEngine.h"
#include "SpectrumAnalyzer.h"

// Card for a single region's stretch/pitch controls
class RegionStretchCard : public juce::Component
{
public:
    RegionStretchCard(int regionId, StretchEngine& engine, juce::Colour colour,
                       double regionDurSec = 0.1);

    void refreshFromEngine();

    std::function<void(int regionId)> onSettingsChanged;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    int            regionId_;
    StretchEngine& engine_;
    juce::Colour   colour_;

    juce::ToggleButton enableBtn_;
    juce::Slider       pitchSlider_;
    juce::Slider       stretchSlider_;
    juce::ToggleButton preservePitchBtn_;
    juce::Label        pitchLabel_   {{}, "Pitch"};
    juce::Label        stretchLabel_ {{}, "Stretch"};
    juce::Label        idLabel_;
    juce::Label        srcDurLabel_;
    juce::Label        durLabel_;
    double             regionDurSec_ = 0.1;
    void               updateDurLabel();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RegionStretchCard)
};

// Full Stretch tab — scrollable list of per-region cards + spectrum view
class StretchTab : public juce::Component
{
public:
    StretchTab(StretchEngine& engine, ChopEngine& chop, SpectrumAnalyzer& spectrum);

    void refreshFromRegions();

    // Called by editor after processing completes
    void onProcessingDone();

    std::function<void(int regionId)> onRegionProcessRequested;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    StretchEngine&   engine_;
    ChopEngine&      chop_;
    SpectrumAnalyzer& spectrum_;

    juce::Viewport   viewport_;
    juce::Component  cardContainer_;

    std::vector<std::unique_ptr<RegionStretchCard>> cards_;

    juce::TextButton applyAllBtn_  { "Render all regions" };
    juce::TextButton resetAllBtn_  { "Reset all" };
    juce::ToggleButton spectrumBtn_{ "Show spectrum" };

    SpectrumOverlay  specOverlay_;
    juce::Label      hintLabel_;

    static const juce::Colour kRegionColours_[];
    void layoutCards();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StretchTab)
};
