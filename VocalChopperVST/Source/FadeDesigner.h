#pragma once
#include <JuceHeader.h>

enum class FadeCurveType { Linear, Exponential, SCurve, Logarithmic };

struct FadeSettings
{
    float         durationMs = 100.f;
    FadeCurveType curve      = FadeCurveType::Linear;
    float         power      = 1.5f;   // used for Exponential

    // Evaluate the fade envelope at normalized position t in [0,1]
    // Returns gain in [0,1], where 0 = silence, 1 = full
    float evaluate(float t, bool isFadeIn) const;
};

// Component that draws a draggable fade envelope curve
class FadeDesignerComponent : public juce::Component
{
public:
    FadeDesignerComponent(bool isFadeIn);

    FadeSettings& getSettings() { return settings_; }
    const FadeSettings& getSettings() const { return settings_; }

    std::function<void()> onSettingsChanged;

    void paint(juce::Graphics& g) override;
    void resized() override {}

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;

private:
    FadeSettings settings_;
    bool isFadeIn_;

    void drawCurve(juce::Graphics& g, juce::Rectangle<float> bounds);
    void drawGrid(juce::Graphics& g, juce::Rectangle<float> bounds);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FadeDesignerComponent)
};
