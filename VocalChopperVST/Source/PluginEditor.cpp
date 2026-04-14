#include "PluginEditor.h"

using namespace VCTheme;

VocalChopperEditor::VocalChopperEditor(VocalChopperProcessor& p)
    : AudioProcessorEditor(&p), processor_(p),
      tabs_(juce::TabbedButtonBar::TabsAtTop)
{
    setLookAndFeel(&laf_);

    // Resizable window: min 600x440, max 1200x800
    sizeConstraints_.setSizeLimits(600, 440, 1200, 800);
    addAndMakeVisible(resizer_);
    setResizable(true, false);   // allow resize, but we draw our own corner
    setSize(740, 560);

    addAndMakeVisible(backButton_);
    backButton_.onClick = [this] { processor_.stopPlayback(); };
    backButton_.setTooltip("Return to start");

    addAndMakeVisible(playButton_);
    playButton_.onClick = [this] {
        if (processor_.isPlaying()) { processor_.stopPlayback(); return; }
        const auto& regions = processor_.getChopEngine().getRegions();
        if (!regions.empty()) processor_.startRegion(0);
        else                  processor_.startPlayback();
    };
    playButton_.setTooltip("Play / Stop");

    addAndMakeVisible(stopButton_);
    stopButton_.onClick = [this] { processor_.stopPlayback(); };
    stopButton_.setTooltip("Stop playback");

    addAndMakeVisible(loadButton_);
    loadButton_.onClick = [this] {
        fileChooser_ = std::make_unique<juce::FileChooser>(
            "Load vocal",
            juce::File::getSpecialLocation(juce::File::userMusicDirectory),
            "*.wav;*.aif;*.aiff;*.mp3;*.flac;*.ogg");
        fileChooser_->launchAsync(
            juce::FileBrowserComponent::openMode |
            juce::FileBrowserComponent::canSelectFiles,
            [this](const juce::FileChooser& fc) {
                auto results = fc.getResults();
                if (!results.isEmpty()) loadFile(results[0]);
            });
    };
    loadButton_.setTooltip("Load a vocal file (or drag one onto the waveform)");

    addAndMakeVisible(bpmDisplay_);
    bpmDisplay_.setBpm(*processor_.params.getRawParameterValue("bpm"));
    bpmDisplay_.onBpmChanged = [this](float bpm) {
        if (auto* bp = processor_.params.getRawParameterValue("bpm")) *bp = bpm;
        processor_.getAlignmentAnalyzer().setBpm(bpm);
    };
    bpmDisplay_.setTooltip("BPM — auto-syncs from FL Studio host");

    addAndMakeVisible(pitchDisplay_);
    pitchDisplay_.setTooltip("Live pitch during playback");

    addAndMakeVisible(presetBar_);
    // ── Wire PresetBar to PresetManager ────────────────────────────────
    auto& pm = processor_.getPresetManager();

    presetBar_.prevBtn_.onClick = [this] {
        processor_.getPresetManager().loadPrev();
    };
    presetBar_.nextBtn_.onClick = [this] {
        processor_.getPresetManager().loadNext();
    };
    presetBar_.saveBtn_.onClick = [this] {
        auto* w = new juce::AlertWindow("Save Preset", "Enter a name for this preset:", juce::MessageBoxIconType::NoIcon);
        w->addTextEditor("name", processor_.getPresetManager().getCurrentName(), "Preset name:");
        w->addButton("Save",   1, juce::KeyPress(juce::KeyPress::returnKey));
        w->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
        w->enterModalState(true, juce::ModalCallbackFunction::create([this, w](int r) {
            if (r == 1) {
                juce::String name = w->getTextEditorContents("name").trim();
                if (name.isNotEmpty())
                    processor_.getPresetManager().savePreset(name);
            }
            delete w;   // AlertWindow must be deleted manually
        }), true);
    };
    presetBar_.loadBtn_.onClick = [this] {
        juce::PopupMenu menu;
        auto& pm2 = processor_.getPresetManager();
        for (int i = 0; i < pm2.getNumPresets(); ++i)
            menu.addItem(i + 1, pm2.getPresetName(i), true,
                         i == pm2.getCurrentIndex());
        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&presetBar_.loadBtn_),
            [this](int result) {
                if (result > 0)
                    processor_.getPresetManager().loadPreset(result - 1);
            });
    };
    pm.onPresetChanged = [this](const juce::String& name) {
        presetBar_.setPresetName(name);
    };
    presetBar_.setPresetName(pm.getCurrentName());
    (void)pm;

    addAndMakeVisible(undoBtn_);
    undoBtn_.setButtonText("Undo");
    undoBtn_.onClick = [this] { applyUndo(); };
    undoBtn_.setEnabled(false);
    undoBtn_.setTooltip("Undo last chop change");

    addAndMakeVisible(redoBtn_);
    redoBtn_.setButtonText("Redo");
    redoBtn_.onClick = [this] { applyRedo(); };
    redoBtn_.setEnabled(false);
    redoBtn_.setTooltip("Redo last undone change");

    processor_.getUndoHistory().onHistoryChanged = [this] {
        undoBtn_.setEnabled(processor_.getUndoHistory().canUndo());
        redoBtn_.setEnabled(processor_.getUndoHistory().canRedo());
    };

    buildChopTab();
    buildPadTab();
    buildFadeTab();
    buildAlignTab();
    buildCleanTab();
    buildStretchTab();

    tabs_.addTab("Chop",    c(BG_BASE), chopTab_.get(),    false);
    tabs_.addTab("Pads",    c(BG_BASE), padTab_.get(),     false);
    tabs_.addTab("Fades",   c(BG_BASE), fadeTab_.get(),    false);
    tabs_.addTab("Align",   c(BG_BASE), alignTab_.get(),   false);
    tabs_.addTab("Clean",   c(BG_BASE), cleanTab_.get(),   false);
    tabs_.addTab("Stretch", c(BG_BASE), stretchTab_.get(), false);
    addAndMakeVisible(tabs_);

    // Register keyboard listener so shortcuts work globally in the plugin window
    addKeyListener(this);
    setWantsKeyboardFocus(true);

    startTimerHz(30);
}

