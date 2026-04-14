#pragma once
#include <JuceHeader.h>
#include "NoiseGate.h"

// ─────────────────────────────────────────────────────────────────────────────
// Waveform + gate overlay viewer
// Shows the audio waveform with coloured overlays for dead vs live regions,
// and a draggable threshold line
// ─────────────────────────────────────────────────────────────────────────────
class GateViewer : public juce::Component
{
public:
    GateViewer();

    void setBuffer(const juce::AudioBuffer<float>* buf, double sampleRate);
    void setScanResult(const CleanScanResult& result);
    void setThresholdDb(float db);

    float getThresholdDb() const { return thresholdDb_; }

    std::function<void(float /*newThresholdDb*/)> onThresholdDragged;

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp  (const juce::MouseEvent& e) override;

private:
    const juce::AudioBuffer<float>* buffer_     = nullptr;
    double    sampleRate_    = 44100.0;
    float     thresholdDb_   = -40.f;
    bool      draggingThresh_= false;

    CleanScanResult scanResult_;
    bool hasScan_ = false;

    void drawWaveform  (juce::Graphics& g, juce::Rectangle<float> b);
    void drawRegions   (juce::Graphics& g, juce::Rectangle<float> b);
    void drawThreshLine(juce::Graphics& g, juce::Rectangle<float> b);
    void drawLegend    (juce::Graphics& g, juce::Rectangle<float> b);
    void drawNoData    (juce::Graphics& g);

    // Map dB to a Y position within bounds (top = 0dB, bottom = -96dB)
    float dbToY(float db, juce::Rectangle<float> b) const;
    float yToDb(float y,  juce::Rectangle<float> b) const;

    // Map sample position to x
    float sampleToX(int sample, juce::Rectangle<float> b) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GateViewer)
};

// ─────────────────────────────────────────────────────────────────────────────
// Full Clean tab component
// ─────────────────────────────────────────────────────────────────────────────
class CleanTab : public juce::Component, public juce::Thread
{
public:
    CleanTab(const juce::AudioBuffer<float>** bufferPtr, double* sampleRatePtr);
    ~CleanTab() override;

    // Called by the editor when a new file is loaded
    void onFileLoaded();

    // The processed (gated) buffer — read by the processor for playback
    const juce::AudioBuffer<float>* getCleanBuffer() const;
    bool hasCleanBuffer() const { return cleanBuffer_ != nullptr; }

    void paint(juce::Graphics& g) override;
    void resized() override;

    // juce::Thread
    void run() override;

    std::function<void()> onCleanReady;   // fires when processing completes

private:
    const juce::AudioBuffer<float>** sourceBufferPtr_;
    double* sampleRatePtr_;

    NoiseGateEngine gateEngine_;
    CleanScanResult lastScan_;

    std::unique_ptr<juce::AudioBuffer<float>> cleanBuffer_;
    std::unique_ptr<juce::AudioBuffer<float>> pendingClean_;

    GateViewer   viewer_;

    // Controls
    juce::Slider  thresholdSlider_;
    juce::Slider  minDeadSlider_;
    juce::Slider  holdSlider_;
    juce::Label   threshLabel_   {{}, "Gate threshold"};
    juce::Label   minDeadLabel_  {{}, "Min silence (ms)"};
    juce::Label   holdLabel_     {{}, "Hold (ms)"};
    juce::TextButton scanBtn_    { "Scan for noise" };
    juce::TextButton applyBtn_   { "Apply clean" };
    juce::TextButton resetBtn_   { "Reset to original" };
    juce::TextButton exportBtn_  { "Save clean file..." };
    juce::ToggleButton autoBtn_  { "Auto-scan on load" };

    // Stats panel
    juce::Label statsLabel_;

    // Progress
    juce::ProgressBar progressBar_;
    double progress_ = 0.0;
    bool   scanning_ = false;

    // Suggested threshold label
    juce::Label suggestLabel_;

    void doScan();
    void doApply();
    void doReset();
    void updateStats();
    void updateSuggestedThreshold();

    // Safely update UI from background thread
    void notifyScanComplete();

    std::unique_ptr<juce::FileChooser> fileChooser_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CleanTab)
};
