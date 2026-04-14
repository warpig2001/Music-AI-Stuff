#include "PitchDetector.h"
#include "ChopEngine.h"

// ─────────────────────────────────────────────────────────────────────────────
// PitchDetector
// ─────────────────────────────────────────────────────────────────────────────

PitchDetector::PitchDetector(int bufferSize)
    : bufferSize_(bufferSize)
{
    yinBuffer_.resize((size_t)bufferSize / 2, 0.f);
    slidingWindow_.resize((size_t)bufferSize, 0.f);
}

// Step 1: difference function d(tau) = sum( (x[j] - x[j+tau])^2 )
void PitchDetector::differenceFunction(const float* x, int N,
                                        std::vector<float>& d)
{
    int half = N / 2;
    d.assign((size_t)half, 0.f);
    for (int tau = 1; tau < half; ++tau)
    {
        float sum = 0.f;
        for (int j = 0; j < half; ++j)
        {
            float diff = x[j] - x[j + tau];
            sum += diff * diff;
        }
        d[(size_t)tau] = sum;
    }
}

// Step 2: cumulative mean normalised difference
void PitchDetector::cumulativeMeanNormDiff(std::vector<float>& d)
{
    d[0] = 1.f;
    float runningSum = 0.f;
    for (size_t tau = 1; tau < d.size(); ++tau)
    {
        runningSum += d[tau];
        if (runningSum > 0.f)
            d[tau] *= (float)tau / runningSum;
        else
            d[tau] = 1.f;
    }
}

// Step 3: absolute threshold — find first tau below threshold
int PitchDetector::absoluteThreshold(const std::vector<float>& d, float threshold)
{
    for (size_t tau = 2; tau < d.size(); ++tau)
    {
        if (d[tau] < threshold)
        {
            // Find local minimum in this dip
            while (tau + 1 < d.size() && d[tau + 1] < d[tau])
                ++tau;
            return (int)tau;
        }
    }
    // Fallback: global minimum
    return (int)(std::min_element(d.begin() + 2, d.end()) - d.begin());
}

// Step 4: parabolic interpolation around tau for sub-sample accuracy
float PitchDetector::parabolicInterpolation(const std::vector<float>& d, int tau)
{
    if (tau <= 0 || tau >= (int)d.size() - 1) return (float)tau;
    float s0 = d[(size_t)(tau - 1)];
    float s1 = d[(size_t)tau];
    float s2 = d[(size_t)(tau + 1)];
    float denom = 2.f * s1 - s2 - s0;
    if (std::abs(denom) < 1e-7f) return (float)tau;
    return (float)tau + (s2 - s0) / (2.f * denom);
}

PitchResult PitchDetector::analyze(const float* samples, int numSamples)
{
    PitchResult result;
    if (numSamples < bufferSize_) return result;

    differenceFunction(samples, bufferSize_, yinBuffer_);
    cumulativeMeanNormDiff(yinBuffer_);

    int tau = absoluteThreshold(yinBuffer_, confidenceThreshold_);
    if (tau < 2)
    {
        result.voiced = false;
        return result;
    }

    float tauF = parabolicInterpolation(yinBuffer_, tau);
    result.confidence = 1.f - yinBuffer_[(size_t)tau];
    result.voiced     = result.confidence > (1.f - confidenceThreshold_);

    if (tauF > 0.f && result.voiced)
    {
        result.frequencyHz = (float)sampleRate_ / tauF;
        // Clamp to vocal range: ~80 Hz (bass) to 1200 Hz (soprano)
        if (result.frequencyHz < 60.f || result.frequencyHz > 1400.f)
        {
            result.voiced = false;
            return result;
        }
        result.midiNote = 12.f * std::log2(result.frequencyHz / 440.f) + 69.f;
        result.noteName = PitchResult::midiToNoteName(result.midiNote);
    }

    return result;
}