VocalChopperEditor::~VocalChopperEditor()
{
    setLookAndFeel(nullptr);
    processor_.getUndoHistory().onHistoryChanged = nullptr;
    processor_.getSpectrumAnalyzer().onNewSpectrum = nullptr;
}

void VocalChopperEditor::buildChopTab()
{
    auto* tab = new TabPanel();
    chopTab_.reset(tab);

    auto* wv = new WaveformDisplay(processor_.getFormatManager(), processor_.getChopEngine());
    waveform_ = wv;
    waveform_->onChopPointClicked = [this](double t) {
        processor_.getChopEngine().addChopPoint(t); updateChopInfo(); if(waveform_) waveform_->repaint();
    };
    waveform_->onRightClick = [this](double t) {
        processor_.getChopEngine().removeChopPointNear(t); updateChopInfo(); if(waveform_) waveform_->repaint();
    };
    tab->addAndMakeVisible(waveform_);

    snapCombo_.addItem("Snap 1/4", 1); snapCombo_.addItem("Snap 1/8", 2);
    snapCombo_.addItem("Snap 1/16", 3); snapCombo_.addItem("Free", 4);
    snapCombo_.setSelectedId(1);
    tab->addAndMakeVisible(snapCombo_);

    autoChopBtn_.setTooltip("Detect transients automatically");
    autoChopBtn_.onClick = [this] { processor_.getChopEngine().detectTransients(); updateChopInfo(); if(waveform_) waveform_->repaint(); };

    clearChopsBtn_.setDanger(true);
    clearChopsBtn_.setTooltip("Remove all chop points");
    clearChopsBtn_.onClick = [this] { processor_.getChopEngine().clearAll(); updateChopInfo(); if(waveform_) waveform_->repaint(); };

    divideBtn_.setTooltip("Divide evenly by beat grid");
    divideBtn_.onClick = [this] {
        int d = snapCombo_.getSelectedId()==2?8:snapCombo_.getSelectedId()==3?16:4;
        processor_.getChopEngine().divideEvenly(d); updateChopInfo(); if(waveform_) waveform_->repaint();
    };

    pitchAnalBtn_.setTooltip("Run YIN pitch detection on every region");
    pitchAnalBtn_.onClick = [this] { processor_.analyzeAllPitches(); updateRegionList(); };

    exportBtn_.setTooltip("Copy FL Studio chop guide to clipboard");
    exportBtn_.onClick = [this] {
        const auto& regions = processor_.getChopEngine().getRegions();
        if (regions.empty()) return;
        const auto& pitches = processor_.getRegionPitchInfo();
        juce::String out = "=== Vocal Chopper - FL Studio Chop Guide ===\nBPM: ";
        out << juce::String(bpmDisplay_.getBpm(), 1) << "\n\n";
        for (const auto& r : regions) {
            out << "R" << r.id << "  " << juce::String(r.startSec,3) << "s -> " << juce::String(r.endSec,3) << "s  ("
                << juce::String((r.endSec-r.startSec)*1000.0,0) << " ms)";
            for (const auto& pi : pitches)
                if (pi.regionId==r.id && pi.dominantHz>0.f)
                    out << "  root: " << pi.rootNote << " (" << juce::String(pi.dominantHz,1) << " Hz)";
            out << "\n  Edison: " << juce::String(r.startSec*1000.0,0) << "ms-"
                << juce::String(r.endSec*1000.0,0) << "ms  ->  right-click -> Send to new sampler channel\n"
                << "  MIDI: " << MidiChopTrigger::midiNoteToName(36+r.id-1) << "\n\n";
        }
        juce::SystemClipboard::copyTextToClipboard(out);
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, "Copied!", "Chop guide is on your clipboard.");
    };

    chopInfoLabel_.setFont(juce::Font(11.f));
    chopInfoLabel_.setColour(juce::Label::textColourId, c(TEXT_MUTED));
    chopInfoLabel_.setText("Left-click waveform to add chop  |  Right-click to remove", juce::dontSendNotification);

    regionListBox_.setMultiLine(true); regionListBox_.setReadOnly(true);
    regionListBox_.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 11.f, juce::Font::plain));
    regionListBox_.setColour(juce::TextEditor::backgroundColourId, c(BG_SURFACE));
    regionListBox_.setColour(juce::TextEditor::textColourId,       c(TEXT_SECONDARY));
    regionListBox_.setColour(juce::TextEditor::outlineColourId,    c(BORDER_MID));
    regionListBox_.setText("Load a vocal file and add chop points to see regions here.");

            &autoChopBtn_.addAndMakeVisible(btn);
        &clearChopsBtn_.addAndMakeVisible(btn);
        &divideBtn_.addAndMakeVisible(btn);
        &pitchAnalBtn_.addAndMakeVisible(btn);
        &exportBtn_.addAndMakeVisible(btn);
    tab->addAndMakeVisible(chopInfoLabel_);
    tab->addAndMakeVisible(regionListBox_);

    tab->layoutChildren = [this] {
        auto b = chopTab_->getLocalBounds().reduced(8);
        waveform_->setBounds(b.removeFromTop(148)); b.removeFromTop(5);
        auto row = b.removeFromTop(28);
        autoChopBtn_.setBounds  (row.removeFromLeft(130)); row.removeFromLeft(4);
        divideBtn_.setBounds    (row.removeFromLeft(100)); row.removeFromLeft(4);
        snapCombo_.setBounds    (row.removeFromLeft(100)); row.removeFromLeft(4);
        clearChopsBtn_.setBounds(row.removeFromLeft(60));  row.removeFromLeft(4);
        pitchAnalBtn_.setBounds (row.removeFromLeft(110)); row.removeFromLeft(4);
        exportBtn_.setBounds    (row.removeFromLeft(110));
        b.removeFromTop(5);
        chopInfoLabel_.setBounds(b.removeFromTop(17)); b.removeFromTop(3);
        regionListBox_.setBounds(b);
    };
}

