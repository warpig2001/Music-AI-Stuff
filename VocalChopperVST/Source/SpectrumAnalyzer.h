#pragma once
#include <JuceHeader.h>
#include <vector>
#include <array>

// Real-time spectrum analyzer using JUCE's FFT.
// Runs a sliding FFT over the vocal buffer at the current playhead position
// and exposes smoothed magnitude data for drawing.

class SpectrumAnalyzer : public juce::Timer
{
public:
    static constexpr int kFftOrder = 11;           // 2048-point FFT
    static constexpr int kFftSize  = 1 << kFftOrder;
    static constexpr int kNumBins  = kFftSize / 2;

    SpectrumAnalyzer();
    ~SpectrumAnalyzer() override;

    void setSampleRate(double sr) { sampleRate_ = sr; }

    // Feed audio data — call from the audio thread or analysis thread
    void pushSamples(const float* data, int numSamples);

    // Returns smoothed magnitude per bin in dBFS — safe to read on message thread
    // after isNewDataReady() returns true
    const std::array<float, kNumBins>& getMagnitudesDb() const { return smoothedDb_; }

    bool isNewDataReady() const { return newDataReady_.load(); }
    void clearNewDataFlag()     { newDataReady_.store(false); }

    // Smoothing (0=instant, 1=frozen). Default 0.85 = nice visual
    void setSmoothingCoeff(float c) { smoothing_ = juce::jlimit(0.f, 0.999f, c); }

    // Callback fired on message thread when new spectrum is ready
    std::function<void()> onNewSpectrum;

    // juce::Timer
    void timerCallback() override;

    // Analyzes a static region of an audio buffer offline
    // Returns magnitude array in dBFS
    static std::array<float, kNumBins> analyzeRegion(
        const juce::AudioBuffer<float>& buf,
        double sampleRate,
        double startSec,
        double endSec);

    // Convert bin index to frequency
    float binToHz(int bin) const
    {
        return (float)bin * (float)sampleRate_ / (float)kFftSize;
    }

    // Find dominant frequency in a magnitude array
    static float findDominantHz(const std::array<float, kNumBins>& mags,
                                  double sampleRate);

private:
    juce::dsp::FFT             fft_;
    juce::dsp::WindowingFunction<float> window_;

    double sampleRate_ = 44100.0;
    float  smoothing_  = 0.85f;

    // Ring buffer for incoming samples
    std::array<float, kFftSize * 2> fifo_{};
    int fifoIndex_ = 0;
    bool fifoFull_ = false;

    // Working buffer for FFT
    std::array<float, kFftSize * 2> fftData_{};

    // Output — written by pushSamples, read by UI
    std::array<float, kNumBins> smoothedDb_{};
    std::array<float, kNumBins> rawDb_{};

    std::atomic<bool> newDataReady_ { false };

    void processFFT();
    static void applyHannWindow(float* data, int size);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrumAnalyzer)
};

// Component that draws a spectrum over a waveform display
class SpectrumOverlay : public juce::Component
{
public:
    explicit SpectrumOverlay(SpectrumAnalyzer& analyzer);

    // Display style
    enum class Style { Bars, Line, Filled };
    void setStyle(Style s) { style_ = s; repaint(); }
    void setEnabled(bool e) { enabled_ = e; repaint(); }
    bool isEnabled() const { return enabled_; }

    void paint(juce::Graphics& g) override;
    void resized() override {}

private:
    SpectrumAnalyzer& analyzer_;
    Style style_   = Style::Filled;
    bool  enabled_ = true;

    // Map dB to normalised height [0..1]
    static float dbToNorm(float db)
    {
        return juce::jlimit(0.f, 1.f, (db + 90.f) / 90.f);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrumOverlay)
};
