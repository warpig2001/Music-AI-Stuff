#pragma once
#include <JuceHeader.h>

struct AlignmentResult
{
    double offsetMs       = 0.0;   // positive = vocal is late, negative = early
    double nearestBeatMs  = 0.0;
    double snapAmountMs   = 0.0;
    int    nearestBar     = 0;
    int    nearestBeat    = 0;
    bool   valid          = false;
    juce::String description;
};

class AlignmentAnalyzer
{
public:
    AlignmentAnalyzer();

    void setBpm(double bpm);
    // JUCE 7: use getPosition() optional API
    void setHostTimeInformation(const juce::AudioPlayHead::PositionInfo& pos);

    // Analyze the onset of audio in buffer against the beat grid
    AlignmentResult analyzeOnset(const juce::AudioBuffer<float>& buffer,
                                  double sampleRate,
                                  double bufferStartTimeSec) const;

    // Snap a time value to the nearest beat subdivision
    double snapToGrid(double timeSec, double subdivisionBeats = 0.25) const;

    // Nudge by ms — returns new time
    double nudge(double timeSec, double deltaMs) const;

    // Generate FL Studio instructions for correcting offset
    juce::String generateFLInstructions(const AlignmentResult& result) const;

private:
    double bpm_            = 140.0;
    double beatsPerBar_    = 4.0;

    double beatDurationSec() const { return 60.0 / bpm_; }
    double barDurationSec()  const { return beatDurationSec() * beatsPerBar_; }

    // Detect the first onset sample using an energy threshold
    int detectOnsetSample(const juce::AudioBuffer<float>& buf, double sr) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AlignmentAnalyzer)
};