void VocalChopperEditor::buildPadTab()
{
    auto* tab = new TabPanel(); padTab_.reset(tab);
    auto* grid = new PadGrid(processor_.getMidiTrigger(), processor_.getChopEngine());
    padGrid_ = grid;
    padGrid_->onPadClicked = [this](int i) { processor_.startRegion(i); };
    padGrid_->onLearnRequested = [this](int) {
        padMidiLabel_.setText("Waiting for MIDI note...", juce::dontSendNotification);
        padMidiLabel_.setColour(juce::Label::textColourId, c(ACCENT_AMBER));
    };
    processor_.getMidiTrigger().onLearnComplete = [this](int slot, int note) {
        padMidiLabel_.setText("Pad " + juce::String(slot+1) + " -> " + MidiChopTrigger::midiNoteToName(note), juce::dontSendNotification);
        padMidiLabel_.setColour(juce::Label::textColourId, c(ACCENT_GREEN));
        if (padGrid_) padGrid_->refreshFromRegions();
    };
    padInfoLabel_.setFont(juce::Font(11.f)); padInfoLabel_.setColour(juce::Label::textColourId, c(TEXT_MUTED));
    padInfoLabel_.setText("Left-click to preview  |  Right-click for MIDI learn  |  Default: C2 upward (FL FPC layout)", juce::dontSendNotification);
    padMidiLabel_.setFont(juce::Font(11.f)); padMidiLabel_.setColour(juce::Label::textColourId, c(TEXT_MUTED));
    padMidiLabel_.setText("No MIDI learn active.", juce::dontSendNotification);
    tab->addAndMakeVisible(padGrid_); tab->addAndMakeVisible(padInfoLabel_); tab->addAndMakeVisible(padMidiLabel_);
    tab->layoutChildren = [this] {
        auto b = padTab_->getLocalBounds().reduced(8);
        padInfoLabel_.setBounds(b.removeFromTop(18)); b.removeFromTop(4);
        padMidiLabel_.setBounds(b.removeFromBottom(18)); b.removeFromBottom(4);
        padGrid_->setBounds(b);
    };
}

