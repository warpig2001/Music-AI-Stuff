#pragma once
#include <JuceHeader.h>
#include "LookAndFeel.h"

// ─────────────────────────────────────────────────────────────────────────────
// SectionPanel
// A titled card with a thin top-accent stripe and hairline border.
// Use to wrap any logical group of controls on a tab.
// ─────────────────────────────────────────────────────────────────────────────
class SectionPanel : public juce::Component
{
public:
    explicit SectionPanel(const juce::String& title,
                           juce::Colour accentColour = VCTheme::c(VCTheme::ACCENT_BLUE))
        : title_(title), accent_(accentColour)
    {
        setOpaque(false);
    }

    void paint(juce::Graphics& g) override
    {
        using namespace VCTheme;
        auto b = getLocalBounds().toFloat().reduced(0.5f);

        // Card background
        g.setColour(c(BG_RAISED));
        g.fillRoundedRectangle(b, RADIUS_MD);

        // Border
        g.setColour(c(BORDER_SUBTLE));
        g.drawRoundedRectangle(b, RADIUS_MD, 0.5f);

        // Top accent stripe
        auto stripe = b.removeFromTop(2.f);
        g.setColour(accent_.withAlpha(0.6f));
        g.fillRect(stripe.withX(b.getX() + RADIUS_MD)
                        .withWidth(b.getWidth() - RADIUS_MD * 2.f));

        // Title text
        g.setColour(c(TEXT_MUTED));
        g.setFont(juce::Font(9.f, juce::Font::plain).withExtraKerningFactor(0.10f));
        g.drawText(title_.toUpperCase(), 12, 6, getWidth() - 24, 14,
                   juce::Justification::centredLeft, false);
    }

    // Call this in resized() — returns bounds for child content area
    juce::Rectangle<int> getContentBounds() const
    {
        return getLocalBounds().reduced(10, 8).withTrimmedTop(16);
    }

private:
    juce::String title_;
    juce::Colour accent_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SectionPanel)
};

// ─────────────────────────────────────────────────────────────────────────────
// VCToolbarButton
// Thin pill-shaped button used in the waveform header and control rows.
// Visually lighter than a full TextButton.
// ─────────────────────────────────────────────────────────────────────────────
class VCToolbarButton : public juce::TextButton
{
public:
    explicit VCToolbarButton(const juce::String& text) : juce::TextButton(text) {}

    void paintButton(juce::Graphics& g, bool highlighted, bool down) override
    {
        using namespace VCTheme;
        auto b = getLocalBounds().toFloat().reduced(0.5f);

        juce::Colour fill, border, textCol;

        if (getToggleState())
        {
            fill    = c(ACCENT_BLUE).withAlpha(0.2f);
            border  = c(ACCENT_BLUE).withAlpha(0.7f);
            textCol = c(ACCENT_BLUE);
        }
        else if (down)
        {
            fill    = c(BG_ACTIVE);
            border  = c(BORDER_STRONG);
            textCol = c(TEXT_PRIMARY);
        }
        else if (highlighted)
        {
            fill    = c(BG_HOVER);
            border  = c(BORDER_STRONG);
            textCol = c(TEXT_PRIMARY);
        }
        else
        {
            fill    = juce::Colours::transparentBlack;
            border  = c(BORDER_MID);
            textCol = c(TEXT_SECONDARY);
        }

        if (isDanger_)
        {
            textCol = c(ACCENT_RED).withAlpha(highlighted ? 1.f : 0.75f);
            border  = c(ACCENT_RED).withAlpha(highlighted ? 0.6f : 0.35f);
            fill    = down ? c(ACCENT_RED).withAlpha(0.15f) : juce::Colours::transparentBlack;
        }

        g.setColour(fill);
        g.fillRoundedRectangle(b, VCTheme::RADIUS_SM);
        g.setColour(border);
        g.drawRoundedRectangle(b, VCTheme::RADIUS_SM, 0.5f);
        g.setColour(textCol);
        g.setFont(juce::Font(10.f).withExtraKerningFactor(0.06f));
        g.drawText(getButtonText().toUpperCase(), getLocalBounds().reduced(6, 0),
                   juce::Justification::centred, false);
    }

    void setDanger(bool d) { isDanger_ = d; repaint(); }

private:
    bool isDanger_ = false;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VCToolbarButton)
};

