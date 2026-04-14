#pragma once
#include <JuceHeader.h>

// ─────────────────────────────────────────────────────────────────────────────
// Design tokens — single source of truth for every colour and size used
// throughout the plugin. Change here, affects everything.
// ─────────────────────────────────────────────────────────────────────────────
namespace VCTheme
{
    // ── Backgrounds ──────────────────────────────────────────────────────────
    inline constexpr juce::uint32 BG_BASE    = 0xff0e0f12;   // darkest — window
    inline constexpr juce::uint32 BG_RAISED  = 0xff161820;   // panels / sections
    inline constexpr juce::uint32 BG_SURFACE = 0xff1e2130;   // controls, inputs
    inline constexpr juce::uint32 BG_HOVER   = 0xff252840;   // hover state
    inline constexpr juce::uint32 BG_ACTIVE  = 0xff2d3050;   // pressed / active

    // ── Borders ───────────────────────────────────────────────────────────
    inline constexpr juce::uint32 BORDER_SUBTLE = 0xff1f2235;
    inline constexpr juce::uint32 BORDER_MID    = 0xff2a2f47;
    inline constexpr juce::uint32 BORDER_STRONG = 0xff3a4060;

    // ── Accent palette ────────────────────────────────────────────────────
    inline constexpr juce::uint32 ACCENT_BLUE   = 0xff4d9fff;   // primary
    inline constexpr juce::uint32 ACCENT_BLUE2  = 0xff2d7de8;   // hover/pressed
    inline constexpr juce::uint32 ACCENT_GREEN  = 0xff3dd68c;   // live / success
    inline constexpr juce::uint32 ACCENT_AMBER  = 0xfff5a623;   // warning / gate
    inline constexpr juce::uint32 ACCENT_RED    = 0xffe84855;   // danger / noise
    inline constexpr juce::uint32 ACCENT_PURPLE = 0xffa78bfa;   // pitch / spectrum
    inline constexpr juce::uint32 ACCENT_TEAL   = 0xff34d8c2;   // stretch

    // ── Text hierarchy ────────────────────────────────────────────────────
    inline constexpr juce::uint32 TEXT_PRIMARY   = 0xffe8eaf2;
    inline constexpr juce::uint32 TEXT_SECONDARY = 0xff9ea5bc;
    inline constexpr juce::uint32 TEXT_MUTED     = 0xff5a6278;
    inline constexpr juce::uint32 TEXT_DISABLED  = 0xff363d52;

    // ── Sizes ─────────────────────────────────────────────────────────────
    inline constexpr float RADIUS_SM  = 3.f;
    inline constexpr float RADIUS_MD  = 5.f;
    inline constexpr float RADIUS_LG  = 8.f;
    inline constexpr float RADIUS_XL  = 12.f;

    // ── Region accent colours (per-chop region) ───────────────────────────
    static const juce::Colour REGION_COLOURS[8] = {
        juce::Colour(0xff4d9fff),   // blue
        juce::Colour(0xff3dd68c),   // green
        juce::Colour(0xfff5a623),   // amber
        juce::Colour(0xffe84855),   // red
        juce::Colour(0xffa78bfa),   // purple
        juce::Colour(0xff34d8c2),   // teal
        juce::Colour(0xfffb923c),   // orange
        juce::Colour(0xff60a5fa),   // light blue
    };

    // ── Helpers ───────────────────────────────────────────────────────────
    static inline juce::Colour c(juce::uint32 hex) { return juce::Colour(hex); }
}

