#include "SpectrumAnalyzer.h"

// ─────────────────────────────────────────────────────────────────────────────
// SpectrumAnalyzer
// ─────────────────────────────────────────────────────────────────────────────

SpectrumAnalyzer::SpectrumAnalyzer()
    : fft_(kFftOrder),
      window_(kFftSize, juce::dsp::WindowingFunction<float>::hann)
{
    smoothedDb_.fill(-90.f);
    rawDb_.fill(-90.f);
    fftData_.fill(0.f);
    fifo_.fill(0.f);

    startTimerHz(30); // poll fifo 30x per second
}

SpectrumAnalyzer::~SpectrumAnalyzer()
{
    stopTimer();
}

void SpectrumAnalyzer::pushSamples(const float* data, int numSamples)
{
    for (int i = 0; i < numSamples; ++i)
    {
        fifo_[(size_t)fifoIndex_] = data[i];
        ++fifoIndex_;
        if (fifoIndex_ == kFftSize)
        {
            fifoIndex_ = 0;
            fifoFull_  = true;
        }
    }
}

void SpectrumAnalyzer::timerCallback()
{
    if (!fifoFull_) return;
    fifoFull_ = false;

    // Copy fifo into fftData (reorder ring buffer)
    for (int i = 0; i < kFftSize; ++i)
        fftData_[(size_t)i] = fifo_[(size_t)((fifoIndex_ + i) % kFftSize)];

    processFFT();

    if (onNewSpectrum)
        juce::MessageManager::callAsync(onNewSpectrum);

    newDataReady_.store(true);
}

void SpectrumAnalyzer::processFFT()
{
    // Zero out the second half (complex output storage)
    for (int i = kFftSize; i < kFftSize * 2; ++i)
        fftData_[(size_t)i] = 0.f;

    // Apply Hann window
    window_.multiplyWithWindowingTable(fftData_.data(), kFftSize);

    // In-place FFT
    fft_.performFrequencyOnlyForwardTransform(fftData_.data());

    // Convert to dBFS and smooth
    const float normFactor = 1.f / (float)kFftSize;
    for (int bin = 0; bin < kNumBins; ++bin)
    {
        float mag = fftData_[(size_t)bin] * normFactor;
        float db  = mag > 0.f ? juce::Decibels::gainToDecibels(mag) : -90.f;
        db = juce::jlimit(-90.f, 0.f, db);

        rawDb_[(size_t)bin]      = db;
        smoothedDb_[(size_t)bin] = smoothedDb_[(size_t)bin] * smoothing_
                                 + db * (1.f - smoothing_);
    }
}

// Static: analyze a region offline
std::array<float, SpectrumAnalyzer::kNumBins>
SpectrumAnalyzer::analyzeRegion(const juce::AudioBuffer<float>& buf,
                                  double sampleRate,
                                  double startSec, double endSec)
{
    std::array<float, kNumBins> result;
    result.fill(-90.f);

    int startSample = (int)(startSec * sampleRate);
    int endSample   = (int)(endSec   * sampleRate);
    startSample = juce::jlimit(0, buf.getNumSamples(), startSample);
    endSample   = juce::jlimit(startSample, buf.getNumSamples(), endSample);

    if (endSample - startSample < kFftSize) return result;

    // Mix to mono and take a window from the middle of the region
    int midSample = startSample + (endSample - startSample) / 2 - kFftSize / 2;
    midSample = juce::jlimit(0, buf.getNumSamples() - kFftSize, midSample);

    std::array<float, kFftSize * 2> data{};
    for (int i = 0; i < kFftSize; ++i)
    {
        float s = 0.f;
        for (int ch = 0; ch < buf.getNumChannels(); ++ch)
            s += buf.getSample(ch, midSample + i);
        data[(size_t)i] = s / (float)buf.getNumChannels();
    }

    // Apply Hann window
    juce::dsp::WindowingFunction<float> win(kFftSize,
        juce::dsp::WindowingFunction<float>::hann);
    win.multiplyWithWindowingTable(data.data(), kFftSize);

    juce::dsp::FFT fft(kFftOrder);
    fft.performFrequencyOnlyForwardTransform(data.data());

    const float norm = 1.f / (float)kFftSize;
    for (int bin = 0; bin < kNumBins; ++bin)
    {
        float mag = data[(size_t)bin] * norm;
        result[(size_t)bin] = mag > 0.f
            ? juce::jlimit(-90.f, 0.f, juce::Decibels::gainToDecibels(mag))
            : -90.f;
    }

    return result;
}

