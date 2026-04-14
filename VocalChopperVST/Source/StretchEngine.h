#pragma once
#include <JuceHeader.h>
#include <vector>

// Per-region stretch/pitch settings
struct RegionStretchSettings
{
    int    regionId      = 0;
    float  pitchSemitones= 0.f;   // ±24 semitones
    float  stretchRatio  = 1.f;   // 0.25x … 4.0x  (1.0 = original)
    bool   preservePitch = true;  // when stretching time, lock pitch?
    bool   enabled       = false; // false = pass-through
};

class StretchEngine
{
public:
    StretchEngine();

    void setSampleRate(double sr) { sampleRate_ = sr; }

    // Set stretch/pitch for a specific region
    void setSettings(const RegionStretchSettings& s);
    const RegionStretchSettings& getSettings(int regionId) const;
    std::vector<RegionStretchSettings>& getAllSettings() { return settings_; }

    // Process a region from the source buffer and return a new pitched/stretched buffer.
    // This is called once (offline) when settings change — not in real time.
    juce::AudioBuffer<float> processRegion(
        const juce::AudioBuffer<float>& source,
        double sampleRate,
        double startSec,
        double endSec,
        const RegionStretchSettings& settings) const;

    // Pitch shift only (no time change) — fast resample trick
    // Returns new buffer with same duration but shifted pitch
    juce::AudioBuffer<float> pitchShift(
        const juce::AudioBuffer<float>& region,
        float semitones,
        double sampleRate) const;

    // Time stretch only (preserves pitch if preservePitch=true)
    // Uses JUCE's built-in lagrange resampler
    juce::AudioBuffer<float> timeStretch(
        const juce::AudioBuffer<float>& region,
        float ratio,
        bool  preservePitch,
        double sampleRate) const;

    // Estimate the resulting duration of a region after processing
    double estimateOutputDuration(double inputSec,
                                   const RegionStretchSettings& s) const;

private:
    double sampleRate_ = 44100.0;
    std::vector<RegionStretchSettings> settings_;

    // Find settings by region ID
    int findSettingsIndex(int regionId) const;

    // Resample a buffer by a ratio using JUCE's LagrangeInterpolator
    juce::AudioBuffer<float> resample(const juce::AudioBuffer<float>& src,
                                       double ratio) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StretchEngine)
};
