#include "StretchTab.h"
#include "LookAndFeel.h"

using namespace VCTheme;

const juce::Colour StretchTab::kRegionColours_[] = {
    c(ACCENT_BLUE), c(ACCENT_GREEN), c(ACCENT_AMBER), c(ACCENT_RED),
    c(ACCENT_PURPLE), c(ACCENT_TEAL), juce::Colour(0xfffb923c), juce::Colour(0xff60a5fa)
};

// ─────────────────────────────────────────────────────────────────────────────
// RegionStretchCard
// ─────────────────────────────────────────────────────────────────────────────

RegionStretchCard::RegionStretchCard(int id, StretchEngine& eng, juce::Colour col,
                                      double regionDurSec)
    : regionId_(id), engine_(eng), colour_(col), regionDurSec_(regionDurSec),
      enableBtn_("Enable"), preservePitchBtn_("Lock pitch")
{
    idLabel_.setText("R" + juce::String(id), juce::dontSendNotification);
    idLabel_.setFont(juce::Font(13.f, juce::Font::bold));
    idLabel_.setColour(juce::Label::textColourId, col);
    addAndMakeVisible(idLabel_);

    // Source duration
    srcDurLabel_.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 10.f, juce::Font::plain));
    srcDurLabel_.setColour(juce::Label::textColourId, c(TEXT_MUTED));
    srcDurLabel_.setText(juce::String(regionDurSec * 1000.0, 0) + " ms in", juce::dontSendNotification);
    addAndMakeVisible(srcDurLabel_);

    // Output duration estimate
    durLabel_.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 10.f, juce::Font::plain));
    durLabel_.setColour(juce::Label::textColourId, c(TEXT_MUTED));
    updateDurLabel();
    addAndMakeVisible(durLabel_);

    // Enable toggle
    enableBtn_.setToggleState(false, juce::dontSendNotification);
    enableBtn_.setTooltip("Enable pitch/stretch processing for this region");
    enableBtn_.onClick = [this] {
        auto s = engine_.getSettings(regionId_);
        s.enabled  = enableBtn_.getToggleState();
        s.regionId = regionId_;
        engine_.setSettings(s);
        pitchSlider_.setEnabled(s.enabled);
        stretchSlider_.setEnabled(s.enabled);
        preservePitchBtn_.setEnabled(s.enabled);
        if (onSettingsChanged) onSettingsChanged(regionId_);
    };
    addAndMakeVisible(enableBtn_);

    // Pitch slider  ±24 semitones
    pitchSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
    pitchSlider_.setTextBoxStyle(juce::Slider::TextBoxRight, false, 54, 18);
    pitchSlider_.setRange(-24.0, 24.0, 0.5);
    pitchSlider_.setValue(0.0);
    pitchSlider_.setTextValueSuffix(" st");
    pitchSlider_.setEnabled(false);
    pitchSlider_.setTooltip("Pitch shift in semitones (±24)");
    pitchSlider_.getProperties().set("accentColour", (int)col.getARGB());
    pitchSlider_.onValueChange = [this] {
        auto s = engine_.getSettings(regionId_);
        s.pitchSemitones = (float)pitchSlider_.getValue();
        s.regionId = regionId_;
        engine_.setSettings(s);
        if (onSettingsChanged) onSettingsChanged(regionId_);
    };
    addAndMakeVisible(pitchSlider_);

    pitchLabel_.setFont(juce::Font(9.f).withExtraKerningFactor(0.08f));
    pitchLabel_.setColour(juce::Label::textColourId, c(TEXT_MUTED));
    addAndMakeVisible(pitchLabel_);

    // Stretch slider  0.25x–4.0x
    stretchSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
    stretchSlider_.setTextBoxStyle(juce::Slider::TextBoxRight, false, 54, 18);
    stretchSlider_.setRange(0.25, 4.0, 0.05);
    stretchSlider_.setValue(1.0);
    stretchSlider_.setTextValueSuffix("x");
    stretchSlider_.setEnabled(false);
    stretchSlider_.setTooltip("Time stretch ratio — 0.25x (quarter speed) to 4.0x (quadruple speed)");
    stretchSlider_.getProperties().set("accentColour", (int)col.getARGB());
    stretchSlider_.onValueChange = [this] {
        auto s = engine_.getSettings(regionId_);
        s.stretchRatio = (float)stretchSlider_.getValue();
        s.regionId = regionId_;
        engine_.setSettings(s);
        updateDurLabel();
        if (onSettingsChanged) onSettingsChanged(regionId_);
    };
    addAndMakeVisible(stretchSlider_);

    stretchLabel_.setFont(juce::Font(9.f).withExtraKerningFactor(0.08f));
    stretchLabel_.setColour(juce::Label::textColourId, c(TEXT_MUTED));
    addAndMakeVisible(stretchLabel_);

    // Preserve pitch toggle
    preservePitchBtn_.setToggleState(true, juce::dontSendNotification);
    preservePitchBtn_.setEnabled(false);
    preservePitchBtn_.setTooltip("Compensate pitch when stretching time so the note stays in tune");
    preservePitchBtn_.onClick = [this] {
        auto s = engine_.getSettings(regionId_);
        s.preservePitch = preservePitchBtn_.getToggleState();
        s.regionId = regionId_;
        engine_.setSettings(s);
        if (onSettingsChanged) onSettingsChanged(regionId_);
    };
    addAndMakeVisible(preservePitchBtn_);
}

