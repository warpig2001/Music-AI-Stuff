#pragma once
#include <atomic>
#include <JuceHeader.h>
#include "ChopEngine.h"
#include "AlignmentAnalyzer.h"
#include "MidiChopTrigger.h"
#include "PitchDetector.h"
#include "NoiseGate.h"
#include "StretchEngine.h"
#include "UndoHistory.h"
#include "SpectrumAnalyzer.h"
#include "PresetManager.h"

class VocalChopperProcessor : public juce::AudioProcessor
{
public:
    VocalChopperProcessor();
    ~VocalChopperProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Vocal Chopper"; }

    bool   acceptsMidi()               const override;
    bool   producesMidi()              const override { return false; }
    bool   isMidiEffect()              const override { return false; }

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override
    {
        // Accept any stereo or mono layout — reject anything else
        if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
         && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
            return false;
        if (layouts.getMainInputChannelSet()  != juce::AudioChannelSet::mono()
         && layouts.getMainInputChannelSet()  != juce::AudioChannelSet::stereo()
         && !layouts.getMainInputChannelSet().isDisabled())
            return false;
        return true;
    }
    double getTailLengthSeconds()      const override { return 0.0; }

    int  getNumPrograms()              override { return 1; }
    int  getCurrentProgram()           override { return 0; }
    void setCurrentProgram(int)        override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& dest) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // ── Public API for editor ─────────────────────────────────────────────
    ChopEngine&        getChopEngine()        { return chopEngine_; }
    AlignmentAnalyzer& getAlignmentAnalyzer() { return alignAnalyzer_; }
    MidiChopTrigger&   getMidiTrigger()       { return midiTrigger_; }
    RegionPitchAnalyzer& getPitchAnalyzer()   { return pitchAnalyzer_; }
    juce::AudioFormatManager& getFormatManager() { return formatManager_; }

    void  loadVocalFile(const juce::File& file);
    bool  isPlaying() const { return isPlaying_; }
    void  startPlayback()   { isPlaying_ = true; playheadSample_ = 0; }
    void  startRegion(int regionIdx);
    void  stopPlayback()    { isPlaying_ = false; }
    double getPlayheadPosition() const;

    // Thread-safe pitch read (audio thread writes, UI thread reads)
    PitchResult getCurrentPitch() const
    {
        const juce::ScopedLock sl(pitchLock_);
        return currentPitch_;
    }

    // All region pitch analysis (run after loading)
    const std::vector<RegionPitchInfo>& getRegionPitchInfo() const { return regionPitches_; }
    void analyzeAllPitches();

    PresetManager&    getPresetManager()    { return presetManager_; }
    StretchEngine&    getStretchEngine()    { return stretchEngine_; }
    UndoHistory&      getUndoHistory()      { return undoHistory_; }
    SpectrumAnalyzer& getSpectrumAnalyzer() { return spectrumAnalyzer_; }

    // Clean buffer (set by CleanTab after gating)
    void  setCleanBuffer(const juce::AudioBuffer<float>* buf) { cleanBufferOverride_ = buf; }
    double getSampleRate() const { return vocalSampleRate_; }
    const juce::AudioBuffer<float>* getSourceBuffer() const { return vocalBuffer_.get(); }
    const juce::AudioBuffer<float>* getActiveBuffer() const
    { return cleanBufferOverride_ ? cleanBufferOverride_ : vocalBuffer_.get(); }

    // Parameters
    juce::AudioProcessorValueTreeState params;

private:
    juce::AudioFormatManager  formatManager_;
    ChopEngine                chopEngine_;
    AlignmentAnalyzer         alignAnalyzer_;
    MidiChopTrigger           midiTrigger_;
    RegionPitchAnalyzer       pitchAnalyzer_;

    std::unique_ptr<juce::AudioBuffer<float>> vocalBuffer_;
    double vocalSampleRate_   = 44100.0;
    std::atomic<int>   playheadSample_  { 0 };
    std::atomic<bool>  isPlaying_      { false };
    int                activeRegionIdx_  = -1;
    bool               applyFades_       = true;

    // currentPitch_ written from audio thread — access via getCachedPitch() only
    PitchResult              currentPitch_;
    juce::CriticalSection    pitchLock_;
    const juce::AudioBuffer<float>* cleanBufferOverride_ = nullptr;
    StretchEngine    stretchEngine_;
    PresetManager    presetManager_ { params };
    UndoHistory      undoHistory_;
    SpectrumAnalyzer spectrumAnalyzer_;
    std::vector<RegionPitchInfo> regionPitches_;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParams();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VocalChopperProcessor)
};
