#include "ChopEngine.h"

const juce::Colour ChopEngine::regionColours_[] = {
    juce::Colour(0xffAFA9EC), juce::Colour(0xff5DCAA5), juce::Colour(0xffEF9F27),
    juce::Colour(0xffD4537E), juce::Colour(0xff85B7EB), juce::Colour(0xff97C459),
    juce::Colour(0xffF0997B), juce::Colour(0xff7F77DD)
};

ChopEngine::ChopEngine() {}

void ChopEngine::setAudioBuffer(const juce::AudioBuffer<float>* buffer, double sampleRate)
{
    sourceBuffer_ = buffer;
    sampleRate_   = sampleRate;
    totalDuration_ = buffer ? (double)buffer->getNumSamples() / sampleRate : 0.0;
    clearAll();
}

void ChopEngine::addChopPoint(double timeSec)
{
    if (timeSec <= 0.0 || timeSec >= totalDuration_) return;

    // Insert sorted
    auto it = std::lower_bound(chopPoints_.begin(), chopPoints_.end(), timeSec);
    // Skip duplicates within 5ms
    if (it != chopPoints_.end() && std::abs(*it - timeSec) < 0.005) return;
    chopPoints_.insert(it, timeSec);

    rebuildRegions();
    if (onRegionsChanged) onRegionsChanged();
}

void ChopEngine::removeChopPointNear(double timeSec, double toleranceSec)
{
    auto it = std::min_element(chopPoints_.begin(), chopPoints_.end(),
        [&](double a, double b) {
            return std::abs(a - timeSec) < std::abs(b - timeSec);
        });

    if (it != chopPoints_.end() && std::abs(*it - timeSec) <= toleranceSec)
    {
        chopPoints_.erase(it);
        rebuildRegions();
        if (onRegionsChanged) onRegionsChanged();
    }
}

void ChopEngine::detectTransients(float sensitivityDb)
{
    if (!sourceBuffer_) return;

    chopPoints_.clear();

    const int   numSamples  = sourceBuffer_->getNumSamples();
    const int   numChannels = sourceBuffer_->getNumChannels();
    const int   windowSize  = (int)(sampleRate_ * 0.01);   // 10ms windows
    const float threshold   = juce::Decibels::decibelsToGain(sensitivityDb);
    const int   minGapSamples = (int)(sampleRate_ * 0.08); // min 80ms between chops

    float prevEnergy = 0.f;
    int   lastChopSample = -minGapSamples;

    for (int i = 0; i + windowSize < numSamples; i += windowSize)
    {
        float energy = 0.f;
        for (int ch = 0; ch < numChannels; ++ch)
        {
            const float* data = sourceBuffer_->getReadPointer(ch, i);
            for (int s = 0; s < windowSize; ++s)
                energy += data[s] * data[s];
        }
        energy /= (float)(windowSize * numChannels);
        energy  = std::sqrt(energy);

        float delta = energy - prevEnergy;
        if (delta > threshold && (i - lastChopSample) > minGapSamples)
        {
            double t = (double)i / sampleRate_;
            if (t > 0.02) // skip first 20ms
            {
                chopPoints_.push_back(t);
                lastChopSample = i;
            }
        }
        prevEnergy = energy;
    }

    rebuildRegions();
    if (onRegionsChanged) onRegionsChanged();
}

void ChopEngine::divideEvenly(int numSlices)
{
    if (totalDuration_ <= 0.0 || numSlices < 2) return;

    chopPoints_.clear();
    double step = totalDuration_ / (double)numSlices;
    for (int i = 1; i < numSlices; ++i)
        chopPoints_.push_back(step * i);

    rebuildRegions();
    if (onRegionsChanged) onRegionsChanged();
}

void ChopEngine::clearAll()
{
    chopPoints_.clear();
    regions_.clear();
    nextId_ = 1;
    if (onRegionsChanged) onRegionsChanged();
}

void ChopEngine::rebuildRegions()
{
    regions_.clear();
    nextId_ = 1;

    std::vector<double> boundaries;
    boundaries.push_back(0.0);
    for (double t : chopPoints_) boundaries.push_back(t);
    boundaries.push_back(totalDuration_ > 0.0 ? totalDuration_ : 4.0);

    for (size_t i = 0; i + 1 < boundaries.size(); ++i)
    {
        ChopRegion r;
        r.id       = nextId_++;
        r.startSec = boundaries[i];
        r.endSec   = boundaries[i + 1];
        r.colour   = regionColours_[i % 8];
        regions_.push_back(r);
    }
}

void ChopEngine::applyMicroFades(juce::AudioBuffer<float>& dest, int regionIndex,
                                  float fadeInMs, float fadeOutMs) const
{
    if (regionIndex < 0 || regionIndex >= (int)regions_.size()) return;

    const int numSamples    = dest.getNumSamples();
    const int fadeInSamples  = (int)(sampleRate_ * fadeInMs  / 1000.0);
    const int fadeOutSamples = (int)(sampleRate_ * fadeOutMs / 1000.0);

    for (int ch = 0; ch < dest.getNumChannels(); ++ch)
    {
        float* data = dest.getWritePointer(ch);

        // Fade in
        for (int s = 0; s < std::min(fadeInSamples, numSamples); ++s)
            data[s] *= (float)s / (float)fadeInSamples;

        // Fade out
        for (int s = 0; s < std::min(fadeOutSamples, numSamples); ++s)
        {
            int idx = numSamples - 1 - s;
            if (idx >= 0)
                data[idx] *= (float)s / (float)fadeOutSamples;
        }
    }
}