// ─────────────────────────────────────────────────────────────────────────────
// VocalChopperLAF — overrides every standard JUCE control to match the theme
// ─────────────────────────────────────────────────────────────────────────────
class VocalChopperLAF : public juce::LookAndFeel_V4
{
public:
    VocalChopperLAF()
    {
        using namespace VCTheme;

        // Window
        setColour(juce::ResizableWindow::backgroundColourId, c(BG_BASE));
        setColour(juce::DocumentWindow::backgroundColourId,  c(BG_BASE));

        // TextButton — used for toolbar buttons and section actions
        setColour(juce::TextButton::buttonColourId,    c(BG_SURFACE));
        setColour(juce::TextButton::buttonOnColourId,  c(ACCENT_BLUE));
        setColour(juce::TextButton::textColourOffId,   c(TEXT_SECONDARY));
        setColour(juce::TextButton::textColourOnId,    c(TEXT_PRIMARY));

        // ToggleButton
        setColour(juce::ToggleButton::tickColourId,           c(ACCENT_BLUE));
        setColour(juce::ToggleButton::tickDisabledColourId,   c(TEXT_DISABLED));
        setColour(juce::ToggleButton::textColourId,           c(TEXT_SECONDARY));

        // Label
        setColour(juce::Label::textColourId,             c(TEXT_PRIMARY));
        setColour(juce::Label::outlineColourId,          juce::Colours::transparentBlack);
        setColour(juce::Label::backgroundColourId,       juce::Colours::transparentBlack);
        setColour(juce::Label::backgroundWhenEditingColourId, c(BG_SURFACE));
        setColour(juce::Label::textWhenEditingColourId,       c(TEXT_PRIMARY));
        setColour(juce::Label::outlineWhenEditingColourId,    c(ACCENT_BLUE));

        // Slider
        setColour(juce::Slider::thumbColourId,              c(TEXT_PRIMARY));
        setColour(juce::Slider::trackColourId,              c(ACCENT_BLUE));
        setColour(juce::Slider::backgroundColourId,         c(BG_SURFACE));
        setColour(juce::Slider::rotarySliderFillColourId,   c(ACCENT_BLUE));
        setColour(juce::Slider::rotarySliderOutlineColourId,c(BG_SURFACE));
        setColour(juce::Slider::textBoxTextColourId,        c(TEXT_PRIMARY));
        setColour(juce::Slider::textBoxBackgroundColourId,  c(BG_SURFACE));
        setColour(juce::Slider::textBoxOutlineColourId,     c(BORDER_MID));
        setColour(juce::Slider::textBoxHighlightColourId,   c(ACCENT_BLUE).withAlpha(0.35f));

        // ComboBox
        setColour(juce::ComboBox::backgroundColourId,       c(BG_SURFACE));
        setColour(juce::ComboBox::textColourId,             c(TEXT_PRIMARY));
        setColour(juce::ComboBox::outlineColourId,          c(BORDER_MID));
        setColour(juce::ComboBox::buttonColourId,           c(BG_HOVER));
        setColour(juce::ComboBox::arrowColourId,            c(TEXT_SECONDARY));
        setColour(juce::ComboBox::focusedOutlineColourId,   c(ACCENT_BLUE));

        // TextEditor
        setColour(juce::TextEditor::backgroundColourId,       c(BG_SURFACE));
        setColour(juce::TextEditor::textColourId,             c(TEXT_SECONDARY));
        setColour(juce::TextEditor::highlightColourId,        c(ACCENT_BLUE).withAlpha(0.3f));
        setColour(juce::TextEditor::highlightedTextColourId,  c(TEXT_PRIMARY));
        setColour(juce::TextEditor::outlineColourId,          c(BORDER_MID));
        setColour(juce::TextEditor::focusedOutlineColourId,   c(ACCENT_BLUE));

        // PopupMenu
        setColour(juce::PopupMenu::backgroundColourId,             c(BG_RAISED));
        setColour(juce::PopupMenu::textColourId,                   c(TEXT_PRIMARY));
        setColour(juce::PopupMenu::headerTextColourId,             c(TEXT_MUTED));
        setColour(juce::PopupMenu::highlightedBackgroundColourId,  c(ACCENT_BLUE));
        setColour(juce::PopupMenu::highlightedTextColourId,        juce::Colours::white);

        // TabbedComponent
        setColour(juce::TabbedButtonBar::tabTextColourId,     c(TEXT_MUTED));
        setColour(juce::TabbedButtonBar::frontTextColourId,   c(TEXT_PRIMARY));
        setColour(juce::TabbedButtonBar::tabOutlineColourId,  c(BORDER_SUBTLE));

        // ScrollBar
        setColour(juce::ScrollBar::backgroundColourId, juce::Colours::transparentBlack);
        setColour(juce::ScrollBar::thumbColourId,      c(BORDER_STRONG));
        setColour(juce::ScrollBar::trackColourId,      c(BG_SURFACE));

        // ProgressBar
        setColour(juce::ProgressBar::backgroundColourId, c(BG_SURFACE));
        setColour(juce::ProgressBar::foregroundColourId, c(ACCENT_BLUE));

        // AlertWindow
        setColour(juce::AlertWindow::backgroundColourId, c(BG_RAISED));
        setColour(juce::AlertWindow::textColourId,       c(TEXT_PRIMARY));
        setColour(juce::AlertWindow::outlineColourId,    c(BORDER_MID));
    }

