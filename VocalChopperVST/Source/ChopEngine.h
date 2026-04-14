#pragma once
#include <JuceHeader.h>
#include <vector>
#include <functional>

// Represents a single chop region within the audio buffer
struct ChopRegion
{
    int    id;
    double startSec;
    double endSec;
    float  fadeInMs  = 10.f;
    float  fadeOutMs = 10.f;
    bool   muted     = false;
    juce::Colour colour;

    double lengthSec() const { return endSec - startSec; }
};

class ChopEngine
{
public:
    ChopEngine();

    // Set the audio buffer to analyze (shared, not owned)
    void setAudioBuffer(const juce::AudioBuffer<float>* buffer, double sampleRate);

    // Manually add a chop point at the given time in seconds
    void addChopPoint(double timeSec);

    // Remove the nearest chop point within tolerance
    void removeChopPointNear(double timeSec, double toleranceSec = 0.05);

    // Auto-detect transients using a simple energy-delta algorithm
    void detectTransients(float sensitivityDb = -20.f);

    // Divide the region evenly into N slices
    void divideEvenly(int numSlices);

    // Clear all chop points
    void clearAll();

    // Get the list of regions derived from current chop points
    const std::vector<ChopRegion>& getRegions() const { return regions_; }

    // Get raw chop point times
    const std::vector<double>& getChopPoints() const { return chopPoints_; }

    // Rebuild regions from chop points (call after any modification)
    void rebuildRegions();

    // Apply micro-fades to prevent clicks at region boundaries
    void applyMicroFades(juce::AudioBuffer<float>& dest, int regionIndex,
                         float fadeInMs, float fadeOutMs) const;

    // Callback fired when regions change
    std::function<void()> onRegionsChanged;

private:
    const juce::AudioBuffer<float>* sourceBuffer_ = nullptr;
    double sampleRate_ = 44100.0;
    double totalDuration_ = 0.0;

    std::vector<double>      chopPoints_;   // sorted list of cut times
    std::vector<ChopRegion>  regions_;

    int nextId_ = 1;
    static const juce::Colour regionColours_[];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChopEngine)
};
