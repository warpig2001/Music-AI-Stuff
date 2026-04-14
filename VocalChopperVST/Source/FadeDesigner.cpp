#include "FadeDesigner.h"

float FadeSettings::evaluate(float t, bool isFadeIn) const
{
    t = juce::jlimit(0.f, 1.f, t);
    float val = 0.f;

    switch (curve)
    {
        case FadeCurveType::Linear:
            val = t;
            break;
        case FadeCurveType::Exponential:
            val = std::pow(t, power);
            break;
        case FadeCurveType::SCurve:
            val = t * t * (3.f - 2.f * t); // smoothstep
            break;
        case FadeCurveType::Logarithmic:
            val = (t <= 0.f) ? 0.f : std::log(1.f + t * 9.f) / std::log(10.f);
            break;
    }

    return isFadeIn ? val : (1.f - val);
}

FadeDesignerComponent::FadeDesignerComponent(bool isFadeIn) : isFadeIn_(isFadeIn)
{
    setSize(200, 80);
}

void FadeDesignerComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(2.f);
    g.fillAll(juce::Colour(0xff1a1a2e));

    drawGrid(g, bounds);
    drawCurve(g, bounds);

    // Border
    g.setColour(juce::Colour(0xff3a3a5c));
    g.drawRect(bounds, 1.f);
}

void FadeDesignerComponent::drawGrid(juce::Graphics& g, juce::Rectangle<float> b)
{
    g.setColour(juce::Colour(0xff2a2a4a));
    for (int i = 1; i < 4; ++i)
    {
        float x = b.getX() + b.getWidth() * i / 4.f;
        float y = b.getY() + b.getHeight() * i / 4.f;
        g.drawLine(x, b.getY(), x, b.getBottom(), 0.5f);
        g.drawLine(b.getX(), y, b.getRight(), y, 0.5f);
    }
}

void FadeDesignerComponent::drawCurve(juce::Graphics& g, juce::Rectangle<float> b)
{
    juce::Path path;
    const int steps = 100;

    for (int i = 0; i <= steps; ++i)
    {
        float t   = (float)i / (float)steps;
        float gain = settings_.evaluate(t, isFadeIn_);
        float x   = b.getX() + t * b.getWidth();
        float y   = b.getBottom() - gain * b.getHeight();

        if (i == 0) path.startNewSubPath(x, y);
        else        path.lineTo(x, y);
    }

    // Fill under curve
    juce::Path fill = path;
    fill.lineTo(b.getRight(), b.getBottom());
    fill.lineTo(b.getX(), b.getBottom());
    fill.closeSubPath();
    g.setColour(juce::Colour(0xffAFA9EC).withAlpha(0.15f));
    g.fillPath(fill);

    // Curve line
    g.setColour(juce::Colour(0xffAFA9EC));
    g.strokePath(path, juce::PathStrokeType(1.8f));
}

void FadeDesignerComponent::mouseDown(const juce::MouseEvent& e)
{
    // Left click cycles curve type
    if (e.mods.isLeftButtonDown())
    {
        int ct = (int)settings_.curve;
        settings_.curve = (FadeCurveType)((ct + 1) % 4);
        repaint();
        if (onSettingsChanged) onSettingsChanged();
    }
}

void FadeDesignerComponent::mouseDrag(const juce::MouseEvent& e)
{
    // Dragging left/right adjusts duration
    float delta = (float)e.getDistanceFromDragStartX();
    settings_.durationMs = juce::jlimit(5.f, 5000.f,
        settings_.durationMs + delta * 2.f);
    repaint();
    if (onSettingsChanged) onSettingsChanged();
}