// ─────────────────────────────────────────────────────────────────────────────
// PrimaryButton
// Full-filled action button (e.g. "Apply Clean", "Render all")
// ─────────────────────────────────────────────────────────────────────────────
class PrimaryButton : public juce::TextButton
{
public:
    explicit PrimaryButton(const juce::String& text,
                            juce::Colour accent = VCTheme::c(VCTheme::ACCENT_BLUE))
        : juce::TextButton(text), accent_(accent) {}

    void paintButton(juce::Graphics& g, bool highlighted, bool down) override
    {
        using namespace VCTheme;
        auto b = getLocalBounds().toFloat().reduced(0.5f);

        juce::Colour fill = down      ? accent_.darker(0.15f)
                          : highlighted ? accent_.brighter(0.08f)
                          :               accent_;
        if (!isEnabled()) fill = c(BG_SURFACE);

        g.setColour(fill);
        g.fillRoundedRectangle(b, RADIUS_SM);
        g.setColour(isEnabled() ? accent_.darker(0.1f) : c(BORDER_MID));
        g.drawRoundedRectangle(b, RADIUS_SM, 0.5f);

        g.setColour(isEnabled() ? juce::Colours::white : c(TEXT_DISABLED));
        g.setFont(juce::Font(11.f, juce::Font::bold).withExtraKerningFactor(0.06f));
        g.drawText(getButtonText().toUpperCase(), getLocalBounds().reduced(6, 0),
                   juce::Justification::centred, false);
    }

    void setAccent(juce::Colour c) { accent_ = c; repaint(); }

private:
    juce::Colour accent_;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PrimaryButton)
};

// ─────────────────────────────────────────────────────────────────────────────
// TogglePill
// Horizontal multi-option selector (like Waves curve type selectors)
// ─────────────────────────────────────────────────────────────────────────────
class TogglePill : public juce::Component
{
public:
    TogglePill() {}

    void addOption(const juce::String& label)
    {
        options_.push_back(label);
        if (selected_ < 0) selected_ = 0;
        repaint();
    }

    void setSelected(int idx)
    {
        selected_ = juce::jlimit(0, (int)options_.size() - 1, idx);
        repaint();
        if (onChange) onChange(selected_);
    }

    int getSelected() const { return selected_; }
    const juce::String& getSelectedText() const
    {
        static juce::String empty;
        return selected_ >= 0 && selected_ < (int)options_.size()
               ? options_[(size_t)selected_] : empty;
    }

    std::function<void(int)> onChange;

    void paint(juce::Graphics& g) override
    {
        using namespace VCTheme;
        auto b = getLocalBounds().toFloat().reduced(0.5f);

        // Container
        g.setColour(c(BG_SURFACE));
        g.fillRoundedRectangle(b, RADIUS_MD);
        g.setColour(c(BORDER_MID));
        g.drawRoundedRectangle(b, RADIUS_MD, 0.5f);

        if (options_.empty()) return;

        const float segW = b.getWidth() / (float)options_.size();
        const float pad = 2.f;

        for (int i = 0; i < (int)options_.size(); ++i)
        {
            juce::Rectangle<float> seg(b.getX() + i * segW + pad, b.getY() + pad,
                                        segW - pad * 2.f, b.getHeight() - pad * 2.f);

            if (i == selected_)
            {
                g.setColour(c(ACCENT_BLUE));
                g.fillRoundedRectangle(seg, RADIUS_SM);
                g.setColour(juce::Colours::white);
            }
            else
            {
                if (hoveredIdx_ == i)
                {
                    g.setColour(c(BG_HOVER));
                    g.fillRoundedRectangle(seg, RADIUS_SM);
                }
                g.setColour(c(TEXT_MUTED));
            }

            g.setFont(juce::Font(10.f).withExtraKerningFactor(0.05f));
            g.drawText(options_[(size_t)i].toUpperCase(), seg.toNearestInt(),
                       juce::Justification::centred, false);
        }
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        if (options_.empty()) return;
        int idx = (int)((float)e.x / (float)getWidth() * (float)options_.size());
        setSelected(juce::jlimit(0, (int)options_.size() - 1, idx));
    }

    void mouseMove(const juce::MouseEvent& e) override
    {
        int idx = (int)((float)e.x / (float)getWidth() * (float)options_.size());
        hoveredIdx_ = juce::jlimit(0, (int)options_.size() - 1, idx);
        repaint();
    }

