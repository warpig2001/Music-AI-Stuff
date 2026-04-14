#include "AlignmentTimeline.h"

AlignmentTimeline::AlignmentTimeline(AlignmentAnalyzer& a, ChopEngine& c)
    : analyzer_(a), chopEngine_(c)
{
    setOpaque(true);
}

void AlignmentTimeline::setBpm(double bpm)         { bpm_ = bpm; repaint(); }
void AlignmentTimeline::setVocalDuration(double d) { vocalDuration_ = d; repaint(); }

void AlignmentTimeline::setAlignmentResult(const AlignmentResult& r)
{
    lastResult_ = r;
    repaint();
}

void AlignmentTimeline::refreshRegions()
{
    const auto& regions = chopEngine_.getRegions();
    regionOffsets_.resize(regions.size(), 0.0);
    repaint();
}

void AlignmentTimeline::setZoom(double spp)
{
    secPerPixel_ = juce::jlimit(0.001, 0.1, spp);
    repaint();
}

void AlignmentTimeline::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff0d0d1a));
    drawBeatGrid(g);
    drawSnapGuides(g);
    drawRegions(g);
    drawRuler(g);
    if (lastResult_.valid) drawOffsetArrow(g);
}

void AlignmentTimeline::drawBeatGrid(juce::Graphics& g)
{
    const float H = (float)getHeight();
    double beatSec = 60.0 / bpm_;
    double barSec  = beatSec * 4.0;

    double startBeat = std::floor(viewOffsetSec_ / beatSec) * beatSec;
    double endTime   = viewOffsetSec_ + (double)getWidth() * secPerPixel_;

    for (double t = startBeat; t < endTime; t += beatSec)
    {
        float x = secToX(t);
        bool isBar = std::fmod(t + 0.0001, barSec) < beatSec * 0.05;

        g.setColour(isBar ? juce::Colour(0xff3a3a6a) : juce::Colour(0xff1e1e3a));
        g.drawLine(x, 0.f, x, H, isBar ? 1.5f : 0.5f);

        if (isBar)
        {
            int barNum = (int)std::round(t / barSec) + 1;
            g.setColour(juce::Colour(0xff5a5a9a));
            g.setFont(10.f);
            g.drawText("Bar " + juce::String(barNum), (int)x + 2, 2, 40, 12,
                       juce::Justification::left);
        }
    }

    // 16th note sub-grid (lighter)
    double subDiv = beatSec / 4.0;
    for (double t = startBeat; t < endTime; t += subDiv)
    {
        // Skip if already drawn as a beat line
        if (std::fmod(t + 0.0001, beatSec) < subDiv * 0.1) continue;
        float x = secToX(t);
        g.setColour(juce::Colour(0xff151528));
        g.drawLine(x, 20.f, x, H, 0.5f);
    }
}

void AlignmentTimeline::drawSnapGuides(juce::Graphics& g)
{
    if (!lastResult_.valid) return;
    // Draw the "snap target" beat as a green line
    double snapTargetSec = lastResult_.nearestBeatMs / 1000.0;
    float x = secToX(snapTargetSec);
    g.setColour(juce::Colour(0xff1D9E75).withAlpha(0.6f));
    g.drawLine(x, 0.f, x, (float)getHeight(), 2.f);

    // Label
    g.setColour(juce::Colour(0xff5DCAA5));
    g.setFont(10.f);
    g.drawText("snap target", (int)x + 3, getHeight() - 14, 70, 12,
               juce::Justification::left);
}

void AlignmentTimeline::drawRegions(juce::Graphics& g)
{
    const auto& regions = chopEngine_.getRegions();
    const float midY = (float)getHeight() * 0.5f;
    const float regionH = (float)getHeight() * 0.45f;

    while (regionOffsets_.size() < regions.size())
        regionOffsets_.push_back(0.0);

    for (size_t i = 0; i < regions.size(); ++i)
    {
        const auto& r = regions[i];
        double offset = regionOffsets_[i];

        float x1 = secToX(r.startSec + offset);
        float x2 = secToX(r.endSec   + offset);

        // Region fill
        g.setColour(r.colour.withAlpha(0.35f));
        g.fillRect(x1, midY - regionH / 2.f, x2 - x1, regionH);

        // Border
        g.setColour(r.colour.withAlpha(0.8f));
        g.drawRect(x1, midY - regionH / 2.f, x2 - x1, regionH, 1.f);

        // Label
        g.setColour(r.colour.brighter(0.3f));
        g.setFont(10.f);
        g.drawText("R" + juce::String(r.id), (int)x1 + 2, (int)(midY - regionH / 2.f) + 2,
                   30, 12, juce::Justification::left);

        // Offset indicator
        if (std::abs(offset) > 0.001)
        {
            g.setColour(juce::Colours::yellow.withAlpha(0.7f));
            g.setFont(9.f);
            juce::String offStr = (offset > 0 ? "+" : "") +
                                  juce::String(offset * 1000.0, 0) + "ms";
            g.drawText(offStr, (int)x1 + 2, (int)(midY + regionH / 2.f - 14), 50, 12,
                       juce::Justification::left);
        }
    }
}

