#include "AlignmentAnalyzer.h"

AlignmentAnalyzer::AlignmentAnalyzer() {}

void AlignmentAnalyzer::setBpm(double bpm)
{
    bpm_ = juce::jlimit(20.0, 400.0, bpm);
}

void AlignmentAnalyzer::setHostTimeInformation(
    const juce::AudioPlayHead::PositionInfo& pos)
{
    if (pos.getBpm().hasValue() && *pos.getBpm() > 0.0)
        bpm_ = *pos.getBpm();
}

int AlignmentAnalyzer::detectOnsetSample(const juce::AudioBuffer<float>& buf,
                                          double sr) const
{
    const int windowSamples = (int)(sr * 0.005); // 5ms windows
    if (windowSamples < 1) return 0;

    const int numCh = buf.getNumChannels();
    const int total = buf.getNumSamples();

    float prevRms = 0.f;
    const float kRatio = 3.0f; // onset = energy 3x greater than prev window

    for (int i = 0; i + windowSamples < total; i += windowSamples)
    {
        float rms = 0.f;
        for (int ch = 0; ch < numCh; ++ch)
        {
            const float* d = buf.getReadPointer(ch, i);
            for (int s = 0; s < windowSamples; ++s) rms += d[s] * d[s];
        }
        rms = std::sqrt(rms / (float)(windowSamples * numCh));

        if (prevRms > 0.001f && rms / prevRms > kRatio)
            return i;

        prevRms = rms;
    }
    return 0;
}

AlignmentResult AlignmentAnalyzer::analyzeOnset(
    const juce::AudioBuffer<float>& buffer,
    double sampleRate,
    double bufferStartTimeSec) const
{
    AlignmentResult result;

    int onsetSample = detectOnsetSample(buffer, sampleRate);
    double onsetTimeSec = bufferStartTimeSec + (double)onsetSample / sampleRate;

    double beatDur = beatDurationSec();
    double nearestBeat = std::round(onsetTimeSec / beatDur) * beatDur;

    result.offsetMs       = (onsetTimeSec - nearestBeat) * 1000.0;
    result.nearestBeatMs  = nearestBeat * 1000.0;
    result.snapAmountMs   = -result.offsetMs;
    result.nearestBar     = (int)(nearestBeat / barDurationSec()) + 1;
    result.nearestBeat    = (int)(std::fmod(nearestBeat, barDurationSec()) / beatDur) + 1;
    result.valid          = true;

    const double absOff = std::abs(result.offsetMs);
    const double beatMs = beatDur * 1000.0;

    if (absOff < 5.0)
        result.description = "Vocal is nearly perfectly on-beat.";
    else if (absOff < beatMs * 0.1)
        result.description = juce::String::formatted(
            "%.1f ms %s — minor timing variation, likely intentional (pocket).",
            absOff, result.offsetMs > 0 ? "late" : "early");
    else if (absOff < beatMs * 0.25)
        result.description = juce::String::formatted(
            "%.1f ms %s — noticeable drag/push. Consider nudging.",
            absOff, result.offsetMs > 0 ? "late" : "early");
    else
        result.description = juce::String::formatted(
            "%.1f ms %s — significantly off-grid. Align to bar %d, beat %d.",
            absOff, result.offsetMs > 0 ? "late" : "early",
            result.nearestBar, result.nearestBeat);

    return result;
}

double AlignmentAnalyzer::snapToGrid(double timeSec, double subdivisionBeats) const
{
    double subdivSec = beatDurationSec() * subdivisionBeats;
    return std::round(timeSec / subdivSec) * subdivSec;
}

double AlignmentAnalyzer::nudge(double timeSec, double deltaMs) const
{
    return timeSec + deltaMs / 1000.0;
}

juce::String AlignmentAnalyzer::generateFLInstructions(const AlignmentResult& result) const
{
    if (!result.valid) return "No analysis available.";

    juce::String out;
    out << "=== FL Studio Alignment Fix ===\n";
    out << "BPM: " << juce::String(bpm_, 1) << "\n";
    out << "Beat duration: " << juce::String(beatDurationSec() * 1000.0, 1) << " ms\n";
    out << "Detected offset: " << juce::String(result.offsetMs, 1) << " ms\n\n";

    if (std::abs(result.offsetMs) < 5.0)
    {
        out << "No action needed — vocal is on-beat.\n";
        return out;
    }

    out << "Fix options:\n\n";

    // Option 1 - Playlist nudge
    out << "1. Playlist nudge (easiest):\n";
    out << "   - Click the vocal clip in the Playlist\n";
    out << "   - Hold Alt and drag " << juce::String(std::abs(result.offsetMs), 0) << " ms ";
    out << (result.offsetMs > 0 ? "to the LEFT (earlier)" : "to the RIGHT (later)") << "\n";
    out << "   - Or: right-click clip > Properties > adjust position\n\n";

    // Option 2 - Edison
    out << "2. Edison (sample-accurate):\n";
    out << "   - Open vocal in Edison (drag to Edison or F9)\n";
    if (result.offsetMs > 0)
    {
        out << "   - Select the first " << juce::String(std::abs(result.offsetMs), 0) << " ms of silence/pre-roll\n";
        out << "   - Edit > Delete selection (removes early content to shift left)\n";
    }
    else
    {
        out << "   - Edit > Paste Special — add " << juce::String(std::abs(result.offsetMs), 0) << " ms of silence at start\n";
    }
    out << "\n";

    // Option 3 - Mixer delay compensation
    out << "3. Mixer track delay (non-destructive):\n";
    out << "   - Open Mixer (F9) and select the vocal track\n";
    out << "   - Scroll down to find the track delay knob\n";
    out << "   - Set to " << juce::String(-result.offsetMs, 0) << " ms\n\n";

    out << "Nearest grid point: Bar " << result.nearestBar
        << ", Beat " << result.nearestBeat << "\n";

    return out;
}