PitchResult PitchDetector::processSamples(const juce::AudioBuffer<float>& buf)
{
    // Mix to mono
    const int n = buf.getNumSamples();
    std::vector<float> mono((size_t)n, 0.f);
    for (int ch = 0; ch < buf.getNumChannels(); ++ch)
    {
        const float* src = buf.getReadPointer(ch);
        for (int i = 0; i < n; ++i) mono[(size_t)i] += src[i];
    }
    float scale = 1.f / (float)buf.getNumChannels();
    for (float& s : mono) s *= scale;

    // Append to sliding window
    for (int i = 0; i < n; ++i)
    {
        slidingWindow_[(size_t)windowWritePos_] = mono[(size_t)i];
        windowWritePos_ = (windowWritePos_ + 1) % bufferSize_;
    }

    // Reorder sliding window into contiguous block
    std::vector<float> ordered(slidingWindow_.size());
    int wp = windowWritePos_;
    for (int i = 0; i < bufferSize_; ++i)
        ordered[(size_t)i] = slidingWindow_[(size_t)((wp + i) % bufferSize_)];

    PitchResult result = analyze(ordered.data(), bufferSize_);

    // Fire callback if pitch changed by more than 0.5 semitone
    if (result.voiced && std::abs(result.midiNote - lastResult_.midiNote) > 0.5f)
    {
        lastResult_ = result;
        if (onPitchChanged) onPitchChanged(result);
    }

    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// RegionPitchAnalyzer
// ─────────────────────────────────────────────────────────────────────────────

RegionPitchAnalyzer::RegionPitchAnalyzer()
{
    // detector_ is default-constructed (2048 samples) via its own constructor
    detector_.setSampleRate(44100.0);
}

RegionPitchInfo RegionPitchAnalyzer::analyzeRegion(
    const juce::AudioBuffer<float>& buf,
    double sampleRate,
    double startSec, double endSec, int regionId)
{
    RegionPitchInfo info;
    info.regionId = regionId;
    detector_.setSampleRate(sampleRate);

    int startSample = (int)(startSec * sampleRate);
    int endSample   = (int)(endSec   * sampleRate);
    startSample = juce::jlimit(0, buf.getNumSamples(), startSample);
    endSample   = juce::jlimit(startSample, buf.getNumSamples(), endSample);

    const int analysisLen = 2048;
    float sumHz = 0.f;
    int   voiced = 0;

    // Mix to mono for analysis
    std::vector<float> mono((size_t)(endSample - startSample), 0.f);
    for (int ch = 0; ch < buf.getNumChannels(); ++ch)
    {
        const float* src = buf.getReadPointer(ch, startSample);
        for (size_t i = 0; i < mono.size(); ++i)
            mono[i] += src[i];
    }
    float scale = 1.f / (float)buf.getNumChannels();
    for (float& s : mono) s *= scale;

    // Hop through region
    for (int pos = 0; pos + analysisLen <= (int)mono.size(); pos += kHopSize)
    {
        PitchResult r = detector_.analyze(mono.data() + pos, analysisLen);
        info.frameResults.push_back(r);
        if (r.voiced)
        {
            sumHz += r.frequencyHz;
            ++voiced;
        }
    }

    if (voiced > 0)
    {
        info.averageHz     = sumHz / (float)voiced;
        info.dominantHz    = info.averageHz; // simplified; could use median
        info.dominantMidi  = 12.f * std::log2(info.dominantHz / 440.f) + 69.f;
        info.rootNote      = PitchResult::midiToNoteName(info.dominantMidi);
    }

    return info;
}

std::vector<RegionPitchInfo> RegionPitchAnalyzer::analyzeAllRegions(
    const juce::AudioBuffer<float>& buf,
    double sampleRate,
    const std::vector<ChopRegion>& regions)
{
    std::vector<RegionPitchInfo> results;
    results.reserve(regions.size());
    for (const auto& r : regions)
        results.push_back(analyzeRegion(buf, sampleRate, r.startSec, r.endSec, r.id));
    return results;
}