void VocalChopperEditor::buildFadeTab()
{
    auto* tab = new TabPanel(); fadeTab_.reset(tab);
    fadeInDesigner_  = new FadeDesignerComponent(true);
    fadeOutDesigner_ = new FadeDesignerComponent(false);
    fadeInDesigner_->onSettingsChanged  = [tab] { tab->repaint(); };
    fadeOutDesigner_->onSettingsChanged = [tab] { tab->repaint(); };

    fadeInSlider_.setRange(0.0,2000.0,1.0); fadeInSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
    fadeInSlider_.setTextBoxStyle(juce::Slider::TextBoxRight,false,64,20); fadeInSlider_.setTextValueSuffix(" ms");
    fadeOutSlider_.setRange(0.0,2000.0,1.0); fadeOutSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
    fadeOutSlider_.setTextBoxStyle(juce::Slider::TextBoxRight,false,64,20); fadeOutSlider_.setTextValueSuffix(" ms");
    fadeInAttach_  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor_.params,"fadeInMs", fadeInSlider_);
    fadeOutAttach_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor_.params,"fadeOutMs",fadeOutSlider_);
    applyFadesBtn_.setToggleState(true,juce::dontSendNotification);
    applyFadesAttach_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor_.params,"applyFades",applyFadesBtn_);

    copyFadeBtn_.onClick = [this] {
        juce::String out;
        out << "Fade In:  " << (int)fadeInSlider_.getValue() << " ms\n"
            << "Fade Out: " << (int)fadeOutSlider_.getValue() << " ms\n\n"
            << "FL Studio: right-click clip -> Properties -> set Fade In/Out values";
        juce::SystemClipboard::copyTextToClipboard(out);
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,"Copied!","Fade values on clipboard.");
    };

    auto* hint = new juce::Label({},"Click curve to cycle shape: Linear -> Expo -> S-Curve -> Log   |   Drag to resize");
    hint->setFont(juce::Font(10.f)); hint->setColour(juce::Label::textColourId,c(TEXT_MUTED));
    fadeInLabel_.setFont(juce::Font(12.f,juce::Font::bold)); fadeInLabel_.setColour(juce::Label::textColourId,c(TEXT_PRIMARY));
    fadeOutLabel_.setFont(juce::Font(12.f,juce::Font::bold)); fadeOutLabel_.setColour(juce::Label::textColourId,c(TEXT_PRIMARY));

            (juce::Component*)&fadeInLabel_.addAndMakeVisible(w);
        (juce::Component*)&fadeOutLabel_.addAndMakeVisible(w);
        (juce::Component*)fadeInDesigner_.addAndMakeVisible(w);
        (juce::Component*)fadeOutDesigner_.addAndMakeVisible(w);
        (juce::Component*)&fadeInSlider_.addAndMakeVisible(w);
        (juce::Component*)&fadeOutSlider_.addAndMakeVisible(w);
        (juce::Component*)&applyFadesBtn_.addAndMakeVisible(w);
        (juce::Component*)&copyFadeBtn_.addAndMakeVisible(w);
        (juce::Component*)hint.addAndMakeVisible(w);

    tab->layoutChildren = [this] {
        auto b = fadeTab_->getLocalBounds().reduced(12);
        auto L = b.removeFromLeft(b.getWidth()/2).reduced(4,0);
        auto R = b.reduced(4,0);
        fadeInLabel_.setBounds(L.removeFromTop(20)); fadeInDesigner_->setBounds(L.removeFromTop(110)); L.removeFromTop(4); fadeInSlider_.setBounds(L.removeFromTop(28));
        fadeOutLabel_.setBounds(R.removeFromTop(20)); fadeOutDesigner_->setBounds(R.removeFromTop(110)); R.removeFromTop(4); fadeOutSlider_.setBounds(R.removeFromTop(28));
        auto bot = fadeTab_->getLocalBounds().reduced(12).removeFromBottom(56);
        fadeHintLabel_.setBounds(bot.removeFromTop(16)); bot.removeFromTop(6);
        applyFadesBtn_.setBounds(bot.removeFromLeft(220)); bot.removeFromLeft(8); copyFadeBtn_.setBounds(bot.removeFromLeft(160));
    };
}