void AlignmentTimeline::drawOffsetArrow(juce::Graphics& g)
{
    const float y = (float)getHeight() * 0.15f;
    double onsetSec  = lastResult_.nearestBeatMs / 1000.0 + lastResult_.offsetMs / 1000.0;
    double targetSec = lastResult_.nearestBeatMs / 1000.0;

    float x1 = secToX(onsetSec);
    float x2 = secToX(targetSec);

    // Arrow line
    g.setColour(juce::Colour(0xffEF9F27).withAlpha(0.85f));
    g.drawLine(x1, y, x2, y, 1.5f);

    // Arrowhead
    float dir = x2 > x1 ? 1.f : -1.f;
    juce::Path arrow;
    arrow.addTriangle(x2, y, x2 - dir * 6.f, y - 4.f, x2 - dir * 6.f, y + 4.f);
    g.fillPath(arrow);

    // Label
    g.setColour(juce::Colour(0xffEF9F27));
    g.setFont(10.f);
    juce::String label = juce::String(std::abs(lastResult_.offsetMs), 0) + "ms "
                       + (lastResult_.offsetMs > 0 ? "late" : "early");
    float midX = (x1 + x2) / 2.f;
    g.drawText(label, (int)midX - 25, (int)y - 16, 60, 12, juce::Justification::centred);

    // Dot at onset
    g.setColour(juce::Colour(0xffEF9F27));
    g.fillEllipse(x1 - 4.f, y - 4.f, 8.f, 8.f);
}

void AlignmentTimeline::drawRuler(juce::Graphics& g)
{
    juce::Rectangle<float> rulerBounds(0.f, 0.f, (float)getWidth(), 16.f);
    g.setColour(juce::Colour(0xff131326));
    g.fillRect(rulerBounds);

    g.setColour(juce::Colour(0xff3a3a5c));
    g.setFont(9.f);

    double beatSec = 60.0 / bpm_;
    double endTime = viewOffsetSec_ + (double)getWidth() * secPerPixel_;
    double step = beatSec;
    if (secPerPixel_ < 0.002) step = beatSec / 4.0;

    for (double t = 0.0; t < endTime; t += step)
    {
        float x = secToX(t);
        g.setColour(juce::Colour(0xff4a4a7a));
        g.drawLine(x, 10.f, x, 16.f, 0.5f);
        g.setColour(juce::Colour(0xff6a6a9a));
        g.drawText(juce::String(t, 2) + "s", (int)x + 1, 0, 36, 10,
                   juce::Justification::left);
    }
}

void AlignmentTimeline::mouseDown(const juce::MouseEvent& e)
{
    const auto& regions = chopEngine_.getRegions();
    while (regionOffsets_.size() < regions.size()) regionOffsets_.push_back(0.0);

    double clickSec = xToSec((float)e.x);
    drag_.regionIdx = -1;

    for (int i = 0; i < (int)regions.size(); ++i)
    {
        double offset = regionOffsets_[(size_t)i];
        double s = regions[(size_t)i].startSec + offset;
        double en = regions[(size_t)i].endSec + offset;
        if (clickSec >= s && clickSec <= en)
        {
            drag_.regionIdx     = i;
            drag_.dragStartSec  = clickSec;
            drag_.dragOffsetSec = offset;
            break;
        }
    }
}

void AlignmentTimeline::mouseDrag(const juce::MouseEvent& e)
{
    if (drag_.regionIdx < 0) return;
    double curSec = xToSec((float)e.x);
    double delta  = curSec - drag_.dragStartSec;

    regionOffsets_[(size_t)drag_.regionIdx] = drag_.dragOffsetSec + delta;
    repaint();

    if (onRegionDragged)
        onRegionDragged(drag_.regionIdx,
                        regionOffsets_[(size_t)drag_.regionIdx] * 1000.0);
}

void AlignmentTimeline::mouseUp(const juce::MouseEvent&)
{
    drag_.regionIdx = -1;
}

void AlignmentTimeline::mouseWheelMove(const juce::MouseEvent&,
                                        const juce::MouseWheelDetails& w)
{
    if (w.isReversed)
        viewOffsetSec_ -= w.deltaY * secPerPixel_ * 100.0;
    else
        secPerPixel_ = juce::jlimit(0.001, 0.1, secPerPixel_ * (1.0 - (double)w.deltaY * 0.1));
    repaint();
}
