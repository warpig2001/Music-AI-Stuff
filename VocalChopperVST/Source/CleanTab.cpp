#include "CleanTab.h"

static juce::Colour kDark    (0xff0d0d1a);
static juce::Colour kMid     (0xff1a1a2e);
static juce::Colour kAccent  (0xffAFA9EC);
static juce::Colour kBorder  (0xff2a2a4a);
static juce::Colour kGreen   (0xff5DCAA5);
static juce::Colour kAmber   (0xffEF9F27);
static juce::Colour kRed     (0xffE24B4A);
static juce::Colour kPurple  (0xff7F77DD);

// ═════════════════════════════════════════════════════════════════════════════
// GateViewer
// ═════════════════════════════════════════════════════════════════════════════

GateViewer::GateViewer() { setOpaque(true); }

void GateViewer::setBuffer(const juce::AudioBuffer<float>* buf, double sr)
{
    buffer_      = buf;
    sampleRate_  = sr;
    hasScan_     = false;
    repaint();
}

void GateViewer::setScanResult(const CleanScanResult& r)
{
    scanResult_ = r;
    hasScan_    = r.valid;
    repaint();
}

void GateViewer::setThresholdDb(float db)
{
    thresholdDb_ = db;
    repaint();
}

void GateViewer::paint(juce::Graphics& g)
{
    g.fillAll(kDark);

    if (!buffer_ || buffer_->getNumSamples() == 0) { drawNoData(g); return; }

    auto b = getLocalBounds().toFloat().reduced(2.f);
    drawWaveform  (g, b);
    if (hasScan_) drawRegions(g, b);
    drawThreshLine(g, b);
    drawLegend    (g, b);
}

void GateViewer::drawWaveform(juce::Graphics& g, juce::Rectangle<float> b)
{
    const int total = buffer_->getNumSamples();
    const int W     = (int)b.getWidth();
    const float midY = b.getCentreY();
    const float halfH = b.getHeight() * 0.42f;

    // Background grid (dB lines)
    static const float kDbLines[] = { -12.f, -24.f, -36.f, -48.f, -60.f };
    for (float db : kDbLines)
    {
        float y = dbToY(db, b);
        g.setColour(kBorder.withAlpha(0.5f));
        g.drawLine(b.getX(), y, b.getRight(), y, 0.5f);
        g.setColour(kAccent.withAlpha(0.25f));
        g.setFont(9.f);
        g.drawText(juce::String((int)db) + " dB",
                   (int)b.getX() + 2, (int)y - 10, 40, 10,
                   juce::Justification::left);
    }

    // Waveform bars
    for (int px = 0; px < W; ++px)
    {
        int startS = (int)((double)px       / W * total);
        int endS   = (int)((double)(px + 1) / W * total);
        endS = juce::jlimit(startS + 1, total, endS);

        float peak = 0.f;
        for (int ch = 0; ch < buffer_->getNumChannels(); ++ch)
        {
            const float* d = buffer_->getReadPointer(ch);
            for (int s = startS; s < endS; ++s)
                peak = std::max(peak, std::abs(d[s]));
        }

        float h = peak * halfH;
        float x = b.getX() + (float)px;

        float alpha = 0.35f + peak * 0.55f;
        g.setColour(kPurple.withAlpha(alpha));
        g.drawLine(x, midY - h, x, midY + h, 1.f);
    }
}

void GateViewer::drawRegions(juce::Graphics& g, juce::Rectangle<float> b)
{
    const int total = buffer_->getNumSamples();

    // Dead (noise) regions — red overlay
    for (const auto& dr : scanResult_.deadRegions)
    {
        int startS = (int)(dr.startSec * sampleRate_);
        int endS   = (int)(dr.endSec   * sampleRate_);
        float x1 = sampleToX(startS, b);
        float x2 = sampleToX(endS,   b);

        juce::Colour fill = dr.isSilence ? kBorder.withAlpha(0.3f)
                                         : kRed.withAlpha(0.22f);
        g.setColour(fill);
        g.fillRect(x1, b.getY(), x2 - x1, b.getHeight());

        // Top tick
        g.setColour(dr.isSilence ? kBorder : kRed.withAlpha(0.6f));
        g.drawLine(x1, b.getY(), x1, b.getY() + 6.f, 1.f);
        g.drawLine(x2, b.getY(), x2, b.getY() + 6.f, 1.f);
    }

    // Live regions — green tint
    for (const auto& lr : scanResult_.liveRegions)
    {
        float x1 = sampleToX((int)(lr.startSec * sampleRate_), b);
        float x2 = sampleToX((int)(lr.endSec   * sampleRate_), b);
        g.setColour(kGreen.withAlpha(0.06f));
        g.fillRect(x1, b.getY(), x2 - x1, b.getHeight());
    }

    (void)total;
}