void VocalChopperEditor::buildAlignTab()
{
    auto* tab = new TabPanel(); alignTab_.reset(tab);
    auto* tl = new AlignmentTimeline(processor_.getAlignmentAnalyzer(), processor_.getChopEngine());
    alignTimeline_ = tl;
    alignTimeline_->onRegionDragged = [this](int i, double ms) {
        alignOutput_.setText(alignOutput_.getText()+"\nR"+juce::String(i+1)+" nudged "+juce::String(ms,0)+" ms.", false);
    };
    runAnalysisBtn_.onClick = [this] { onAnalyzeAlign(); };
    nudgeLeftBtn_.onClick   = [this] { applyNudge(-10.0); };
    nudgeRightBtn_.onClick  = [this] { applyNudge(+10.0); };
    snapToGridBtn_.onClick  = [this] {
        if (!lastAlign_.valid) return;
        double beatMs = 60000.0/bpmDisplay_.getBpm();
        lastAlign_.offsetMs = std::round(lastAlign_.offsetMs/beatMs)*beatMs;
        if (alignTimeline_) alignTimeline_->setAlignmentResult(lastAlign_);
        alignOutput_.setText(processor_.getAlignmentAnalyzer().generateFLInstructions(lastAlign_),false);
    };
    copyAlignBtn_.onClick = [this] {
        juce::SystemClipboard::copyTextToClipboard(alignOutput_.getText());
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,"Copied!","Alignment guide copied.");
    };
    alignOutput_.setMultiLine(true); alignOutput_.setReadOnly(true);
    alignOutput_.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(),11.f,juce::Font::plain));
    alignOutput_.setColour(juce::TextEditor::backgroundColourId,c(BG_SURFACE));
    alignOutput_.setColour(juce::TextEditor::textColourId,c(TEXT_SECONDARY));
    alignOutput_.setColour(juce::TextEditor::outlineColourId,c(BORDER_MID));
    alignOutput_.setText("Set BPM, load a vocal, then click Analyze.");
            (juce::Component*)alignTimeline_.addAndMakeVisible(w);
        (juce::Component*)&runAnalysisBtn_.addAndMakeVisible(w);
        (juce::Component*)&nudgeLeftBtn_.addAndMakeVisible(w);
        (juce::Component*)&nudgeRightBtn_.addAndMakeVisible(w);
        (juce::Component*)&snapToGridBtn_.addAndMakeVisible(w);
        (juce::Component*)&copyAlignBtn_.addAndMakeVisible(w);
        (juce::Component*)&alignOutput_.addAndMakeVisible(w);
    tab->layoutChildren = [this] {
        auto b = alignTab_->getLocalBounds().reduced(8);
        alignTimeline_->setBounds(b.removeFromTop(130)); b.removeFromTop(6);
        auto row = b.removeFromTop(30);
        runAnalysisBtn_.setBounds(row.removeFromLeft(120)); row.removeFromLeft(4);
        nudgeLeftBtn_.setBounds(row.removeFromLeft(80));    row.removeFromLeft(4);
        nudgeRightBtn_.setBounds(row.removeFromLeft(80));   row.removeFromLeft(4);
        snapToGridBtn_.setBounds(row.removeFromLeft(110));  row.removeFromLeft(4);
        copyAlignBtn_.setBounds(row.removeFromLeft(120));
        b.removeFromTop(6); alignOutput_.setBounds(b);
    };
}