    void mouseExit(const juce::MouseEvent&) override { hoveredIdx_ = -1; repaint(); }

private:
    std::vector<juce::String> options_;
    int selected_   = -1;
    int hoveredIdx_ = -1;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TogglePill)
};

// ─────────────────────────────────────────────────────────────────────────────
// LabelledSlider
// A complete "control unit" — label above, slider, value readout to right.
// Optionally shows a unit suffix and tooltip.
// ─────────────────────────────────────────────────────────────────────────────
class LabelledSlider : public juce::Component
{
public:
    juce::Slider slider;

    LabelledSlider(const juce::String& label,
                   const juce::String& suffix = "",
                   juce::Colour accentColour = VCTheme::c(VCTheme::ACCENT_BLUE))
        : label_(label), suffix_(suffix)
    {
        slider.setSliderStyle(juce::Slider::LinearHorizontal);
        slider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 58, 18);
        slider.setTextValueSuffix(suffix.isNotEmpty() ? " " + suffix : "");
        slider.getProperties().set("accentColour", (int)accentColour.getARGB());
        addAndMakeVisible(slider);
    }

    void resized() override
    {
        auto b = getLocalBounds();
        slider.setBounds(b);
    }

    void paint(juce::Graphics& g) override
    {
        using namespace VCTheme;
        g.setColour(c(TEXT_MUTED));
        g.setFont(juce::Font(9.f).withExtraKerningFactor(0.08f));
        g.drawText(label_.toUpperCase(), 0, 0, slider.getTextBoxWidth() > 0 ? getWidth() - 64 : getWidth(), 12,
                   juce::Justification::centredLeft);
    }

    int getIdealHeight() const { return 28; }

private:
    juce::String label_, suffix_;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LabelledSlider)
};

// ─────────────────────────────────────────────────────────────────────────────
// StatCard
// Small metric display — a number and a label underneath.
// Used in the Clean tab scan results.
// ─────────────────────────────────────────────────────────────────────────────
class StatCard : public juce::Component
{
public:
    StatCard(const juce::String& label, juce::Colour accent = VCTheme::c(VCTheme::ACCENT_BLUE))
        : label_(label), accent_(accent) {}

    void setValue(const juce::String& v) { value_ = v; repaint(); }
    void setAccent(juce::Colour c) { accent_ = c; repaint(); }

    void paint(juce::Graphics& g) override
    {
        using namespace VCTheme;
        auto b = getLocalBounds().toFloat().reduced(0.5f);

        g.setColour(c(BG_SURFACE));
        g.fillRoundedRectangle(b, RADIUS_MD);
        g.setColour(c(BORDER_SUBTLE));
        g.drawRoundedRectangle(b, RADIUS_MD, 0.5f);

        // Top accent stripe
        g.setColour(accent_.withAlpha(0.5f));
        g.fillRect(b.withHeight(2.f).withX(b.getX() + RADIUS_MD)
                    .withWidth(b.getWidth() - RADIUS_MD * 2.f));

        // Value
        g.setColour(value_.isEmpty() ? c(TEXT_DISABLED) : c(TEXT_PRIMARY));
        g.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 16.f, juce::Font::bold));
        g.drawText(value_.isEmpty() ? "—" : value_,
                   getLocalBounds().withTrimmedTop(8).withTrimmedBottom(16),
                   juce::Justification::centred, false);

        // Label
        g.setColour(c(TEXT_MUTED));
        g.setFont(juce::Font(9.f).withExtraKerningFactor(0.07f));
        g.drawText(label_.toUpperCase(), getLocalBounds().removeFromBottom(18).reduced(4, 0),
                   juce::Justification::centred, false);
    }

private:
    juce::String label_, value_;
    juce::Colour accent_;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StatCard)
};

// ─────────────────────────────────────────────────────────────────────────────
// TransportButton
// Round play / stop / back icon button for the toolbar
// ─────────────────────────────────────────────────────────────────────────────
class TransportButton : public juce::Button
{
public:
    enum Icon { Play, Stop, Back, Forward };

    TransportButton(Icon icon, bool isPrimary = false)
        : juce::Button("transport"), icon_(icon), isPrimary_(isPrimary) {}