void GateViewer::drawThreshLine(juce::Graphics& g, juce::Rectangle<float> b)
{
    float y = dbToY(thresholdDb_, b);

    // Dashed threshold line
    juce::Path line;
    line.startNewSubPath(b.getX(), y);
    line.lineTo(b.getRight(), y);
    float dashes[] = { 6.f, 4.f };
    juce::PathStrokeType stroke(1.5f);
    juce::Path dashed;
    stroke.createDashedStroke(dashed, line, dashes, 2);
    g.setColour(kAmber.withAlpha(0.85f));
    g.fillPath(dashed);

    // Label
    g.setColour(kAmber);
    g.setFont(10.f);
    g.drawText("Gate: " + juce::String((int)thresholdDb_) + " dB",
               (int)b.getRight() - 80, (int)y - 14, 78, 12,
               juce::Justification::right);

    // Drag handle
    g.setColour(kAmber.withAlpha(draggingThresh_ ? 1.f : 0.6f));
    g.fillEllipse(b.getX() + 4.f, y - 5.f, 10.f, 10.f);
}

void GateViewer::drawLegend(juce::Graphics& g, juce::Rectangle<float> b)
{
    float x = b.getRight() - 130.f;
    float y = b.getY() + 6.f;

    g.setFont(9.f);
    // Live
    g.setColour(kGreen.withAlpha(0.7f));
    g.fillRect(x, y, 10.f, 10.f);
    g.setColour(kAccent.withAlpha(0.6f));
    g.drawText("kept (live)", (int)x + 13, (int)y, 60, 10, juce::Justification::left);

    y += 14.f;
    g.setColour(kRed.withAlpha(0.6f));
    g.fillRect(x, y, 10.f, 10.f);
    g.setColour(kAccent.withAlpha(0.6f));
    g.drawText("noise / silence", (int)x + 13, (int)y, 80, 10, juce::Justification::left);
}

void GateViewer::drawNoData(juce::Graphics& g)
{
    g.setColour(kBorder);
    g.setFont(13.f);
    g.drawText("Load a vocal file to scan for noise",
               getLocalBounds(), juce::Justification::centred);

    // Dashed border
    juce::Path border;
    border.addRectangle(getLocalBounds().toFloat().reduced(8.f));
    float dashes[] = {6.f, 4.f};
    juce::PathStrokeType st(1.f);
    juce::Path dashed;
    st.createDashedStroke(dashed, border, dashes, 2);
    g.setColour(kBorder.withAlpha(0.6f));
    g.fillPath(dashed);
}

float GateViewer::dbToY(float db, juce::Rectangle<float> b) const
{
    // Map dB range [-80, 0] to [bottom, top]
    float norm = (db - (-80.f)) / (0.f - (-80.f));
    norm = juce::jlimit(0.f, 1.f, norm);
    return b.getBottom() - norm * b.getHeight();
}

float GateViewer::yToDb(float y, juce::Rectangle<float> b) const
{
    float norm = (b.getBottom() - y) / b.getHeight();
    norm = juce::jlimit(0.f, 1.f, norm);
    return -80.f + norm * 80.f;
}

float GateViewer::sampleToX(int sample, juce::Rectangle<float> b) const
{
    if (!buffer_ || buffer_->getNumSamples() == 0) return b.getX();
    float norm = (float)sample / (float)buffer_->getNumSamples();
    return b.getX() + norm * b.getWidth();
}

void GateViewer::mouseDown(const juce::MouseEvent& e)
{
    auto b = getLocalBounds().toFloat().reduced(2.f);
    float threshY = dbToY(thresholdDb_, b);
    if (std::abs(e.y - threshY) < 12.f)
        draggingThresh_ = true;
}