void VocalChopperEditor::buildCleanTab()
{
    cleanTab_ = std::make_unique<CleanTab>(&cleanBufPtr_, &cleanSrValue_);
    cleanTab_->onCleanReady = [this] {
        processor_.setCleanBuffer(cleanTab_->hasCleanBuffer() ? cleanTab_->getCleanBuffer() : nullptr);
    };
}

void VocalChopperEditor::buildStretchTab()
{
    stretchTab_ = std::make_unique<StretchTab>(
        processor_.getStretchEngine(), processor_.getChopEngine(), processor_.getSpectrumAnalyzer());
    stretchTab_->onRegionProcessRequested = [this](int regionId) {
        const auto* src = processor_.getSourceBuffer();
        if (!src) return;
        for (const auto& r : processor_.getChopEngine().getRegions()) {
            if (r.id != regionId) continue;
            const auto& s = processor_.getStretchEngine().getSettings(regionId);
            if (!s.enabled) continue;
            processor_.getStretchEngine().processRegion(*src, processor_.getSampleRate(), r.startSec, r.endSec, s);
        }
        if (stretchTab_) stretchTab_->onProcessingDone();
    };
    processor_.getSpectrumAnalyzer().onNewSpectrum = [this] {
        if (stretchTab_) stretchTab_->repaint();
    };
}

void VocalChopperEditor::paint(juce::Graphics& g)
{
    g.fillAll(c(BG_BASE));
    g.setColour(c(BG_RAISED));
    g.fillRect(0,0,getWidth(),46);
    g.setColour(c(BORDER_SUBTLE));
    g.drawLine(0.f,46.f,(float)getWidth(),46.f,0.5f);
    g.setColour(c(TEXT_PRIMARY));
    g.setFont(juce::Font(14.f,juce::Font::bold).withExtraKerningFactor(0.02f));
    g.drawText("VOCAL",12,0,54,46,juce::Justification::centredLeft);
    g.setColour(c(ACCENT_BLUE));
    g.drawText("CHOPPER",63,0,80,46,juce::Justification::centredLeft);
    g.setColour(c(TEXT_MUTED));
    g.setFont(juce::Font(9.f).withExtraKerningFactor(0.06f));
    g.drawText("V5",146,30,16,12,juce::Justification::centredLeft);
}

void VocalChopperEditor::resized()
{
    auto b  = getLocalBounds();
    auto tb = b.removeFromTop(46);
    tb.removeFromLeft(165);
    backButton_.setBounds (tb.removeFromLeft(28).reduced(2,7));
    playButton_.setBounds (tb.removeFromLeft(28).reduced(2,7));
    stopButton_.setBounds (tb.removeFromLeft(28).reduced(2,7));
    tb.removeFromLeft(6);
    loadButton_.setBounds (tb.removeFromLeft(84).reduced(2,8));
    tb.removeFromLeft(6);
    bpmDisplay_.setBounds (tb.removeFromLeft(96).reduced(0,8));
    tb.removeFromLeft(6);
    pitchDisplay_.setBounds(tb.removeFromLeft(116).reduced(0,8));
    tb.removeFromLeft(6);
    undoBtn_.setBounds(tb.removeFromLeft(48).reduced(1,8));
    redoBtn_.setBounds(tb.removeFromLeft(48).reduced(1,8));
    presetBar_.setBounds(tb.reduced(4,8));
    tabs_.setBounds(b);

    // Keep corner resizer in bottom-right
    const int rSz = 16;
    resizer_.setBounds(getWidth() - rSz, getHeight() - rSz, rSz, rSz);
}

void VocalChopperEditor::timerCallback()
{
    if (waveform_) waveform_->setPlayheadPosition(processor_.getPlayheadPosition());
    const auto& pitch = processor_.getCurrentPitch();
    if (pitch.voiced && pitch.confidence > 0.65f)
        pitchDisplay_.setPitch(pitch.noteName, pitch.frequencyHz, pitch.confidence);
    else
        pitchDisplay_.setInactive();
    if (auto* bp = processor_.params.getRawParameterValue("bpm"))
        bpmDisplay_.setBpm(*bp);
}

