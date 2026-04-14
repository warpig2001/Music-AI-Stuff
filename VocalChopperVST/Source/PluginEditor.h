#pragma once
#include <JuceHeader.h>
#include "UIComponents.h"
#include "PluginProcessor.h"
#include "WaveformDisplay.h"
#include "FadeDesigner.h"
#include "PadGrid.h"
#include "AlignmentTimeline.h"
#include "CleanTab.h"
#include "StretchTab.h"
#include "LookAndFeel.h"
#include "PresetManager.h"

// ─── Tab panel base: gives each tab a proper resized() virtual method ────────
class TabPanel : public juce::Component
{
public:
    TabPanel() = default;
    // Assign a layout lambda here — called automatically by resized()
    std::function<void()> layoutChildren;
    void resized() override { if (layoutChildren) layoutChildren(); }
private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TabPanel)
};

class VocalChopperEditor : public juce::AudioProcessorEditor,
                            public juce::Timer,
                            public juce::FileDragAndDropTarget,
                            public juce::KeyListener
{
public:
    explicit VocalChopperEditor(VocalChopperProcessor&);
    ~VocalChopperEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    bool isInterestedInFileDrag(const juce::StringArray&) override { return true; }
    void filesDropped(const juce::StringArray& files, int, int) override;

    bool keyPressed(const juce::KeyPress& key, juce::Component* origin) override;

private:
    VocalChopperProcessor& processor_;
    VocalChopperLAF laf_;

    // ── Toolbar ───────────────────────────────────────────────────────────
    VCToolbarButton      loadButton_  { "Load Vocal" };
    TransportButton    playButton_  { TransportButton::Play,    true };
    TransportButton    stopButton_  { TransportButton::Stop,    false };
    TransportButton    backButton_  { TransportButton::Back,    false };
    BpmDisplay         bpmDisplay_;
    LivePitchDisplay   pitchDisplay_;
    PresetBar          presetBar_;
    juce::TextButton   undoBtn_     { "\xe2\x86\xa9" };  // ↩ UTF-8
    juce::TextButton   redoBtn_     { "\xe2\x86\xaa" };  // ↪ UTF-8

    juce::ResizableCornerComponent resizer_ { this, &sizeConstraints_ };
    juce::ComponentBoundsConstrainer sizeConstraints_;

    juce::TabbedComponent tabs_;

    // ── Chop tab ──────────────────────────────────────────────────────────
    std::unique_ptr<TabPanel> chopTab_;
    WaveformDisplay*   waveform_       = nullptr;
    VCToolbarButton      autoChopBtn_   { "Auto transients" };
    VCToolbarButton      clearChopsBtn_ { "Clear" };
    VCToolbarButton      divideBtn_     { "Beat divide" };
    VCToolbarButton      pitchAnalBtn_  { "Analyze pitch" };
    juce::ComboBox     snapCombo_;
    juce::Label        chopInfoLabel_;
    juce::TextEditor   regionListBox_;
    VCToolbarButton      exportBtn_     { "Copy FL guide" };

    // ── Pad tab ───────────────────────────────────────────────────────────
    std::unique_ptr<TabPanel> padTab_;
    PadGrid*  padGrid_ = nullptr;
    juce::Label padInfoLabel_;
    juce::Label padMidiLabel_;

    // ── Fades tab ─────────────────────────────────────────────────────────
    std::unique_ptr<TabPanel>     fadeTab_;
    FadeDesignerComponent*        fadeInDesigner_  = nullptr;
    FadeDesignerComponent*        fadeOutDesigner_ = nullptr;
    juce::Slider                  fadeInSlider_;
    juce::Slider                  fadeOutSlider_;
    juce::Label                   fadeInLabel_  {{}, "Fade In"};
    juce::Label                   fadeOutLabel_ {{}, "Fade Out"};
    juce::ToggleButton            applyFadesBtn_ { "Apply fades on playback" };
    VCToolbarButton                 copyFadeBtn_   { "Copy FL fade values" };

    // ── Align tab ─────────────────────────────────────────────────────────
    std::unique_ptr<TabPanel> alignTab_;
    AlignmentTimeline* alignTimeline_  = nullptr;
    VCToolbarButton      runAnalysisBtn_ { "Analyze offset" };
    VCToolbarButton      nudgeLeftBtn_   { "< -10ms" };
    VCToolbarButton      nudgeRightBtn_  { "+10ms >" };
    VCToolbarButton      snapToGridBtn_  { "Snap to beat" };
    VCToolbarButton      copyAlignBtn_   { "Copy FL guide" };
    juce::TextEditor   alignOutput_;
    AlignmentResult    lastAlign_;

    // ── Clean + Stretch tabs (self-contained components) ──────────────────
    std::unique_ptr<CleanTab>   cleanTab_;
    std::unique_ptr<StretchTab> stretchTab_;

    // ── Parameter attachments ─────────────────────────────────────────────
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> fadeInAttach_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> fadeOutAttach_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> applyFadesAttach_;

    // ── CleanTab buffer pointer indirection ───────────────────────────────
    const juce::AudioBuffer<float>* cleanBufPtr_  = nullptr;
    double                          cleanSrValue_ = 44100.0;

    // ── Private helpers ───────────────────────────────────────────────────
    void buildChopTab();
    void buildPadTab();
    void buildFadeTab();
    void buildAlignTab();
    void buildCleanTab();
    void buildStretchTab();
    void updateChopInfo();
    void updateRegionList();
    void onAnalyzeAlign();
    void applyNudge(double deltaMs);
    void loadFile(const juce::File& f);
    void applyUndo();
    void applyRedo();

    std::unique_ptr<juce::FileChooser> fileChooser_;

    // Owned hint labels (avoid raw heap allocation)
    juce::Label fadeHintLabel_;
    juce::Label alignHintLabel_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VocalChopperEditor)
};