    void paintButton(juce::Graphics& g, bool highlighted, bool down) override
    {
        using namespace VCTheme;
        auto b = getLocalBounds().toFloat().reduced(1.f);
        const float cx = b.getCentreX(), cy = b.getCentreY();
        const float r  = b.getWidth() * 0.5f;

        // Background circle
        juce::Colour bg = isPrimary_ ? c(ACCENT_BLUE) : c(BG_SURFACE);
        if (down)        bg = isPrimary_ ? c(ACCENT_BLUE2) : c(BG_ACTIVE);
        else if (highlighted) bg = isPrimary_ ? c(ACCENT_BLUE).brighter(0.1f) : c(BG_HOVER);

        g.setColour(bg);
        g.fillEllipse(b);
        g.setColour(isPrimary_ ? c(ACCENT_BLUE).darker(0.2f) : c(BORDER_MID));
        g.drawEllipse(b, 0.5f);

        // Icon
        juce::Colour iconCol = isPrimary_ ? juce::Colours::white
                                          : (highlighted ? c(TEXT_PRIMARY) : c(TEXT_SECONDARY));
        g.setColour(iconCol);

        const float s = r * 0.4f;
        juce::Path path;

        switch (icon_)
        {
            case Play:
                path.addTriangle(cx - s * 0.7f, cy - s, cx - s * 0.7f, cy + s, cx + s, cy);
                g.fillPath(path);
                break;

            case Stop:
                g.fillRect(cx - s * 0.75f, cy - s * 0.75f, s * 1.5f, s * 1.5f);
                break;

            case Back:
                path.addTriangle(cx + s * 0.5f, cy - s, cx + s * 0.5f, cy + s, cx - s * 0.3f, cy);
                g.fillPath(path);
                g.fillRect(cx - s * 0.85f, cy - s, s * 0.3f, s * 2.f);
                break;

            case Forward:
                path.addTriangle(cx - s * 0.5f, cy - s, cx - s * 0.5f, cy + s, cx + s * 0.3f, cy);
                g.fillPath(path);
                g.fillRect(cx + s * 0.55f, cy - s, s * 0.3f, s * 2.f);
                break;
        }
    }

private:
    Icon icon_;
    bool isPrimary_;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TransportButton)
};

// ─────────────────────────────────────────────────────────────────────────────
// PresetBar
// Plugin-wide preset selector with prev/next arrows and a name display.
// ─────────────────────────────────────────────────────────────────────────────
class PresetBar : public juce::Component
{
public:
    // Public so editor can attach onClick callbacks
    juce::TextButton prevBtn_ { "<" };
    juce::TextButton nextBtn_ { ">" };
    juce::TextButton saveBtn_ { "Save" };
    juce::TextButton loadBtn_ { "Load" };

    PresetBar()
    {
        addAndMakeVisible(&prevBtn_);
        addAndMakeVisible(&nextBtn_);
        addAndMakeVisible(&saveBtn_);
        addAndMakeVisible(&loadBtn_);

        addAndMakeVisible(nameLabel_);
        nameLabel_.setJustificationType(juce::Justification::centred);
        nameLabel_.setFont(juce::Font(11.f, juce::Font::bold));
        nameLabel_.setColour(juce::Label::textColourId, VCTheme::c(VCTheme::TEXT_PRIMARY));
        nameLabel_.setText("Default", juce::dontSendNotification);
    }

    void setPresetName(const juce::String& name) {
        nameLabel_.setText(name, juce::dontSendNotification);
    }

    void paint(juce::Graphics& g) override
    {
        using namespace VCTheme;
        // Subtle background pill around the name area
        auto nb = nameLabel_.getBounds().expanded(4, 2).toFloat();
        g.setColour(c(BG_SURFACE));
        g.fillRoundedRectangle(nb, RADIUS_SM);
        g.setColour(c(BORDER_MID));
        g.drawRoundedRectangle(nb, RADIUS_SM, 0.5f);
    }

    void resized() override
    {
        auto b = getLocalBounds();
        prevBtn_.setBounds(b.removeFromLeft(22).reduced(1, 3));
        nextBtn_.setBounds(b.removeFromLeft(22).reduced(1, 3));
        b.removeFromLeft(4);
        saveBtn_.setBounds(b.removeFromRight(42).reduced(1, 3));
        loadBtn_.setBounds(b.removeFromRight(42).reduced(1, 3));
        b.removeFromRight(4);
        nameLabel_.setBounds(b.reduced(2, 3));
    }

private:
    juce::Label nameLabel_;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetBar)
};

