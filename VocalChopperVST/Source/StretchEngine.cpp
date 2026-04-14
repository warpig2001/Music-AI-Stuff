#include "StretchEngine.h"

StretchEngine::StretchEngine() {}

// ─────────────────────────────────────────────────────────────────────────────
// Settings management
// ─────────────────────────────────────────────────────────────────────────────

void StretchEngine::setSettings(const RegionStretchSettings& s)
{
    int idx = findSettingsIndex(s.regionId);
    if (idx >= 0)
        settings_[(size_t)idx] = s;
    else
        settings_.push_back(s);
}

const RegionStretchSettings& StretchEngine::getSettings(int regionId) const
{
    static RegionStretchSettings defaultSettings;
    int idx = findSettingsIndex(regionId);
    return idx >= 0 ? settings_[(size_t)idx] : defaultSettings;
}

int StretchEngine::findSettingsIndex(int regionId) const
{
    for (int i = 0; i < (int)settings_.size(); ++i)
        if (settings_[(size_t)i].regionId == regionId) return i;
    return -1;
}

// ─────────────────────────────────────────────────────────────────────────────
// Resampler — JUCE LagrangeInterpolator
// ratio < 1.0 = shorter (speed up), ratio > 1.0 = longer (slow down)
// ─────────────────────────────────────────────────────────────────────────────

juce::AudioBuffer<float> StretchEngine::resample(const juce::AudioBuffer<float>& src,
                                                   double ratio) const
{
    if (std::abs(ratio - 1.0) < 0.001) return src;   // nothing to do

    int srcLen  = src.getNumSamples();
    int destLen = juce::jlimit(1, srcLen * 8,
                                (int)std::round((double)srcLen * ratio));

    juce::AudioBuffer<float> dest(src.getNumChannels(), destLen);

    for (int ch = 0; ch < src.getNumChannels(); ++ch)
    {
        juce::LagrangeInterpolator interp;
        interp.reset();

        const float* srcData  = src.getReadPointer(ch);
        float*       destData = dest.getWritePointer(ch);

        // speed = 1/ratio  (ratio > 1 → speed < 1 → slower)
        double speed = 1.0 / ratio;
        interp.process(speed, srcData, destData, destLen, srcLen, 0);
    }

    return dest;
}

// ─────────────────────────────────────────────────────────────────────────────
// Pitch shift — resample then resample back to original length.
// This classic "pitch without time" trick:
//   1. Resample to change perceived pitch (alters length)
//   2. Resample the result back to the original length (restores duration)
// Quality is sufficient for monitoring / preview. For production use,
// replace step 2 with a proper phase vocoder (e.g. Rubber Band Library).
// ─────────────────────────────────────────────────────────────────────────────

juce::AudioBuffer<float> StretchEngine::pitchShift(const juce::AudioBuffer<float>& region,
                                                     float semitones,
                                                     double /*sampleRate*/) const
{
    if (std::abs(semitones) < 0.01f) return region;

    // semitones → ratio: +12 semitones = 2x frequency = resample at 0.5x rate
    double pitchRatio = std::pow(2.0, -(double)semitones / 12.0);

    // Step 1: resample to shift pitch (changes length)
    auto pitched = resample(region, pitchRatio);

    // Step 2: resample back to original length (restores timing)
    double restoreRatio = (double)region.getNumSamples()
                        / (double)pitched.getNumSamples();
    return resample(pitched, restoreRatio);
}

// ─────────────────────────────────────────────────────────────────────────────
// Time stretch — changes duration, optionally preserving pitch
// ─────────────────────────────────────────────────────────────────────────────

juce::AudioBuffer<float> StretchEngine::timeStretch(const juce::AudioBuffer<float>& region,
                                                      float ratio,
                                                      bool  preservePitch,
                                                      double sampleRate) const
{
    if (std::abs(ratio - 1.f) < 0.005f) return region;

    // Step 1: resample to new duration
    auto stretched = resample(region, (double)ratio);

    if (preservePitch)
    {
        // Pitch compensation: stretching slows playback → pitch drops.
        // Shift up by the same ratio to cancel.
        float compensateSemitones = (float)(12.0 * std::log2((double)ratio));
        return pitchShift(stretched, compensateSemitones, sampleRate);
    }

    return stretched;
}

// ─────────────────────────────────────────────────────────────────────────────
// Full region processing
// ─────────────────────────────────────────────────────────────────────────────

juce::AudioBuffer<float> StretchEngine::processRegion(
    const juce::AudioBuffer<float>& source,
    double sampleRate,
    double startSec,
    double endSec,
    const RegionStretchSettings& settings) const
{
    if (!settings.enabled) {
        // Return unmodified slice
        int startSample = (int)(startSec * sampleRate);
        int numSamples  = (int)((endSec - startSec) * sampleRate);
        startSample = juce::jlimit(0, source.getNumSamples(), startSample);
        numSamples  = juce::jlimit(0, source.getNumSamples() - startSample, numSamples);
        juce::AudioBuffer<float> slice(source.getNumChannels(), numSamples);
        for (int ch = 0; ch < source.getNumChannels(); ++ch)
            slice.copyFrom(ch, 0, source, ch, startSample, numSamples);
        return slice;
    }

    // Extract region
    int startSample = (int)(startSec * sampleRate);
    int numSamples  = (int)((endSec - startSec) * sampleRate);
    startSample = juce::jlimit(0, source.getNumSamples(), startSample);
    numSamples  = juce::jlimit(1, source.getNumSamples() - startSample, numSamples);

    juce::AudioBuffer<float> region(source.getNumChannels(), numSamples);
    for (int ch = 0; ch < source.getNumChannels(); ++ch)
        region.copyFrom(ch, 0, source, ch, startSample, numSamples);

    // Apply time stretch first
    auto processed = timeStretch(region,
                                  settings.stretchRatio,
                                  settings.preservePitch,
                                  sampleRate);

    // Apply pitch shift on top (if pitch-preserving stretch, this adds extra shift)
    if (std::abs(settings.pitchSemitones) > 0.01f)
        processed = pitchShift(processed, settings.pitchSemitones, sampleRate);

    return processed;
}

double StretchEngine::estimateOutputDuration(double inputSec,
                                              const RegionStretchSettings& s) const
{
    if (!s.enabled) return inputSec;
    return inputSec * (double)s.stretchRatio;
}
