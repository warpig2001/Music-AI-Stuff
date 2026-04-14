#include "WaveformDisplay.h"

WaveformDisplay::WaveformDisplay(juce::AudioFormatManager& fmt, ChopEngine& chop)
    : formatManager_(fmt), chopEngine_(chop), thumbnailCache_(5)
{
    thumbnail_ = std::make_unique<juce::AudioThumbnail>(512, fmt, thumbnailCache_);
    thumbnail_->addChangeListener(this);

    setOpaque(true);
}

WaveformDisplay::~WaveformDisplay()
{
    thumbnail_->removeChangeListener(this);
}

void WaveformDisplay::loadFile(const juce::File& file)
{
    thumbnail_->setSource(new juce::FileInputSource(file));

    // Also decode into a buffer for analysis
    std::unique_ptr<juce::AudioFormatReader> reader(formatManager_.createReaderFor(file));
    if (reader)
    {
        sampleRate_ = reader->sampleRate;
        audioBuffer_ = std::make_unique<juce::AudioBuffer<float>>(
            (int)reader->numChannels,
            (int)reader->lengthInSamples);
        reader->read(audioBuffer_.get(), 0, (int)reader->lengthInSamples, 0, true, true);
        fileLoaded_ = true;
        repaint();
    }
}

void WaveformDisplay::loadBuffer(const juce::AudioBuffer<float>& buf, double sr)
{
    sampleRate_ = sr;
    audioBuffer_ = std::make_unique<juce::AudioBuffer<float>>(buf);
    fileLoaded_  = audioBuffer_->getNumSamples() > 0;
    repaint();
}

void WaveformDisplay::setPlayheadPosition(double pos)
{
    playheadPos_ = pos;
    repaint();
}

void WaveformDisplay::setZoom(float pps)
{
    pixelsPerSec_ = juce::jlimit(20.f, 1000.f, pps);
    repaint();
}

void WaveformDisplay::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Background
    g.fillAll(juce::Colour(0xff0d0d1a));

    if (!fileLoaded_)
    {
        drawDropHint(g);
        return;
    }

    // Ruler
    float rulerH = 18.f;
    auto rulerBounds = bounds.removeFromTop(rulerH);
    drawRuler(g, rulerBounds);

    // Waveform
    if (thumbnail_->getTotalLength() > 0.0)
    {
        g.setColour(juce::Colour(0xff7F77DD).withAlpha(0.85f));
        thumbnail_->drawChannels(g, bounds.toNearestInt(),
                                  viewStart_,
                                  viewStart_ + (double)getWidth() / pixelsPerSec_,
                                  1.0f);
    }
    else
    {
        // Fallback: draw from buffer directly
        const int numSamples = audioBuffer_->getNumSamples();
        const int W = (int)bounds.getWidth();
        const float midY = bounds.getCentreY();
        const float halfH = bounds.getHeight() * 0.45f;

        g.setColour(juce::Colour(0xff7F77DD).withAlpha(0.8f));
        juce::Path wave;
        for (int px = 0; px < W; ++px)
        {
            int startSample = (int)((double)(px + viewStart_ * pixelsPerSec_) / pixelsPerSec_ * sampleRate_);
            int endSample   = (int)((double)(px + 1 + viewStart_ * pixelsPerSec_) / pixelsPerSec_ * sampleRate_);
            startSample = juce::jlimit(0, numSamples - 1, startSample);
            endSample   = juce::jlimit(startSample + 1, numSamples, endSample);

            float peak = 0.f;
            for (int ch = 0; ch < audioBuffer_->getNumChannels(); ++ch)
                for (int s = startSample; s < endSample; ++s)
                    peak = std::max(peak, std::abs(audioBuffer_->getSample(ch, s)));

            float top = midY - peak * halfH;
            float bot = midY + peak * halfH;
            if (px == 0) wave.startNewSubPath((float)px, top);
            wave.lineTo((float)px, top);
            wave.lineTo((float)px, bot);
        }
        g.strokePath(wave, juce::PathStrokeType(1.f));
    }

    // Chop region overlays
    drawChopMarkers(g);
    drawPlayhead(g);
}