    // ── Fonts ─────────────────────────────────────────────────────────────

    juce::Font getTextButtonFont(juce::TextButton&, int /*h*/) override
    {
        return juce::Font(11.f).withExtraKerningFactor(0.06f);
    }

    juce::Font getLabelFont(juce::Label& label) override
    {
        // Section headers: we detect them by a custom property set in UIComponents
        if (label.getProperties().contains("sectionHeader"))
            return juce::Font(9.f, juce::Font::plain).withExtraKerningFactor(0.10f);
        if (label.getProperties().contains("valueDisplay"))
            return juce::Font(juce::Font::getDefaultMonospacedFontName(), 11.f, juce::Font::bold);
        return juce::Font(11.f);
    }

    juce::Font getComboBoxFont(juce::ComboBox&) override
    {
        return juce::Font(11.f);
    }

    juce::Font getPopupMenuFont() override
    {
        return juce::Font(12.f);
    }

    juce::Font getSliderPopupFont(juce::Slider&) override
    {
        return juce::Font(juce::Font::getDefaultMonospacedFontName(), 11.f, juce::Font::bold);
    }

    // ── Buttons ───────────────────────────────────────────────────────────

    void drawButtonBackground(juce::Graphics& g,
                               juce::Button& btn,
                               const juce::Colour& /*bg*/,
                               bool highlighted,
                               bool down) override
    {
        using namespace VCTheme;
        auto bounds = btn.getLocalBounds().toFloat().reduced(0.5f);

        juce::Colour fill, border;

        if (btn.getToggleState())
        {
            fill   = c(ACCENT_BLUE).withAlpha(down ? 1.f : highlighted ? 0.9f : 0.85f);
            border = c(ACCENT_BLUE);
        }
        else if (down)
        {
            fill   = c(BG_ACTIVE);
            border = c(BORDER_STRONG);
        }
        else if (highlighted)
        {
            fill   = c(BG_HOVER);
            border = c(BORDER_STRONG);
        }
        else
        {
            fill   = c(BG_SURFACE);
            border = c(BORDER_MID);
        }

        // Danger/destructive buttons: red tint
        if (btn.getProperties().contains("danger"))
        {
            fill   = down ? c(ACCENT_RED).withAlpha(0.25f) : c(ACCENT_RED).withAlpha(highlighted ? 0.15f : 0.08f);
            border = c(ACCENT_RED).withAlpha(0.5f);
        }

        // Primary/CTA buttons
        if (btn.getProperties().contains("primary"))
        {
            fill   = down ? c(ACCENT_BLUE2) : highlighted ? c(ACCENT_BLUE).brighter(0.1f) : c(ACCENT_BLUE);
            border = c(ACCENT_BLUE);
        }

        g.setColour(fill);
        g.fillRoundedRectangle(bounds, RADIUS_SM);
        g.setColour(border);
        g.drawRoundedRectangle(bounds, RADIUS_SM, 0.5f);
    }