void RegionStretchCard::updateDurLabel()
{
    const auto& s = engine_.getSettings(regionId_);
    double outSec = engine_.estimateOutputDuration(regionDurSec_, s);
    durLabel_.setText(juce::String(outSec * 1000.0, 0) + " ms out",
                       juce::dontSendNotification);
    // Colour-code: green if shorter, amber if longer
    if (outSec < regionDurSec_ * 0.95)
        durLabel_.setColour(juce::Label::textColourId, c(ACCENT_GREEN));
    else if (outSec > regionDurSec_ * 1.05)
        durLabel_.setColour(juce::Label::textColourId, c(ACCENT_AMBER));
    else
        durLabel_.setColour(juce::Label::textColourId, c(TEXT_MUTED));
}

void RegionStretchCard::refreshFromEngine()
{
    const auto& s = engine_.getSettings(regionId_);
    enableBtn_.setToggleState       (s.enabled,        juce::dontSendNotification);
    pitchSlider_.setValue           (s.pitchSemitones,  juce::dontSendNotification);
    stretchSlider_.setValue         (s.stretchRatio,    juce::dontSendNotification);
    preservePitchBtn_.setToggleState(s.preservePitch,   juce::dontSendNotification);
    pitchSlider_.setEnabled         (s.enabled);
    stretchSlider_.setEnabled       (s.enabled);
    preservePitchBtn_.setEnabled    (s.enabled);
    updateDurLabel();
}

void RegionStretchCard::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced(1.f);

    g.setColour(c(BG_RAISED));
    g.fillRoundedRectangle(b, VCTheme::RADIUS_MD);
    g.setColour(colour_.withAlpha(0.2f));
    g.drawRoundedRectangle(b, VCTheme::RADIUS_MD, 0.5f);

    // Left colour stripe
    g.setColour(colour_.withAlpha(enableBtn_.getToggleState() ? 0.8f : 0.3f));
    g.fillRoundedRectangle(b.removeFromLeft(4.f).reduced(0.f, 1.f), 2.f);
}

void RegionStretchCard::resized()
{
    auto b = getLocalBounds().reduced(6, 4);
    b.removeFromLeft(10);  // stripe space

    // Top row
    auto top = b.removeFromTop(22);
    idLabel_.setBounds      (top.removeFromLeft(30));   top.removeFromLeft(4);
    enableBtn_.setBounds    (top.removeFromLeft(72));   top.removeFromLeft(8);
    preservePitchBtn_.setBounds(top.removeFromLeft(94)); top.removeFromLeft(8);
    srcDurLabel_.setBounds  (top.removeFromLeft(70));   top.removeFromLeft(4);
    durLabel_.setBounds     (top.removeFromLeft(80));

    b.removeFromTop(3);

    // Bottom row: two halves
    auto bot  = b;
    auto half = bot.removeFromLeft(bot.getWidth() / 2);
    pitchLabel_.setBounds  (half.removeFromLeft(36));
    pitchSlider_.setBounds (half);
    stretchLabel_.setBounds(bot.removeFromLeft(50));
    stretchSlider_.setBounds(bot);
}

// ─────────────────────────────────────────────────────────────────────────────
// StretchTab
// ─────────────────────────────────────────────────────────────────────────────

