#pragma once
#include <JuceHeader.h>
#include "AlignmentAnalyzer.h"
#include "ChopEngine.h"

// Interactive timeline showing beat grid, vocal regions, and offset arrows.
// The user can drag individual regions to nudge them.
class AlignmentTimeline : public juce::Component
{
public:
    AlignmentTimeline(AlignmentAnalyzer& analyzer, ChopEngine& chop);

    void setBpm(double bpm);
    void setVocalDuration(double durationSec);
    void setAlignmentResult(const AlignmentResult& result);
    void setZoom(double secPerPixel);

    // Call this when regions update
    void refreshRegions();

    // Callback: user dragged a region, returns new offset in ms
    std::function<void(int regionIdx, double newOffsetMs)> onRegionDragged;

    void paint(juce::Graphics& g) override;
    void resized() override {}
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e,
                        const juce::MouseWheelDetails& w) override;

private:
    AlignmentAnalyzer& analyzer_;
    ChopEngine&        chopEngine_;

    double bpm_          = 140.0;
    double vocalDuration_= 4.0;
    double secPerPixel_  = 0.01;  // zoom: 10ms per pixel default
    double viewOffsetSec_= 0.0;   // horizontal scroll

    AlignmentResult lastResult_;

    // Per-region drag state
    struct RegionDragState
    {
        int    regionIdx      = -1;
        double dragStartSec   = 0.0;
        double dragOffsetSec  = 0.0;
    };
    RegionDragState drag_;

    std::vector<double> regionOffsets_; // user-applied nudge per region (seconds)

    // Drawing helpers
    void drawBeatGrid(juce::Graphics& g);
    void drawRegions(juce::Graphics& g);
    void drawOffsetArrow(juce::Graphics& g);
    void drawRuler(juce::Graphics& g);
    void drawSnapGuides(juce::Graphics& g);

    float secToX(double t) const { return (float)((t - viewOffsetSec_) / secPerPixel_); }
    double xToSec(float x) const { return viewOffsetSec_ + (double)x * secPerPixel_; }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AlignmentTimeline)
};
