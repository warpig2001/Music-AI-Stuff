#include "NoiseGate.h"

NoiseGateEngine::NoiseGateEngine() {}

// ─────────────────────────────────────────────────────────────────────────────
// RMS helper — mono mix of all channels over a sample range
// ─────────────────────────────────────────────────────────────────────────────
float NoiseGateEngine::computeRms(const juce::AudioBuffer<float>& buf,
                                   int startSample, int numSamples) const
{
    if (numSamples <= 0) return 0.f;

    double sum = 0.0;
    int total  = 0;
    const int numCh = buf.getNumChannels();

    for (int ch = 0; ch < numCh; ++ch)
    {
        const float* data = buf.getReadPointer(ch);
        int end = std::min(startSample + numSamples, buf.getNumSamples());
        for (int i = startSample; i < end; ++i)
        {
            double s = (double)data[i];
            sum += s * s;
            ++total;
        }
    }

    if (total == 0) return 0.f;
    return (float)std::sqrt(sum / (double)total);
}

// ─────────────────────────────────────────────────────────────────────────────
// Noise floor estimation — take the average of the quietest 10% of windows
// ─────────────────────────────────────────────────────────────────────────────
float NoiseGateEngine::estimateNoiseFloor(const juce::AudioBuffer<float>& buf) const
{
    const int windowSamples = (int)(sampleRate_ * 0.020); // 20ms windows
    if (windowSamples < 1 || buf.getNumSamples() < windowSamples) return -96.f;

    std::vector<float> rmsValues;
    rmsValues.reserve((size_t)(buf.getNumSamples() / windowSamples));

    for (int i = 0; i + windowSamples < buf.getNumSamples(); i += windowSamples)
        rmsValues.push_back(computeRms(buf, i, windowSamples));

    if (rmsValues.empty()) return -96.f;

    std::sort(rmsValues.begin(), rmsValues.end());

    // Average the quietest 15%
    int numQuiet = std::max(1, (int)(rmsValues.size() * 0.15));
    double sum = 0.0;
    for (int i = 0; i < numQuiet; ++i) sum += rmsValues[(size_t)i];
    float avgQuiet = (float)(sum / numQuiet);

    return avgQuiet > 0.f ? juce::Decibels::gainToDecibels(avgQuiet) : -96.f;
}