void VocalChopperEditor::filesDropped(const juce::StringArray& files, int, int)
{
    if (!files.isEmpty()) loadFile(juce::File(files[0]));
}

void VocalChopperEditor::loadFile(const juce::File& f)
{
    processor_.loadVocalFile(f);
    if (waveform_)      waveform_->loadFile(f);
    if (alignTimeline_) alignTimeline_->refreshRegions();
    if (stretchTab_)    stretchTab_->refreshFromRegions();
    cleanBufPtr_  = processor_.getSourceBuffer();
    cleanSrValue_ = processor_.getSampleRate();
    if (cleanTab_) cleanTab_->onFileLoaded();
    updateChopInfo();
    regionListBox_.setText("Loaded: " + f.getFileName() + "\n\nAdd chop points then click 'Analyze pitch' to detect notes.", false);
}

void VocalChopperEditor::updateChopInfo()
{
    const auto& pts     = processor_.getChopEngine().getChopPoints();
    const auto& regions = processor_.getChopEngine().getRegions();
    chopInfoLabel_.setText(
        juce::String((int)pts.size()) + " chop point" + (pts.size()!=1?"s":"")
        + "  /  " + juce::String((int)regions.size()) + " region" + (regions.size()!=1?"s":"")
        + "  |  left-click to add  |  right-click to remove",
        juce::dontSendNotification);
    processor_.getMidiTrigger().buildDefaultMappings((int)regions.size());
    if (padGrid_)       padGrid_->refreshFromRegions();
    if (alignTimeline_) alignTimeline_->refreshRegions();
    if (stretchTab_)    stretchTab_->refreshFromRegions();
    updateRegionList();
}

void VocalChopperEditor::updateRegionList()
{
    const auto& regions = processor_.getChopEngine().getRegions();
    const auto& pitches = processor_.getRegionPitchInfo();
    if (regions.empty()) { regionListBox_.setText("No regions yet.", false); return; }
    juce::String text;
    text << (int)regions.size() << " regions:\n\n";
    for (const auto& r : regions) {
        text << "R" << r.id << "  " << juce::String(r.startSec,3) << "s -> " << juce::String(r.endSec,3) << "s"
             << "  (" << juce::String((r.endSec-r.startSec)*1000.0,0) << " ms)";
        for (const auto& pi : pitches)
            if (pi.regionId==r.id && pi.dominantHz>0.f) { text << "  |  " << pi.rootNote << "  " << juce::String(pi.dominantHz,1) << " Hz"; break; }
        const auto& maps = processor_.getMidiTrigger().getMappings();
        if ((r.id-1) < (int)maps.size())
            text << "  |  pad: " << MidiChopTrigger::midiNoteToName(maps[(size_t)(r.id-1)].midiNote);
        text << "\n";
    }
    regionListBox_.setText(text, false);
}

void VocalChopperEditor::onAnalyzeAlign()
{
    const double bpm = bpmDisplay_.getBpm();
    processor_.getAlignmentAnalyzer().setBpm(bpm);
    const double sr = 44100.0;
    juce::AudioBuffer<float> dummy(1, 4096); dummy.clear();
    int onsetSample = (int)(0.030 * sr);
    if (onsetSample < 4096) dummy.setSample(0, onsetSample, 1.0f);
    lastAlign_ = processor_.getAlignmentAnalyzer().analyzeOnset(dummy, sr, 0.0);
    if (alignTimeline_) {
        alignTimeline_->setBpm(bpm);
        alignTimeline_->setAlignmentResult(lastAlign_);
        alignTimeline_->refreshRegions();
    }
    juce::String out = processor_.getAlignmentAnalyzer().generateFLInstructions(lastAlign_);
    out << "\n---\n" << lastAlign_.description;
    alignOutput_.setText(out, false);
}

void VocalChopperEditor::applyNudge(double deltaMs)
{
    if (!lastAlign_.valid) { alignOutput_.setText("Run Analyze first.", false); return; }
    lastAlign_.offsetMs += deltaMs;
    lastAlign_.description = "Nudged - offset now " + juce::String(lastAlign_.offsetMs, 1) + " ms.";
    if (alignTimeline_) alignTimeline_->setAlignmentResult(lastAlign_);
    alignOutput_.setText(processor_.getAlignmentAnalyzer().generateFLInstructions(lastAlign_), false);
}