void WaveformDisplay::drawRuler(juce::Graphics& g, juce::Rectangle<float> r)
{
    g.setColour(juce::Colour(0xff1a1a2e));
    g.fillRect(r);

    g.setColour(juce::Colour(0xff4a4a7a));
    g.setFont(10.f);

    double endTime = viewStart_ + (double)getWidth() / pixelsPerSec_;
    double step = 0.5;
    if (pixelsPerSec_ < 40)  step = 2.0;
    if (pixelsPerSec_ > 200) step = 0.1;

    for (double t = std::ceil(viewStart_ / step) * step; t < endTime; t += step)
    {
        float x = timeToX(t);
        bool isBig = std::fmod(t, step * 4) < 0.001;
        g.drawLine(x, r.getBottom() - (isBig ? 8.f : 4.f), x, r.getBottom(), 0.5f);
        if (isBig)
            g.drawText(juce::String(t, 1) + "s", (int)x + 2, (int)r.getY(), 40, 14,
                       juce::Justification::left);
    }
}

void WaveformDisplay::drawChopMarkers(juce::Graphics& g)
{
    const auto& regions = chopEngine_.getRegions();
    const float h = (float)getHeight();

    for (size_t i = 0; i < regions.size(); ++i)
    {
        const auto& r = regions[i];
        float x1 = timeToX(r.startSec);
        float x2 = timeToX(r.endSec);

        // Region tint
        g.setColour(r.colour.withAlpha(0.12f));
        g.fillRect(x1, 18.f, x2 - x1, h - 18.f);

        // Chop line (except at 0)
        if (r.startSec > 0.001)
        {
            g.setColour(r.colour.withAlpha(0.9f));
            g.drawLine(x1, 18.f, x1, h, 1.5f);

            // Handle triangle
            juce::Path tri;
            tri.addTriangle(x1 - 5.f, 18.f, x1 + 5.f, 18.f, x1, 26.f);
            g.fillPath(tri);
        }

        // Label
        g.setColour(r.colour.withAlpha(0.7f));
        g.setFont(10.f);
        g.drawText("R" + juce::String(r.id), (int)x1 + 3, 20, 20, 12,
                   juce::Justification::left);
    }
}

void WaveformDisplay::drawPlayhead(juce::Graphics& g)
{
    if (!fileLoaded_) return;
    double dur = duration();
    if (dur <= 0.0) return;

    float x = timeToX(playheadPos_ * dur);
    g.setColour(juce::Colours::white.withAlpha(0.8f));
    g.drawLine(x, 0.f, x, (float)getHeight(), 1.5f);
}

void WaveformDisplay::drawDropHint(juce::Graphics& g)
{
    g.setColour(juce::Colour(0xff3a3a6a));
    g.setFont(14.f);
    g.drawText("Drop a vocal file here or use the Load button",
               getLocalBounds(), juce::Justification::centred);

    // Dashed border
    juce::Path border;
    border.addRectangle(getLocalBounds().toFloat().reduced(10.f));
    juce::PathStrokeType stroke(1.5f);
    float dashes[] = {6.f, 4.f};
    stroke.createDashedStroke(border, border, dashes, 2);
    g.setColour(juce::Colour(0xff4a4a7a));
    g.fillPath(border);
}

void WaveformDisplay::resized() {}

void WaveformDisplay::mouseDown(const juce::MouseEvent& e)
{
    if (!fileLoaded_) return;
    double t = xToTime((float)e.x);

    if (e.mods.isRightButtonDown())
    {
        if (onRightClick) onRightClick(t);
    }
    else
    {
        if (onChopPointClicked) onChopPointClicked(t);
    }
}

void WaveformDisplay::mouseDrag(const juce::MouseEvent& e)
{
    // Horizontal drag pans the view
    if (e.mods.isMiddleButtonDown())
    {
        viewStart_ -= (double)e.getDistanceFromDragStartX() / pixelsPerSec_;
        viewStart_  = juce::jmax(0.0, viewStart_);
        repaint();
    }
}

void WaveformDisplay::mouseWheelMove(const juce::MouseEvent&,
                                      const juce::MouseWheelDetails& w)
{
    pixelsPerSec_ = juce::jlimit(20.f, 800.f,
                                  pixelsPerSec_ * (1.f + w.deltaY * 0.15f));
    repaint();
}

void WaveformDisplay::changeListenerCallback(juce::ChangeBroadcaster*)
{
    repaint();
}

double WaveformDisplay::duration() const
{
    if (thumbnail_->getTotalLength() > 0.0) return thumbnail_->getTotalLength();
    if (audioBuffer_ && sampleRate_ > 0.0)
        return (double)audioBuffer_->getNumSamples() / sampleRate_;
    return 0.0;
}

double WaveformDisplay::xToTime(float x) const
{
    return viewStart_ + (double)x / pixelsPerSec_;
}

float WaveformDisplay::timeToX(double t) const
{
    return (float)((t - viewStart_) * pixelsPerSec_);
}
