#pragma once
#include <JuceHeader.h>
#include "ChopEngine.h"

class WaveformDisplay : public juce::Component,
                        public juce::ChangeListener
{
public:
    WaveformDisplay(juce::AudioFormatManager& formatManager,
                    ChopEngine& chopEngine);
    ~WaveformDisplay() override;

    // Load audio from file
    void loadFile(const juce::File& file);

    // Load from an existing audio buffer
    void loadBuffer(const juce::AudioBuffer<float>& buffer, double sampleRate);

    // Returns the loaded audio buffer (nullptr if none loaded)
    const juce::AudioBuffer<float>* getBuffer() const { return audioBuffer_.get(); }
    double getSampleRate() const { return sampleRate_; }

    // Playhead position 0..1
    void setPlayheadPosition(double normalisedPos);

    // Zoom: pixels per second
    void setZoom(float pixelsPerSec);

    std::function<void(double /*timeSec*/)> onChopPointClicked;
    std::function<void(double /*timeSec*/)> onRightClick;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e,
                        const juce::MouseWheelDetails& w) override;
    void changeListenerCallback(juce::ChangeBroadcaster*) override;

private:
    juce::AudioFormatManager&               formatManager_;
    ChopEngine&                             chopEngine_;
    juce::AudioThumbnailCache               thumbnailCache_;
    std::unique_ptr<juce::AudioThumbnail>   thumbnail_;
    std::unique_ptr<juce::AudioBuffer<float>> audioBuffer_;

    double sampleRate_    = 44100.0;
    double playheadPos_   = 0.0;   // normalised
    double viewStart_     = 0.0;   // seconds
    float  pixelsPerSec_  = 100.f;
    bool   fileLoaded_    = false;

    double duration() const;
    double xToTime(float x) const;
    float  timeToX(double t) const;

    void drawChopMarkers(juce::Graphics& g);
    void drawPlayhead(juce::Graphics& g);
    void drawRuler(juce::Graphics& g, juce::Rectangle<float> rulerBounds);
    void drawDropHint(juce::Graphics& g);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformDisplay)
};