// ─────────────────────────────────────────────────────────────────────────────
// Main scan — detect dead vs. live regions using a sliding RMS gate
// ─────────────────────────────────────────────────────────────────────────────
CleanScanResult NoiseGateEngine::scan(const juce::AudioBuffer<float>& buf,
                                       float thresholdDb,
                                       float minDeadMs,
                                       float holdMs)
{
    CleanScanResult result;
    result.audioFileSec = (double)buf.getNumSamples() / sampleRate_;

    if (buf.getNumSamples() < 128)
        return result;

    // Estimate noise floor before scanning
    result.noiseFloorDb    = estimateNoiseFloor(buf);
    result.suggestedGateDb = result.noiseFloorDb + 12.f; // 12 dB above floor
    result.suggestedGateDb = juce::jlimit(-80.f, -10.f, result.suggestedGateDb);

    const float threshold    = juce::Decibels::decibelsToGain(thresholdDb);
    const int   windowSamples= (int)(sampleRate_ * 0.010);  // 10ms analysis window
    const int   minDeadSamples=(int)(sampleRate_ * minDeadMs / 1000.0);
    const int   holdSamples   =(int)(sampleRate_ * holdMs   / 1000.0);

    // Build per-window gate state: true = above threshold (live)
    int numWindows = buf.getNumSamples() / windowSamples;
    std::vector<bool> isLive(numWindows, false);
    float peakLinear = 0.f;

    for (int w = 0; w < numWindows; ++w)
    {
        float rms = computeRms(buf, w * windowSamples, windowSamples);
        peakLinear = std::max(peakLinear, rms);
        isLive[(size_t)w] = (rms >= threshold);

        if (onProgress) onProgress((float)w / (float)numWindows * 0.7f);
    }

    result.peakDb = peakLinear > 0.f ? juce::Decibels::gainToDecibels(peakLinear) : -96.f;

    // Apply hold — once gate opens, hold it open for holdSamples
    {
        int holdRemaining = 0;
        for (int w = 0; w < numWindows; ++w)
        {
            if (isLive[(size_t)w])
                holdRemaining = holdSamples / windowSamples;
            else if (holdRemaining > 0)
            {
                isLive[(size_t)w] = true;
                --holdRemaining;
            }
        }
    }

    // Convert window booleans into DeadRegion / live region spans
    bool prevState = isLive[0];
    int  spanStart = 0;

    auto commitSpan = [&](int startW, int endW, bool live) {
        int startSample = startW * windowSamples;
        int endSample   = std::min(endW * windowSamples, buf.getNumSamples());
        int spanSamples = endSample - startSample;
        if (spanSamples < 1) return;

        double startSec = sampleToSec(startSample);
        double endSec   = sampleToSec(endSample);
        float  avgRms   = computeRms(buf, startSample, spanSamples);
        float  avgDb    = avgRms > 0.f ? juce::Decibels::gainToDecibels(avgRms) : -96.f;

        if (!live)
        {
            if (spanSamples < minDeadSamples) return; // too short to bother

            DeadRegion dr;
            dr.startSec   = startSec;
            dr.endSec     = endSec;
            dr.avgRmsDb   = avgDb;
            dr.isSilence  = (avgDb < -90.f);
            dr.markedForRemoval = true;
            result.deadRegions.push_back(dr);
            result.totalDeadSec  += dr.lengthSec();
            result.deadCount++;
        }
        else
        {
            DeadRegion lr;
            lr.startSec   = startSec;
            lr.endSec     = endSec;
            lr.avgRmsDb   = avgDb;
            lr.isSilence  = false;
            result.liveRegions.push_back(lr);
            result.totalLiveSec += lr.lengthSec();
        }
    };

    for (int w = 1; w < numWindows; ++w)
    {
        if (isLive[(size_t)w] != prevState)
        {
            commitSpan(spanStart, w, prevState);
            spanStart = w;
            prevState = isLive[(size_t)w];
        }
        if (onProgress) onProgress(0.7f + (float)w / (float)numWindows * 0.3f);
    }
    commitSpan(spanStart, numWindows, prevState);

    result.valid = true;
    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// Apply gate — zero out dead regions in the buffer
// ─────────────────────────────────────────────────────────────────────────────
juce::AudioBuffer<float> NoiseGateEngine::applyGate(const juce::AudioBuffer<float>& src,
                                                      const CleanScanResult& scanResult,
                                                      bool /*trimOut*/) const
{
    // Copy source
    juce::AudioBuffer<float> dest(src.getNumChannels(), src.getNumSamples());
    for (int ch = 0; ch < src.getNumChannels(); ++ch)
        dest.copyFrom(ch, 0, src, ch, 0, src.getNumSamples());

    // Zero out all marked dead regions
    for (const auto& dr : scanResult.deadRegions)
    {
        if (!dr.markedForRemoval) continue;

        int startSample = secToSample(dr.startSec);
        int endSample   = secToSample(dr.endSec);
        startSample = juce::jlimit(0, dest.getNumSamples(), startSample);
        endSample   = juce::jlimit(startSample, dest.getNumSamples(), endSample);
        int numSamples = endSample - startSample;

        for (int ch = 0; ch < dest.getNumChannels(); ++ch)
            dest.clear(ch, startSample, numSamples);
    }

    return dest;
}

// ─────────────────────────────────────────────────────────────────────────────
// Boundary fades — tiny ramp at each gate open/close to avoid clicks
// ─────────────────────────────────────────────────────────────────────────────
void NoiseGateEngine::applyBoundaryFades(juce::AudioBuffer<float>& buf,
                                          const CleanScanResult& scanResult,
                                          float fadeLenMs) const
{
    const int fadeSamples = std::max(1, (int)(sampleRate_ * fadeLenMs / 1000.0));
    const int total = buf.getNumSamples();

    for (const auto& dr : scanResult.deadRegions)
    {
        if (!dr.markedForRemoval) continue;

        int startSample = juce::jlimit(0, total, secToSample(dr.startSec));
        int endSample   = juce::jlimit(0, total, secToSample(dr.endSec));

        // Fade out just before dead region starts
        int fadeOutStart = std::max(0, startSample - fadeSamples);
        for (int ch = 0; ch < buf.getNumChannels(); ++ch)
        {
            float* data = buf.getWritePointer(ch);
            for (int s = fadeOutStart; s < startSample; ++s)
            {
                float t = 1.f - (float)(s - fadeOutStart) / (float)fadeSamples;
                data[s] *= t;
            }
        }

        // Fade in just after dead region ends
        int fadeInEnd = std::min(total, endSample + fadeSamples);
        for (int ch = 0; ch < buf.getNumChannels(); ++ch)
        {
            float* data = buf.getWritePointer(ch);
            for (int s = endSample; s < fadeInEnd; ++s)
            {
                float t = (float)(s - endSample) / (float)fadeSamples;
                data[s] *= t;
            }
        }
    }
}
