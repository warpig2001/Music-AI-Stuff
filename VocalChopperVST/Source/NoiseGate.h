#pragma once
#include <JuceHeader.h>
#include <vector>
#include <functional>

// Represents a detected "dead" region — silence or noise floor only
struct DeadRegion
{
    double startSec   = 0.0;
    double endSec     = 0.0;
    float  avgRmsDb   = -96.f;   // average RMS level in dB
    bool   isSilence  = false;   // true = digital silence, false = noise floor
    bool   markedForRemoval = true;

    double lengthSec() const { return endSec - startSec; }
    double lengthMs()  const { return (endSec - startSec) * 1000.0; }
};

// Result of a full clean scan
struct CleanScanResult
{
    std::vector<DeadRegion> deadRegions;   // regions below threshold
    std::vector<DeadRegion> liveRegions;   // regions with real audio content

    float  noiseFloorDb   = -96.f;   // estimated background noise floor
    float  peakDb         = 0.f;
    float  suggestedGateDb = -40.f;  // recommended threshold

    double totalDeadSec   = 0.0;
    double totalLiveSec   = 0.0;
    double audioFileSec   = 0.0;
    int    deadCount      = 0;

    bool valid = false;
};

class NoiseGateEngine
{
public:
    NoiseGateEngine();

    void setSampleRate(double sr)      { sampleRate_ = sr; }

    // ── Analysis ──────────────────────────────────────────────────────────

    // Full scan of an audio buffer — builds dead/live region lists
    CleanScanResult scan(const juce::AudioBuffer<float>& buf,
                         float thresholdDb    = -40.f,
                         float minDeadMs      = 80.f,   // ignore gaps shorter than this
                         float holdMs         = 50.f);  // hold open after signal drops

    // Estimate the noise floor from the quietest parts of the buffer
    float estimateNoiseFloor(const juce::AudioBuffer<float>& buf) const;

    // ── Processing ────────────────────────────────────────────────────────

    // Apply the gate: returns a new buffer with dead regions replaced by
    // silence (or optionally trimmed out entirely)
    juce::AudioBuffer<float> applyGate(const juce::AudioBuffer<float>& src,
                                        const CleanScanResult& scan,
                                        bool trimOut = false) const;

    // Apply fade-out/in around each kept/removed boundary to avoid clicks
    void applyBoundaryFades(juce::AudioBuffer<float>& buf,
                             const CleanScanResult& scan,
                             float fadeLenMs = 5.f) const;

    // Callback for progress updates during long scans
    std::function<void(float /*0..1*/)> onProgress;

private:
    double sampleRate_ = 44100.0;

    // Compute RMS of a sample range in one channel (or mixed)
    float computeRms(const juce::AudioBuffer<float>& buf,
                     int startSample, int numSamples) const;

    // Convert buffer sample index to seconds
    double sampleToSec(int s) const { return (double)s / sampleRate_; }
    int    secToSample(double t) const { return (int)(t * sampleRate_); }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NoiseGateEngine)
};
