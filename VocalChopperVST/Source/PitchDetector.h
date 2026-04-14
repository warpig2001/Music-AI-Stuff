#pragma once
#include <JuceHeader.h>
#include <vector>
#include <functional>

struct PitchResult
{
    float  frequencyHz  = 0.f;
    float  midiNote     = 0.f;
    float  confidence   = 0.f;   // 0..1
    bool   voiced       = false;
    juce::String noteName;       // e.g. "A#4"

    static juce::String midiToNoteName(float midi)
    {
        if (midi < 0) return "-";
        static const char* names[] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
        int n = (int)std::round(midi);
        return juce::String(names[n % 12]) + juce::String(n / 12 - 1);
    }
};

// YIN pitch detector — accurate for monophonic vocals
// Reference: de Cheveigné & Kawahara (2002)
class PitchDetector
{
public:
    explicit PitchDetector(int bufferSize = 2048);

    void setSampleRate(double sr) { sampleRate_ = sr; }
    void setConfidenceThreshold(float t) { confidenceThreshold_ = t; }

    // Analyze a mono buffer and return pitch estimate
    PitchResult analyze(const float* samples, int numSamples);

    // Sliding window: call this every audio block, get pitch continuously
    PitchResult processSamples(const juce::AudioBuffer<float>& buf);

    // Callback fired when pitch changes significantly (>0.5 semitone)
    std::function<void(const PitchResult&)> onPitchChanged;

private:
    double sampleRate_          = 44100.0;
    float  confidenceThreshold_ = 0.15f;
    int    bufferSize_;

    std::vector<float> yinBuffer_;
    std::vector<float> slidingWindow_;
    int                windowWritePos_ = 0;

    PitchResult lastResult_;

    // YIN steps
    void   differenceFunction(const float* x, int N, std::vector<float>& d);
    void   cumulativeMeanNormDiff(std::vector<float>& d);
    int    absoluteThreshold(const std::vector<float>& d, float threshold);
    float  parabolicInterpolation(const std::vector<float>& d, int tau);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PitchDetector)
};

// Stores per-region pitch analysis
struct RegionPitchInfo
{
    int   regionId      = 0;
    float dominantHz    = 0.f;
    float dominantMidi  = 0.f;
    float averageHz     = 0.f;
    juce::String rootNote;
    std::vector<PitchResult> frameResults;
};

// Analyzes pitch across all chop regions in a buffer
class RegionPitchAnalyzer
{
public:
    RegionPitchAnalyzer();
    void setSampleRate(double sr) { detector_.setSampleRate(sr); }

    // Analyze pitch in a specific time range within a buffer
    RegionPitchInfo analyzeRegion(
        const juce::AudioBuffer<float>& buf,
        double sampleRate,
        double startSec,
        double endSec,
        int    regionId);

    std::vector<RegionPitchInfo> analyzeAllRegions(
        const juce::AudioBuffer<float>& buf,
        double sampleRate,
        const std::vector<struct ChopRegion>& regions);

    // Access the internal detector for live real-time pitch tracking
    PitchDetector& getSingleDetector() { return detector_; }

private:
    PitchDetector detector_ { 2048 };   // default 2048-sample window
    static constexpr int kHopSize = 512;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RegionPitchAnalyzer)
};