    void drawButtonText(juce::Graphics& g, juce::TextButton& btn,
                         bool highlighted, bool /*down*/) override
    {
        using namespace VCTheme;
        juce::Colour col;

        if (btn.getToggleState() || btn.getProperties().contains("primary"))
            col = juce::Colours::white;
        else if (!btn.isEnabled())
            col = c(TEXT_DISABLED);
        else if (highlighted)
            col = c(TEXT_PRIMARY);
        else
            col = c(TEXT_SECONDARY);

        if (btn.getProperties().contains("danger"))
            col = c(ACCENT_RED).withAlpha(!btn.isEnabled() ? 0.35f : 1.f);

        g.setColour(col);
        g.setFont(getTextButtonFont(btn, btn.getHeight()));
        g.drawText(btn.getButtonText().toUpperCase(),
                   btn.getLocalBounds(),
                   juce::Justification::centred, false);
    }

    // ── Toggle button (custom checkbox) ──────────────────────────────────

    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& btn,
                           bool highlighted, bool /*down*/) override
    {
        using namespace VCTheme;
        const float h   = (float)btn.getHeight();
        const float boxW = h * 0.85f;
        const float boxH = h * 0.85f;
        const float boxX = 0.f;
        const float boxY = (h - boxH) * 0.5f;

        // Box background
        juce::Colour boxFill = btn.getToggleState()
            ? c(ACCENT_BLUE)
            : (highlighted ? c(BG_HOVER) : c(BG_SURFACE));
        juce::Colour boxBorder = btn.getToggleState()
            ? c(ACCENT_BLUE)
            : c(BORDER_MID);

        g.setColour(boxFill);
        g.fillRoundedRectangle(boxX, boxY, boxW, boxH, RADIUS_SM - 1.f);
        g.setColour(boxBorder);
        g.drawRoundedRectangle(boxX + 0.5f, boxY + 0.5f, boxW - 1.f, boxH - 1.f, RADIUS_SM - 1.f, 0.5f);

        // Tick mark
        if (btn.getToggleState())
        {
            g.setColour(juce::Colours::white);
            const float cx = boxX + boxW * 0.5f;
            const float cy = boxY + boxH * 0.5f;
            juce::Path tick;
            tick.startNewSubPath(cx - boxW * 0.25f, cy);
            tick.lineTo(cx - boxW * 0.08f, cy + boxH * 0.22f);
            tick.lineTo(cx + boxW * 0.26f, cy - boxH * 0.22f);
            g.strokePath(tick, juce::PathStrokeType(1.5f,
                juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        // Text
        g.setColour(btn.isEnabled() ? c(TEXT_SECONDARY) : c(TEXT_DISABLED));
        g.setFont(juce::Font(11.f));
        g.drawText(btn.getButtonText(),
                   (int)(boxW + 6.f), 0, btn.getWidth() - (int)(boxW + 6.f), btn.getHeight(),
                   juce::Justification::centredLeft, true);
    }

    // ── Sliders ───────────────────────────────────────────────────────────

    void drawLinearSlider(juce::Graphics& g, int x, int y, int w, int h,
                           float sliderPos, float minPos, float maxPos,
                           juce::Slider::SliderStyle style,
                           juce::Slider& slider) override
    {
        using namespace VCTheme;

        if (style != juce::Slider::LinearHorizontal &&
            style != juce::Slider::LinearVertical)
        {
            LookAndFeel_V4::drawLinearSlider(g, x, y, w, h, sliderPos,
                                              minPos, maxPos, style, slider);
            return;
        }

        const bool isHoriz = (style == juce::Slider::LinearHorizontal);
        const float trackThick = 3.f;
        const float thumbR = 6.f;

        // Determine accent color from slider property (for per-region sliders)
        juce::Colour accent = c(ACCENT_BLUE);
        if (slider.getProperties().contains("accentColour"))
            accent = juce::Colour((juce::uint32)(int)slider.getProperties()["accentColour"]);

        if (isHoriz)
        {
            const float trackY  = y + h * 0.5f - trackThick * 0.5f;
            const float trackX1 = (float)x + thumbR;
            const float trackX2 = (float)(x + w) - thumbR;

            // Track background
            g.setColour(c(BG_SURFACE));
            g.fillRoundedRectangle(trackX1, trackY, trackX2 - trackX1, trackThick, trackThick * 0.5f);

            // Filled portion
            float fillEnd = juce::jlimit(trackX1, trackX2, sliderPos);
            g.setColour(accent.withAlpha(slider.isEnabled() ? 1.f : 0.35f));
            g.fillRoundedRectangle(trackX1, trackY, fillEnd - trackX1, trackThick, trackThick * 0.5f);

            // Track groove lines (subtle texture)
            g.setColour(c(BORDER_SUBTLE));
            g.drawLine(trackX1, trackY + trackThick, trackX2, trackY + trackThick, 0.5f);

            // Thumb
            const float cx = juce::jlimit(trackX1, trackX2, sliderPos);
            const float cy = y + h * 0.5f;
            g.setColour(slider.isEnabled() ? c(TEXT_PRIMARY) : c(TEXT_DISABLED));
            g.fillEllipse(cx - thumbR, cy - thumbR, thumbR * 2.f, thumbR * 2.f);
            g.setColour(accent.withAlpha(slider.isEnabled() ? 0.8f : 0.3f));
            g.drawEllipse(cx - thumbR + 0.5f, cy - thumbR + 0.5f,
                          thumbR * 2.f - 1.f, thumbR * 2.f - 1.f, 1.f);

            // Inner dot
            g.setColour(accent);
            g.fillEllipse(cx - 2.f, cy - 2.f, 4.f, 4.f);
        }
        else
        {
            // Vertical
            const float trackX   = x + w * 0.5f - trackThick * 0.5f;
            const float trackY1  = (float)y + thumbR;
            const float trackY2  = (float)(y + h) - thumbR;

            g.setColour(c(BG_SURFACE));
            g.fillRoundedRectangle(trackX, trackY1, trackThick, trackY2 - trackY1, trackThick * 0.5f);

            float fillStart = juce::jlimit(trackY1, trackY2, sliderPos);
            g.setColour(accent);
            g.fillRoundedRectangle(trackX, fillStart, trackThick, trackY2 - fillStart, trackThick * 0.5f);

            const float cx = x + w * 0.5f;
            const float cy = juce::jlimit(trackY1, trackY2, sliderPos);
            g.setColour(c(TEXT_PRIMARY));
            g.fillEllipse(cx - thumbR, cy - thumbR, thumbR * 2.f, thumbR * 2.f);
            g.setColour(accent.withAlpha(0.8f));
            g.drawEllipse(cx - thumbR + 0.5f, cy - thumbR + 0.5f,
                          thumbR * 2.f - 1.f, thumbR * 2.f - 1.f, 1.f);
            g.setColour(accent);
            g.fillEllipse(cx - 2.f, cy - 2.f, 4.f, 4.f);
        }
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float startAngle, float endAngle,
                           juce::Slider& slider) override
    {
        using namespace VCTheme;

        juce::Colour accent = c(ACCENT_BLUE);
        if (slider.getProperties().contains("accentColour"))
            accent = juce::Colour((juce::uint32)(int)slider.getProperties()["accentColour"]);

        const float centreX = x + width  * 0.5f;
        const float centreY = y + height * 0.5f;
        const float rx      = width  * 0.5f - 4.f;
        const float ry      = height * 0.5f - 4.f;
        const float radius  = juce::jmin(rx, ry);

        // Outer shadow ring
        g.setColour(c(BG_BASE));
        g.fillEllipse(centreX - radius - 2.f, centreY - radius - 2.f,
                      (radius + 2.f) * 2.f, (radius + 2.f) * 2.f);

        // Body
        g.setColour(c(BG_SURFACE));
        g.fillEllipse(centreX - radius, centreY - radius, radius * 2.f, radius * 2.f);

        // Groove arc
        const float arcThick = 3.5f;
        juce::Path groove;
        groove.addCentredArc(centreX, centreY, radius - arcThick * 0.5f,
                              radius - arcThick * 0.5f,
                              0.f, startAngle, endAngle, true);
        g.setColour(c(BORDER_MID));
        g.strokePath(groove, juce::PathStrokeType(arcThick,
            juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Value arc
        const float angle = startAngle + sliderPos * (endAngle - startAngle);
        juce::Path arc;
        arc.addCentredArc(centreX, centreY, radius - arcThick * 0.5f,
                           radius - arcThick * 0.5f,
                           0.f, startAngle, angle, true);
        g.setColour(accent.withAlpha(slider.isEnabled() ? 1.f : 0.4f));
        g.strokePath(arc, juce::PathStrokeType(arcThick,
            juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Tick mark (pointer line)
        const float pointerLen = radius * 0.55f;
        const float pointerThick = 2.f;
        juce::Path pointer;
        pointer.addRectangle(-pointerThick * 0.5f, -(radius - 6.f), pointerThick, pointerLen);
        pointer.applyTransform(juce::AffineTransform::rotation(angle)
                                .translated(centreX, centreY));
        g.setColour(slider.isEnabled() ? c(TEXT_PRIMARY) : c(TEXT_DISABLED));
        g.fillPath(pointer);

        // Centre dot
        g.setColour(c(BG_BASE));
        g.fillEllipse(centreX - 4.f, centreY - 4.f, 8.f, 8.f);
        g.setColour(accent);
        g.fillEllipse(centreX - 2.5f, centreY - 2.5f, 5.f, 5.f);

        // Rim border
        g.setColour(c(BORDER_MID));
        g.drawEllipse(centreX - radius + 0.5f, centreY - radius + 0.5f,
                      radius * 2.f - 1.f, radius * 2.f - 1.f, 0.5f);
    }

    int getSliderThumbRadius(juce::Slider& s) override
    {
        return s.isHorizontal() || s.isVertical() ? 6 : 8;
    }

    juce::Label* createSliderTextBox(juce::Slider& s) override
    {
        auto* label = LookAndFeel_V4::createSliderTextBox(s);
        if (label)
        {
            label->setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 11.f, juce::Font::bold));
            label->setColour(juce::Label::textColourId, VCTheme::c(VCTheme::TEXT_SECONDARY));
        }
        return label;
    }

    // ── ComboBox ──────────────────────────────────────────────────────────

    void drawComboBox(juce::Graphics& g, int width, int height, bool /*isButtonDown*/,
                       int, int, int, int, juce::ComboBox& box) override
    {
        using namespace VCTheme;
        auto b = juce::Rectangle<float>(0.f, 0.f, (float)width, (float)height).reduced(0.5f);

        g.setColour(c(BG_SURFACE));
        g.fillRoundedRectangle(b, RADIUS_SM);
        g.setColour(box.hasKeyboardFocus(true) ? c(ACCENT_BLUE) : c(BORDER_MID));
        g.drawRoundedRectangle(b, RADIUS_SM, 0.5f);

        // Arrow
        const float arrowH = height * 0.22f;
        const float arrowX = width - 16.f;
        const float arrowY = height * 0.5f - arrowH * 0.5f;
        juce::Path arrow;
        arrow.addTriangle(arrowX, arrowY, arrowX + 8.f, arrowY, arrowX + 4.f, arrowY + arrowH);
        g.setColour(c(TEXT_MUTED));
        g.fillPath(arrow);
    }

    void positionComboBoxText(juce::ComboBox& box, juce::Label& label) override
    {
        label.setBounds(8, 0, box.getWidth() - 24, box.getHeight());
        label.setFont(juce::Font(11.f));
        label.setColour(juce::Label::textColourId, VCTheme::c(VCTheme::TEXT_PRIMARY));
    }

    // ── PopupMenu ─────────────────────────────────────────────────────────

    void drawPopupMenuBackground(juce::Graphics& g, int w, int h) override
    {
        using namespace VCTheme;
        g.setColour(c(BG_RAISED));
        g.fillRoundedRectangle(0.f, 0.f, (float)w, (float)h, RADIUS_MD);
        g.setColour(c(BORDER_MID));
        g.drawRoundedRectangle(0.5f, 0.5f, (float)w - 1.f, (float)h - 1.f, RADIUS_MD, 0.5f);
    }

    void drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
                            bool isSeparator, bool isActive, bool isHighlighted,
                            bool isTicked, bool hasSubMenu,
                            const juce::String& text,
                            const juce::String& shortcutKeyText,
                            const juce::Drawable* icon,
                            const juce::Colour* textColour) override
    {
        using namespace VCTheme;

        if (isSeparator)
        {
            g.setColour(c(BORDER_MID));
            g.fillRect(area.reduced(8, 0).withHeight(1));
            return;
        }

        if (isHighlighted && isActive)
        {
            g.setColour(c(ACCENT_BLUE));
            g.fillRoundedRectangle(area.reduced(4, 2).toFloat(), RADIUS_SM - 1.f);
        }

        juce::Colour col = isHighlighted && isActive ? juce::Colours::white
                         : !isActive ? c(TEXT_DISABLED)
                         : (textColour ? *textColour : c(TEXT_PRIMARY));

        g.setColour(col);
        g.setFont(juce::Font(12.f));

        auto textArea = area.reduced(12, 0);
        if (isTicked)
        {
            g.setColour(isHighlighted ? juce::Colours::white : c(ACCENT_BLUE));
            g.fillEllipse(area.getX() + 4.f, area.getCentreY() - 3.f, 6.f, 6.f);
            textArea = textArea.withLeft(textArea.getX() + 4);
        }
        g.setColour(col);
        g.drawText(text, textArea, juce::Justification::centredLeft);

        if (shortcutKeyText.isNotEmpty())
        {
            g.setFont(juce::Font(10.f));
            g.setColour(isHighlighted ? juce::Colours::white.withAlpha(0.6f) : c(TEXT_MUTED));
            g.drawText(shortcutKeyText, area.reduced(12, 0), juce::Justification::centredRight);
        }

        juce::ignoreUnused(hasSubMenu, icon);
    }

    // ── Tabs ─────────────────────────────────────────────────────────────

    void drawTabButton(juce::TabBarButton& btn, juce::Graphics& g,
                        bool isMouseOver, bool isMouseDown) override
    {
        using namespace VCTheme;
        auto b = btn.getLocalBounds().toFloat();
        const bool isActive = btn.isFrontTab();

        // Background
        g.setColour(isActive ? c(BG_BASE) : juce::Colours::transparentBlack);
        if (isActive) g.fillRect(b);

        // Bottom border highlight for active tab
        if (isActive)
        {
            g.setColour(c(ACCENT_BLUE));
            g.fillRect(b.removeFromBottom(2.f));
        }

        // Text
        juce::Colour textCol = isActive   ? c(TEXT_PRIMARY)
                             : isMouseOver? c(TEXT_SECONDARY)
                             :              c(TEXT_MUTED);
        g.setColour(textCol);
        g.setFont(juce::Font(10.f, juce::Font::plain).withExtraKerningFactor(0.08f));
        g.drawText(btn.getButtonText().toUpperCase(),
                   btn.getLocalBounds().reduced(2, 0),
                   juce::Justification::centred, false);

        juce::ignoreUnused(isMouseDown);
    }

    void drawTabAreaBehindFrontButton(juce::TabbedButtonBar& bar,
                                       juce::Graphics& g,
                                       int w, int h) override
    {
        using namespace VCTheme;
        g.setColour(c(BORDER_SUBTLE));
        if (bar.getOrientation() == juce::TabbedButtonBar::TabsAtTop)
            g.fillRect(0, h - 1, w, 1);
        else
            g.fillRect(0, 0, w, 1);
    }

    int getTabButtonBestWidth(juce::TabBarButton&, int) override { return 80; }

    // ── ScrollBar ─────────────────────────────────────────────────────────

    void drawScrollbar(juce::Graphics& g, juce::ScrollBar& bar,
                        int x, int y, int width, int height,
                        bool isScrollbarVertical,
                        int thumbStartPosition, int thumbSize,
                        bool isMouseOver, bool isMouseDown) override
    {
        using namespace VCTheme;
        juce::Rectangle<float> thumb;
        const float margin = 2.f;
        if (isScrollbarVertical)
            thumb = { (float)x + margin, (float)(y + thumbStartPosition) + margin,
                      (float)width - margin * 2.f, (float)thumbSize - margin * 2.f };
        else
            thumb = { (float)(x + thumbStartPosition) + margin, (float)y + margin,
                      (float)thumbSize - margin * 2.f, (float)height - margin * 2.f };

        float alpha = isMouseDown ? 0.6f : isMouseOver ? 0.45f : 0.3f;
        g.setColour(c(BORDER_STRONG).withAlpha(alpha));
        g.fillRoundedRectangle(thumb, 3.f);

        juce::ignoreUnused(bar);
    }

    // ── ProgressBar ───────────────────────────────────────────────────────

    void drawProgressBar(juce::Graphics& g, juce::ProgressBar& bar,
                          int width, int height, double progress,
                          const juce::String& textToShow) override
    {
        using namespace VCTheme;
        auto b = bar.getLocalBounds().toFloat().reduced(0.5f);

        // Track
        g.setColour(c(BG_SURFACE));
        g.fillRoundedRectangle(b, RADIUS_SM);
        g.setColour(c(BORDER_MID));
        g.drawRoundedRectangle(b, RADIUS_SM, 0.5f);

        // Fill
        if (progress > 0.0)
        {
            auto fill = b.withWidth((float)(b.getWidth() * progress));
            g.setColour(c(ACCENT_BLUE));
            g.fillRoundedRectangle(fill, RADIUS_SM);
        }

        // Indeterminate — animated shimmer (we draw a stripe if progress < 0)
        if (progress < 0.0 || progress > 1.0)
        {
            g.setColour(c(ACCENT_BLUE).withAlpha(0.4f));
            g.fillRoundedRectangle(b.withWidth(b.getWidth() * 0.4f), RADIUS_SM);
        }

        if (textToShow.isNotEmpty())
        {
            g.setColour(c(TEXT_SECONDARY));
            g.setFont(juce::Font(10.f));
            g.drawText(textToShow, bar.getLocalBounds(), juce::Justification::centred);
        }

        juce::ignoreUnused(width, height);
    }

    // ── AlertWindow ───────────────────────────────────────────────────────

    juce::AlertWindow* createAlertWindow(const juce::String& title,
                                          const juce::String& message,
                                          const juce::String& btn1,
                                          const juce::String& btn2,
                                          const juce::String& btn3,
                                          juce::MessageBoxIconType iconType,
                                          int nButtons,
                                          juce::Component* assoc) override
    {
        auto* aw = LookAndFeel_V4::createAlertWindow(title, message, btn1, btn2, btn3,
                                                       iconType, nButtons, assoc);
        return aw;
    }

    void drawAlertBox(juce::Graphics& g, juce::AlertWindow& aw,
                       const juce::Rectangle<int>& textArea,
                       juce::TextLayout& textLayout) override
    {
        using namespace VCTheme;
        g.setColour(c(BG_RAISED));
        g.fillRoundedRectangle(aw.getLocalBounds().toFloat(), RADIUS_LG);
        g.setColour(c(BORDER_MID));
        g.drawRoundedRectangle(aw.getLocalBounds().toFloat().reduced(0.5f), RADIUS_LG, 0.5f);
        textLayout.draw(g, textArea.toFloat());
    }

    // ── Tooltip ───────────────────────────────────────────────────────────

    juce::Rectangle<int> getTooltipBounds(const juce::String& text,
                                           juce::Point<int> pos,
                                           juce::Rectangle<int> parentArea) override
    {
        auto r = LookAndFeel_V4::getTooltipBounds(text, pos, parentArea);
        return r.translated(0, -6);
    }

    void drawTooltip(juce::Graphics& g, const juce::String& text, int w, int h) override
    {
        using namespace VCTheme;
        auto b = juce::Rectangle<float>(0.f, 0.f, (float)w, (float)h).reduced(0.5f);
        g.setColour(c(BG_RAISED));
        g.fillRoundedRectangle(b, RADIUS_SM);
        g.setColour(c(BORDER_STRONG));
        g.drawRoundedRectangle(b, RADIUS_SM, 0.5f);
        g.setColour(c(TEXT_SECONDARY));
        g.setFont(juce::Font(11.f));
        g.drawText(text, 8, 0, w - 16, h, juce::Justification::centredLeft);
    }

};