StretchTab::StretchTab(StretchEngine& eng, ChopEngine& chop, SpectrumAnalyzer& spec)
    : engine_(eng), chop_(chop), spectrum_(spec), specOverlay_(spec)
{
    viewport_.setScrollBarsShown(true, false);
    viewport_.setViewedComponent(&cardContainer_, false);
    addAndMakeVisible(viewport_);

    specOverlay_.setEnabled(false);
    specOverlay_.setInterceptsMouseClicks(false, false);
    addAndMakeVisible(specOverlay_);

    // Apply all — iterate only enabled regions, no duplicate firing
    applyAllBtn_.setTooltip("Render pitch/stretch for all enabled regions");
    applyAllBtn_.onClick = [this] {
        const auto& regions = chop_.getRegions();
        for (const auto& r : regions)
        {
            const auto& s = engine_.getSettings(r.id);
            if (s.enabled && onRegionProcessRequested)
                onRegionProcessRequested(r.id);
        }
    };

    resetAllBtn_.setTooltip("Reset all regions to original pitch and speed");
    resetAllBtn_.onClick = [this] {
        for (const auto& r : chop_.getRegions())
        {
            RegionStretchSettings def;
            def.regionId = r.id;
            engine_.setSettings(def);
        }
        for (auto& card : cards_) card->refreshFromEngine();
    };

    spectrumBtn_.setTooltip("Show live FFT spectrum overlay");
    spectrumBtn_.onStateChange = [this] {
        specOverlay_.setEnabled(spectrumBtn_.getToggleState());
        resized();
        repaint();
    };

    hintLabel_.setFont(juce::Font(11.f));
    hintLabel_.setColour(juce::Label::textColourId, c(TEXT_MUTED));
    hintLabel_.setText(
        "Enable a region, adjust pitch/stretch, then Render.  "
        "Pitch: ±24 st.  Stretch: 0.25x-4.0x.  Lock pitch prevents detuning when stretching.",
        juce::dontSendNotification);

    addAndMakeVisible(applyAllBtn_);
    addAndMakeVisible(resetAllBtn_);
    addAndMakeVisible(spectrumBtn_);
    addAndMakeVisible(hintLabel_);
}

void StretchTab::refreshFromRegions()
{
    cards_.clear();
    cardContainer_.removeAllChildren();

    const auto& regions = chop_.getRegions();

    for (size_t i = 0; i < regions.size(); ++i)
    {
        const auto& r   = regions[i];
        juce::Colour col = kRegionColours_[i % 8];
        double durSec    = r.endSec - r.startSec;

        auto card = std::make_unique<RegionStretchCard>(r.id, engine_, col, durSec);
        card->onSettingsChanged = [this](int) { repaint(); };
        cardContainer_.addAndMakeVisible(card.get());
        cards_.push_back(std::move(card));
    }

    layoutCards();
}

void StretchTab::layoutCards()
{
    const int cardH = 76;
    int totalH = (int)cards_.size() * (cardH + 4) + 8;
    const int containerW = juce::jmax(1, viewport_.getWidth());
    cardContainer_.setSize(containerW, juce::jmax(totalH, viewport_.getHeight()));

    int y = 4;
    for (auto& c : cards_)
    {
        c->setBounds(4, y, containerW - 8, cardH);
        y += cardH + 4;
    }
}

void StretchTab::onProcessingDone()
{
    hintLabel_.setText("Rendering complete. Check the waveform display to audition results.",
                        juce::dontSendNotification);
    hintLabel_.setColour(juce::Label::textColourId, c(ACCENT_GREEN));
}

void StretchTab::paint(juce::Graphics& g)
{
    g.fillAll(c(BG_BASE));
}

void StretchTab::resized()
{
    auto b = getLocalBounds().reduced(6);

    // Spectrum strip
    if (spectrumBtn_.getToggleState())
    {
        specOverlay_.setBounds(b.removeFromTop(60));
        b.removeFromTop(4);
    }
    else
    {
        specOverlay_.setBounds({});
    }

    hintLabel_.setBounds(b.removeFromTop(32));
    b.removeFromTop(4);

    auto row = b.removeFromTop(30);
    applyAllBtn_.setBounds(row.removeFromLeft(160).reduced(2, 2));
    resetAllBtn_.setBounds(row.removeFromLeft(100).reduced(2, 2));
    row.removeFromLeft(8);
    spectrumBtn_.setBounds(row.removeFromLeft(140).reduced(2, 2));
    b.removeFromTop(4);

    viewport_.setBounds(b);
    layoutCards();
}