void GateViewer::mouseDrag(const juce::MouseEvent& e)
{
    if (!draggingThresh_) return;
    auto b = getLocalBounds().toFloat().reduced(2.f);
    float newDb = yToDb((float)e.y, b);
    newDb = juce::jlimit(-80.f, -3.f, newDb);
    thresholdDb_ = newDb;
    repaint();
    if (onThresholdDragged) onThresholdDragged(newDb);
}

void GateViewer::mouseUp(const juce::MouseEvent&)
{
    draggingThresh_ = false;
}

// ═════════════════════════════════════════════════════════════════════════════
// CleanTab
// ═════════════════════════════════════════════════════════════════════════════

CleanTab::CleanTab(const juce::AudioBuffer<float>** bufPtr, double* srPtr)
    : juce::Thread("NoiseGateScan"),
      sourceBufferPtr_(bufPtr),
      sampleRatePtr_(srPtr),
      progressBar_(progress_)
{
    addAndMakeVisible(viewer_);

    // Threshold slider
    thresholdSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
    thresholdSlider_.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    thresholdSlider_.setRange(-80.0, -3.0, 0.5);
    thresholdSlider_.setValue(-40.0);
    thresholdSlider_.setTextValueSuffix(" dB");
    thresholdSlider_.onValueChange = [this] {
        viewer_.setThresholdDb((float)thresholdSlider_.getValue());
    };

    // Min dead duration slider
    minDeadSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
    minDeadSlider_.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    minDeadSlider_.setRange(20.0, 1000.0, 5.0);
    minDeadSlider_.setValue(80.0);
    minDeadSlider_.setTextValueSuffix(" ms");

    // Hold slider
    holdSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
    holdSlider_.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    holdSlider_.setRange(0.0, 500.0, 5.0);
    holdSlider_.setValue(50.0);
    holdSlider_.setTextValueSuffix(" ms");

    // Labels
    auto initLabel = [this](juce::Label& lbl) {
        lbl.setFont(juce::Font(11.f));
        lbl.setColour(juce::Label::textColourId, kAccent.withAlpha(0.6f));
        addAndMakeVisible(&lbl);
    };
    initLabel(threshLabel_);
    initLabel(minDeadLabel_);
    initLabel(holdLabel_);

    // Stats label
    statsLabel_.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 11.f, juce::Font::plain));
    statsLabel_.setColour(juce::Label::textColourId, kAccent.withAlpha(0.7f));
    statsLabel_.setJustificationType(juce::Justification::topLeft);
    statsLabel_.setText("No scan yet. Load a vocal file and click 'Scan for noise'.", juce::dontSendNotification);

    // Suggested threshold
    suggestLabel_.setFont(juce::Font(11.f));
    suggestLabel_.setColour(juce::Label::textColourId, kAmber.withAlpha(0.8f));

    // Buttons
    scanBtn_.onClick = [this] { doScan(); };
    applyBtn_.onClick = [this] { doApply(); };
    resetBtn_.onClick = [this] { doReset(); };
    exportBtn_.onClick = [this] {
        if (!cleanBuffer_) return;
        fileChooser_ = std::make_unique<juce::FileChooser>(
            "Save clean vocal",
            juce::File::getSpecialLocation(juce::File::userMusicDirectory),
            "*.wav");
        fileChooser_->launchAsync(
            juce::FileBrowserComponent::saveMode |
            juce::FileBrowserComponent::canSelectFiles,
            [this](const juce::FileChooser& fc) {
                auto results = fc.getResults();
                if (results.isEmpty()) return;
                auto f = results[0].withFileExtension(".wav");
                juce::WavAudioFormat fmt;
                auto stream = f.createOutputStream();
                if (!stream) return;
                juce::StringPairArray metadata;
                auto* writer = fmt.createWriterFor(
                    stream.get(),
                    *sampleRatePtr_,
                    (unsigned)cleanBuffer_->getNumChannels(),
                    24, metadata, 0);
                if (writer) {
                    stream.release();   // writer now owns the stream
                    std::unique_ptr<juce::AudioFormatWriter> w(writer);
                    w->writeFromAudioSampleBuffer(
                        *cleanBuffer_, 0, cleanBuffer_->getNumSamples());
                }
            });
    };

    applyBtn_.setEnabled(false);
    resetBtn_.setEnabled(false);
    exportBtn_.setEnabled(false);

    // Viewer threshold drag callback
    viewer_.onThresholdDragged = [this](float db) {
        thresholdSlider_.setValue(db, juce::dontSendNotification);
    };

    addAndMakeVisible(thresholdSlider_);
    addAndMakeVisible(minDeadSlider_);
    addAndMakeVisible(holdSlider_);
    addAndMakeVisible(scanBtn_);
    addAndMakeVisible(applyBtn_);
    addAndMakeVisible(resetBtn_);
    addAndMakeVisible(exportBtn_);
    addAndMakeVisible(autoBtn_);
    addAndMakeVisible(statsLabel_);
    addAndMakeVisible(suggestLabel_);
    addAndMakeVisible(progressBar_);

    progressBar_.setVisible(false);
}