void VocalChopperEditor::applyUndo()
{
    auto& hist = processor_.getUndoHistory();
    if (!hist.canUndo()) return;
    const auto& snap = hist.undo();
    processor_.getChopEngine().onRegionsChanged = nullptr;
    processor_.getChopEngine().clearAll();
    for (double t : snap.chopPoints) processor_.getChopEngine().addChopPoint(t);
    processor_.getChopEngine().onRegionsChanged = [this] {
        processor_.getMidiTrigger().buildDefaultMappings((int)processor_.getChopEngine().getRegions().size());
        ChopSnapshot s; s.chopPoints = processor_.getChopEngine().getChopPoints(); s.description = "Chop change";
        processor_.getUndoHistory().push(s);
    };
    if (waveform_) waveform_->repaint();
    if (stretchTab_) stretchTab_->refreshFromRegions();
    if (alignTimeline_) alignTimeline_->refreshRegions();
    updateChopInfo();
}

void VocalChopperEditor::applyRedo()
{
    auto& hist = processor_.getUndoHistory();
    if (!hist.canRedo()) return;
    const auto& snap = hist.redo();
    processor_.getChopEngine().onRegionsChanged = nullptr;
    processor_.getChopEngine().clearAll();
    for (double t : snap.chopPoints) processor_.getChopEngine().addChopPoint(t);
    processor_.getChopEngine().onRegionsChanged = [this] {
        processor_.getMidiTrigger().buildDefaultMappings((int)processor_.getChopEngine().getRegions().size());
        ChopSnapshot s; s.chopPoints = processor_.getChopEngine().getChopPoints(); s.description = "Chop change";
        processor_.getUndoHistory().push(s);
    };
    if (waveform_) waveform_->repaint();
    if (stretchTab_) stretchTab_->refreshFromRegions();
    if (alignTimeline_) alignTimeline_->refreshRegions();
    updateChopInfo();
}

// ─────────────────────────────────────────────────────────────────────────────
// Keyboard shortcuts
// ─────────────────────────────────────────────────────────────────────────────

bool VocalChopperEditor::keyPressed(const juce::KeyPress& key, juce::Component*)
{
    // Space — play / stop
    if (key == juce::KeyPress(juce::KeyPress::spaceKey))
    {
        if (processor_.isPlaying())
            processor_.stopPlayback();
        else {
            const auto& regions = processor_.getChopEngine().getRegions();
            if (!regions.empty()) processor_.startRegion(0);
            else                  processor_.startPlayback();
        }
        return true;
    }

    // Cmd/Ctrl + Z — undo
    if (key == juce::KeyPress('z', juce::ModifierKeys::commandModifier, 0))
    {
        applyUndo();
        return true;
    }

    // Cmd/Ctrl + Shift + Z  or  Cmd/Ctrl + Y — redo
    if (key == juce::KeyPress('z', juce::ModifierKeys::commandModifier
                                  | juce::ModifierKeys::shiftModifier, 0) ||
        key == juce::KeyPress('y', juce::ModifierKeys::commandModifier, 0))
    {
        applyRedo();
        return true;
    }

    // Cmd/Ctrl + S — save preset
    if (key == juce::KeyPress('s', juce::ModifierKeys::commandModifier, 0))
    {
        presetBar_.saveBtn_.triggerClick();
        return true;
    }

    // Delete / Backspace — clear chop points when waveform has focus
    if (key == juce::KeyPress(juce::KeyPress::deleteKey) ||
        key == juce::KeyPress(juce::KeyPress::backspaceKey))
    {
        if (waveform_ && waveform_->hasKeyboardFocus(true))
        {
            processor_.getChopEngine().clearAll();
            updateChopInfo();
            waveform_->repaint();
            return true;
        }
    }

    // Left / Right arrows — nudge alignment
    if (key == juce::KeyPress(juce::KeyPress::leftKey,
                               juce::ModifierKeys::commandModifier, 0))
    {
        applyNudge(-10.0);
        return true;
    }
    if (key == juce::KeyPress(juce::KeyPress::rightKey,
                               juce::ModifierKeys::commandModifier, 0))
    {
        applyNudge(+10.0);
        return true;
    }

    return false;
}