float SpectrumAnalyzer::findDominantHz(const std::array<float, kNumBins>& mags,
                                         double sampleRate)
{
    int bestBin = 1; // skip DC
    float bestMag = -90.f;
    // Search within vocal range ~80–1200 Hz
    int minBin = (int)(80.0  * kFftSize / sampleRate);
    int maxBin = (int)(1200.0 * kFftSize / sampleRate);
    minBin = juce::jlimit(1, kNumBins - 1, minBin);
    maxBin = juce::jlimit(1, kNumBins - 1, maxBin);

    for (int b = minBin; b <= maxBin; ++b)
    {
        if (mags[(size_t)b] > bestMag)
        {
            bestMag = mags[(size_t)b];
            bestBin = b;
        }
    }
    return (float)bestBin * (float)sampleRate / (float)kFftSize;
}

// ─────────────────────────────────────────────────────────────────────────────
// SpectrumOverlay
// ─────────────────────────────────────────────────────────────────────────────

SpectrumOverlay::SpectrumOverlay(SpectrumAnalyzer& a) : analyzer_(a)
{
    setOpaque(false);
    setInterceptsMouseClicks(false, false);
}

void SpectrumOverlay::paint(juce::Graphics& g)
{
    if (!enabled_) return;

    const auto& mags = analyzer_.getMagnitudesDb();
    auto b = getLocalBounds().toFloat();

    const int    W       = (int)b.getWidth();
    const float  H       = b.getHeight();
    const int    numBins = SpectrumAnalyzer::kNumBins;

    // We display bins on a log-frequency scale for a natural look
    // Map pixel x → frequency → bin
    const float minLog = std::log10(20.f);
    const float maxLog = std::log10(20000.f);

    juce::Path specPath;
    bool started = false;

    for (int px = 0; px < W; ++px)
    {
        float t    = (float)px / (float)W;
        float freq = std::pow(10.f, minLog + t * (maxLog - minLog));
        int   bin  = (int)(freq * (float)SpectrumAnalyzer::kFftSize / 44100.f);
        bin = juce::jlimit(0, numBins - 1, bin);

        float norm = dbToNorm(mags[(size_t)bin]);
        float y    = b.getBottom() - norm * H;

        if (!started) { specPath.startNewSubPath((float)px, y); started = true; }
        else           specPath.lineTo((float)px, y);
    }

    if (style_ == Style::Filled || style_ == Style::Line)
    {
        // Filled version: close the path at the bottom
        if (style_ == Style::Filled)
        {
            juce::Path fill = specPath;
            fill.lineTo(b.getRight(), b.getBottom());
            fill.lineTo(b.getX(),     b.getBottom());
            fill.closeSubPath();
            g.setColour(juce::Colour(0xff5DCAA5).withAlpha(0.18f));
            g.fillPath(fill);
        }

        // Stroke the line
        g.setColour(juce::Colour(0xff5DCAA5).withAlpha(0.55f));
        g.strokePath(specPath, juce::PathStrokeType(1.2f));
    }
    else
    {
        // Bar style
        for (int px = 0; px < W; px += 2)
        {
            float t    = (float)px / (float)W;
            float freq = std::pow(10.f, minLog + t * (maxLog - minLog));
            int   bin  = (int)(freq * (float)SpectrumAnalyzer::kFftSize / 44100.f);
            bin = juce::jlimit(0, numBins - 1, bin);

            float norm = dbToNorm(mags[(size_t)bin]);
            float barH = norm * H;
            g.setColour(juce::Colour::fromHSV(0.45f + norm * 0.1f, 0.7f, 0.8f, 0.5f));
            g.fillRect((float)px, b.getBottom() - barH, 1.5f, barH);
        }
    }

    // Frequency labels
    g.setFont(9.f);
    g.setColour(juce::Colour(0xff5DCAA5).withAlpha(0.4f));
    static const float kFreqLabels[] = { 100.f, 500.f, 1000.f, 5000.f, 10000.f };
    for (float freq : kFreqLabels)
    {
        float t  = (std::log10(freq) - minLog) / (maxLog - minLog);
        float x  = t * b.getWidth();
        juce::String lbl = freq >= 1000.f
            ? juce::String(freq / 1000.f, 0) + "k"
            : juce::String((int)freq);
        g.drawText(lbl, (int)x - 10, (int)b.getBottom() - 14, 22, 12,
                   juce::Justification::centred);
        g.drawLine(x, b.getBottom() - 4.f, x, b.getBottom(), 0.5f);
    }
}