CleanTab::~CleanTab()
{
    stopThread(3000);
}

void CleanTab::onFileLoaded()
{
    cleanBuffer_.reset();
    lastScan_ = CleanScanResult{};
    applyBtn_.setEnabled(false);
    resetBtn_.setEnabled(false);
    exportBtn_.setEnabled(false);
    statsLabel_.setText("File loaded. Click 'Scan for noise' to analyze.",
                         juce::dontSendNotification);

    if (*sourceBufferPtr_)
        viewer_.setBuffer(*sourceBufferPtr_, *sampleRatePtr_);

    if (autoBtn_.getToggleState())
        doScan();
}

const juce::AudioBuffer<float>* CleanTab::getCleanBuffer() const
{
    return cleanBuffer_.get();
}

void CleanTab::paint(juce::Graphics& g)
{
    g.fillAll(kDark);
}

void CleanTab::resized()
{
    auto b = getLocalBounds().reduced(8);

    // Viewer takes most of the vertical space
    viewer_.setBounds(b.removeFromTop(160));
    b.removeFromTop(6);

    // Three sliders in a row
    auto sliderRow = b.removeFromTop(28);
    int slotW = sliderRow.getWidth() / 3;

    auto slot1 = sliderRow.removeFromLeft(slotW);
    auto slot2 = sliderRow.removeFromLeft(slotW);
    auto slot3 = sliderRow;

    threshLabel_.setBounds(slot1.removeFromLeft(110));
    thresholdSlider_.setBounds(slot1);
    minDeadLabel_.setBounds(slot2.removeFromLeft(120));
    minDeadSlider_.setBounds(slot2);
    holdLabel_.setBounds(slot3.removeFromLeft(70));
    holdSlider_.setBounds(slot3);

    b.removeFromTop(4);

    // Suggested threshold + auto toggle
    auto row2 = b.removeFromTop(22);
    suggestLabel_.setBounds(row2.removeFromLeft(260));
    autoBtn_.setBounds(row2.removeFromLeft(160));

    b.removeFromTop(4);

    // Button row
    auto btnRow = b.removeFromTop(30);
    scanBtn_.setBounds   (btnRow.removeFromLeft(130).reduced(2, 2));
    applyBtn_.setBounds  (btnRow.removeFromLeft(110).reduced(2, 2));
    resetBtn_.setBounds  (btnRow.removeFromLeft(130).reduced(2, 2));
    exportBtn_.setBounds (btnRow.removeFromLeft(140).reduced(2, 2));

    b.removeFromTop(4);
    progressBar_.setBounds(b.removeFromTop(16));
    b.removeFromTop(4);
    statsLabel_.setBounds(b);
}

// ─── Background thread ────────────────────────────────────────────────────────

void CleanTab::doScan()
{
    if (!*sourceBufferPtr_) return;
    if (isThreadRunning()) return;

    scanning_ = true;
    progress_ = 0.0;
    progressBar_.setVisible(true);
    scanBtn_.setEnabled(false);
    statsLabel_.setText("Scanning...", juce::dontSendNotification);

    startThread();
}