// ─────────────────────────────────────────────────────────────────────────────
// BpmDisplay
// Clickable BPM readout with up/down arrows. Syncs from host automatically.
// ─────────────────────────────────────────────────────────────────────────────
class BpmDisplay : public juce::Component, public juce::SettableTooltipClient
{
public:
    BpmDisplay()
    {
        upBtn_.setButtonText("+");
        downBtn_.setButtonText("-");
        addAndMakeVisible(upBtn_);
        addAndMakeVisible(downBtn_);

        upBtn_.onClick   = [this] { setBpm(bpm_ + 1.f); };
        downBtn_.onClick = [this] { setBpm(bpm_ - 1.f); };
    }

    void setBpm(float bpm)
    {
        bpm_ = juce::jlimit(40.f, 300.f, bpm);
        repaint();
        if (onBpmChanged) onBpmChanged(bpm_);
    }

    float getBpm() const { return bpm_; }

    std::function<void(float)> onBpmChanged;

    void paint(juce::Graphics& g) override
    {
        using namespace VCTheme;
        auto b = getLocalBounds().toFloat().reduced(0.5f);

        g.setColour(c(BG_SURFACE));
        g.fillRoundedRectangle(b, RADIUS_SM);
        g.setColour(c(BORDER_MID));
        g.drawRoundedRectangle(b, RADIUS_SM, 0.5f);

        // BPM value
        g.setColour(c(TEXT_PRIMARY));
        g.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 12.f, juce::Font::bold));
        g.drawText(juce::String((int)bpm_) + " BPM",
                   upBtn_.getRight(), 0, getWidth() - upBtn_.getRight() - downBtn_.getWidth(), getHeight(),
                   juce::Justification::centred, false);
    }

    void resized() override
    {
        auto b = getLocalBounds();
        downBtn_.setBounds(b.removeFromLeft(18).reduced(1, 4));
        upBtn_.setBounds  (b.removeFromRight(18).reduced(1, 4));
    }

private:
    juce::TextButton upBtn_, downBtn_;
    float bpm_ = 140.f;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BpmDisplay)
};

// ─────────────────────────────────────────────────────────────────────────────
// LivePitchDisplay
// Animated pitch readout — note name + Hz, colour-coded by confidence.
// ─────────────────────────────────────────────────────────────────────────────
class LivePitchDisplay : public juce::Component,
                          public juce::SettableTooltipClient,
                          private juce::Timer
{
public:
    LivePitchDisplay()
    {
        startTimerHz(10);
    }

    void setPitch(const juce::String& noteName, float hz, float confidence)
    {
        noteName_   = noteName;
        hz_         = hz;
        confidence_ = confidence;
        voiced_     = (confidence > 0.65f);
        repaint();
    }

    void setInactive() { voiced_ = false; repaint(); }

    void paint(juce::Graphics& g) override
    {
        using namespace VCTheme;
        auto b = getLocalBounds().toFloat().reduced(0.5f);

        juce::Colour bg    = voiced_ ? c(ACCENT_GREEN).withAlpha(0.1f) : c(BG_SURFACE);
        juce::Colour border= voiced_ ? c(ACCENT_GREEN).withAlpha(0.4f) : c(BORDER_MID);

        g.setColour(bg);
        g.fillRoundedRectangle(b, RADIUS_SM);
        g.setColour(border);
        g.drawRoundedRectangle(b, RADIUS_SM, 0.5f);

        if (voiced_)
        {
            g.setColour(c(ACCENT_GREEN));
            g.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 12.f, juce::Font::bold));
            g.drawText(noteName_ + "  " + juce::String((int)hz_) + " Hz",
                       getLocalBounds().reduced(8, 0), juce::Justification::centredLeft, false);
        }
        else
        {
            g.setColour(c(TEXT_MUTED));
            g.setFont(juce::Font(12.f));
            g.drawText("—", getLocalBounds().reduced(8, 0), juce::Justification::centredLeft, false);
        }
    }

    void timerCallback() override { repaint(); }

private:
    juce::String noteName_;
    float hz_         = 0.f;
    float confidence_ = 0.f;
    bool  voiced_     = false;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LivePitchDisplay)
};