void CleanTab::run()
{
    if (!*sourceBufferPtr_) return;

    gateEngine_.setSampleRate(*sampleRatePtr_);
    gateEngine_.onProgress = [this](float p) {
        progress_ = (double)p;
    };

    float threshold = (float)thresholdSlider_.getValue();
    float minDead   = (float)minDeadSlider_.getValue();
    float hold      = (float)holdSlider_.getValue();

    lastScan_ = gateEngine_.scan(**sourceBufferPtr_, threshold, minDead, hold);

    // Notify UI on message thread
    juce::MessageManager::callAsync([this] { notifyScanComplete(); });
}

void CleanTab::notifyScanComplete()
{
    scanning_ = false;
    progressBar_.setVisible(false);
    scanBtn_.setEnabled(true);
    applyBtn_.setEnabled(lastScan_.valid);

    viewer_.setScanResult(lastScan_);
    updateStats();
    updateSuggestedThreshold();
}

void CleanTab::updateStats()
{
    if (!lastScan_.valid) return;

    double cleanPct = lastScan_.audioFileSec > 0
        ? (lastScan_.totalLiveSec / lastScan_.audioFileSec * 100.0) : 0.0;
    double deadPct  = lastScan_.audioFileSec > 0
        ? (lastScan_.totalDeadSec / lastScan_.audioFileSec * 100.0) : 0.0;

    juce::String s;
    s << "Scan complete:\n"
      << "  File duration:   " << juce::String(lastScan_.audioFileSec, 2) << " sec\n"
      << "  Live audio:      " << juce::String(lastScan_.totalLiveSec, 2)
                               << " sec  (" << juce::String(cleanPct, 0) << "%)\n"
      << "  Noise / silence: " << juce::String(lastScan_.totalDeadSec, 2)
                               << " sec  (" << juce::String(deadPct, 0) << "%)  —  "
                               << lastScan_.deadCount << " gap" << (lastScan_.deadCount != 1 ? "s" : "") << "\n"
      << "  Peak level:      " << juce::String(lastScan_.peakDb, 1) << " dB\n"
      << "  Noise floor est: " << juce::String(lastScan_.noiseFloorDb, 1) << " dB\n"
      << "  Gate threshold:  " << juce::String((float)thresholdSlider_.getValue(), 1) << " dB\n";

    if (lastScan_.deadCount == 0)
        s << "\n  No removable noise regions found at this threshold.\n"
          << "  Try lowering the threshold or clicking the suggested value.";
    else
        s << "\n  Click 'Apply clean' to silence the " << lastScan_.deadCount
          << " noise region" << (lastScan_.deadCount != 1 ? "s" : "")
          << ", or drag the orange threshold line in the viewer to adjust.";

    statsLabel_.setText(s, juce::dontSendNotification);
}

void CleanTab::updateSuggestedThreshold()
{
    if (!lastScan_.valid) return;
    suggestLabel_.setText(
        "Suggested gate: " + juce::String(lastScan_.suggestedGateDb, 1) + " dB  "
        "(noise floor " + juce::String(lastScan_.noiseFloorDb, 1) + " dB + 12 dB headroom)",
        juce::dontSendNotification);
}

void CleanTab::doApply()
{
    if (!*sourceBufferPtr_ || !lastScan_.valid) return;

    auto result = gateEngine_.applyGate(**sourceBufferPtr_, lastScan_, false);
    gateEngine_.applyBoundaryFades(result, lastScan_, 5.f);

    cleanBuffer_ = std::make_unique<juce::AudioBuffer<float>>(std::move(result));

    resetBtn_.setEnabled(true);
    exportBtn_.setEnabled(true);

    statsLabel_.setText(
        statsLabel_.getText() + "\n\nClean applied — "
        + juce::String(lastScan_.deadCount) + " noise region"
        + (lastScan_.deadCount != 1 ? "s" : "")
        + " silenced with 5ms boundary fades.\n"
        + "Use 'Save clean file...' to export as WAV, or the processor\n"
        + "will use the clean version for playback automatically.",
        juce::dontSendNotification);

    if (onCleanReady) onCleanReady();
}

void CleanTab::doReset()
{
    cleanBuffer_.reset();
    resetBtn_.setEnabled(false);
    exportBtn_.setEnabled(false);
    statsLabel_.setText(statsLabel_.getText() + "\n\nReset — original file restored.",
                         juce::dontSendNotification);
    if (onCleanReady) onCleanReady();
}
