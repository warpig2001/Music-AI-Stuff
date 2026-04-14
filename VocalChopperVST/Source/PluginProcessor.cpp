#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout VocalChopperProcessor::createParams()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "bpm", "BPM", juce::NormalisableRange<float>(40.f, 300.f, 0.1f), 140.f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "fadeInMs", "Fade In (ms)", juce::NormalisableRange<float>(0.f, 2000.f, 1.f), 10.f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "fadeOutMs", "Fade Out (ms)", juce::NormalisableRange<float>(0.f, 2000.f, 1.f), 10.f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "gain", "Output Gain", juce::NormalisableRange<float>(0.f, 2.f, 0.01f), 1.f));
    p.push_back(std::make_unique<juce::AudioParameterBool>(
        "applyFades", "Apply Fades", true));
    return { p.begin(), p.end() };
}

VocalChopperProcessor::VocalChopperProcessor()
    : AudioProcessor(BusesProperties()
          .withInput ("Input",  juce::AudioChannelSet::stereo(), true)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      params(*this, nullptr, "VocalChopper", createParams())
{
    formatManager_.registerBasicFormats();
    // Default MIDI mapping hook — rebuilt after each load
    chopEngine_.onRegionsChanged = [this] {
        midiTrigger_.buildDefaultMappings((int)chopEngine_.getRegions().size());
    };
}

VocalChopperProcessor::~VocalChopperProcessor() {}

// FIX BUG 2: acceptsMidi must return true so FL routes MIDI to us
bool VocalChopperProcessor::acceptsMidi() const { return true; }

juce::AudioProcessorEditor* VocalChopperProcessor::createEditor()
{
    return new VocalChopperEditor(*this);   // FIX BUG 11
}

void VocalChopperProcessor::prepareToPlay(double sampleRate, int)
{
    spectrumAnalyzer_.setSampleRate(sampleRate);
    pitchAnalyzer_.setSampleRate(sampleRate);

    if (auto* ph = getPlayHead())
    {
        if (auto pos = ph->getPosition())
        {
            if (pos->getBpm().hasValue() && *pos->getBpm() > 0.0)
            {
                double bpm = *pos->getBpm();
                alignAnalyzer_.setBpm(bpm);
                if (auto* bp = params.getRawParameterValue("bpm"))
                    *bp = (float)bpm;
            }
        }
    }
}

void VocalChopperProcessor::releaseResources() {}

void VocalChopperProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                          juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    const auto* buf = getActiveBuffer();
    if (!buf) return;

    // Handle MIDI triggers
    auto events = midiTrigger_.processMidi(midiMessages, chopEngine_);
    if (!events.empty())
    {
        activeRegionIdx_ = events[0].regionIdx;
        playheadSample_  = 0;
        isPlaying_       = true;
    }

    if (!isPlaying_) return;

    const float gain      = *params.getRawParameterValue("gain");
    const float fadeInMs  = *params.getRawParameterValue("fadeInMs");
    const float fadeOutMs = *params.getRawParameterValue("fadeOutMs");
    const bool  doFades   = (bool)(int)*params.getRawParameterValue("applyFades");

    const auto& regions = chopEngine_.getRegions();
    if (regions.empty() || activeRegionIdx_ < 0 ||
        activeRegionIdx_ >= (int)regions.size())
    {
        isPlaying_ = false;
        return;
    }

    const auto& region  = regions[(size_t)activeRegionIdx_];
    int startSample = (int)(region.startSec * vocalSampleRate_);
    int endSample   = (int)(region.endSec   * vocalSampleRate_);
    int regionLen   = endSample - startSample;

    const int outCh   = buffer.getNumChannels();
    const int srcCh   = buf->getNumChannels();
    const int blockSz = buffer.getNumSamples();

    for (int s = 0; s < blockSz; ++s)
    {
        if (playheadSample_ >= regionLen) { isPlaying_ = false; break; }
        int src = startSample + playheadSample_;
        if (src >= buf->getNumSamples())  { isPlaying_ = false; break; }

        float env = 1.f;
        if (doFades)
        {
            float fi = fadeInMs  * (float)vocalSampleRate_ / 1000.f;
            float fo = fadeOutMs * (float)vocalSampleRate_ / 1000.f;
            if (playheadSample_ < (int)fi && fi > 0.f)
                env = (float)playheadSample_ / fi;
            else if ((regionLen - playheadSample_) < (int)fo && fo > 0.f)
                env = (float)(regionLen - playheadSample_) / fo;
        }

        for (int ch = 0; ch < outCh; ++ch)
            buffer.setSample(ch, s, buf->getSample(juce::jmin(ch, srcCh - 1), src) * gain * env);

        ++playheadSample_;
    }

    // Live pitch detection (locked for thread-safe UI read)
    {
        const juce::ScopedLock sl(pitchLock_);
        currentPitch_ = pitchAnalyzer_.getSingleDetector().processSamples(buffer);
    }

    // Feed spectrum analyser (mono mix)
    if (buffer.getNumSamples() > 0 && buffer.getNumChannels() > 0)
    {
        std::vector<float> mono((size_t)buffer.getNumSamples(), 0.f);
        const float sc = 1.f / (float)buffer.getNumChannels();
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            const float* rd = buffer.getReadPointer(ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i)
                mono[(size_t)i] += rd[i] * sc;
        }
        spectrumAnalyzer_.pushSamples(mono.data(), buffer.getNumSamples());
    }
}

void VocalChopperProcessor::startRegion(int idx)
{
    activeRegionIdx_ = idx;
    playheadSample_  = 0;
    isPlaying_       = true;
}

void VocalChopperProcessor::loadVocalFile(const juce::File& file)
{
    std::unique_ptr<juce::AudioFormatReader> reader(
        formatManager_.createReaderFor(file));
    if (!reader) return;

    isPlaying_       = false;
    vocalSampleRate_ = reader->sampleRate;
    vocalBuffer_     = std::make_unique<juce::AudioBuffer<float>>(
                           (int)reader->numChannels, (int)reader->lengthInSamples);
    reader->read(vocalBuffer_.get(), 0, (int)reader->lengthInSamples, 0, true, true);  // unique_ptr cleans up reader automatically

    cleanBufferOverride_ = nullptr;   // reset clean buffer on new load
    chopEngine_.setAudioBuffer(vocalBuffer_.get(), vocalSampleRate_);
    stretchEngine_.setSampleRate(vocalSampleRate_);
    spectrumAnalyzer_.setSampleRate(vocalSampleRate_);
    pitchAnalyzer_.setSampleRate(vocalSampleRate_);
    alignAnalyzer_.setBpm(*params.getRawParameterValue("bpm"));

    undoHistory_.clear();
    undoHistory_.push({ {}, "File loaded" });

    // Re-wire so future chop changes feed the undo stack
    chopEngine_.onRegionsChanged = [this] {
        midiTrigger_.buildDefaultMappings((int)chopEngine_.getRegions().size());
        ChopSnapshot snap;
        snap.chopPoints  = chopEngine_.getChopPoints();
        snap.description = "Chop change";
        undoHistory_.push(snap);
    };
}

void VocalChopperProcessor::analyzeAllPitches()
{
    const auto* buf = getActiveBuffer();
    if (!buf) return;
    regionPitches_ = pitchAnalyzer_.analyzeAllRegions(
        *buf, vocalSampleRate_, chopEngine_.getRegions());
}

// FIX BUG 1: declare activeBuffer properly in scope
double VocalChopperProcessor::getPlayheadPosition() const
{
    const auto* buf = getActiveBuffer();
    if (!buf || buf->getNumSamples() == 0) return 0.0;

    const auto& regions = chopEngine_.getRegions();
    if (regions.empty() || activeRegionIdx_ < 0 ||
        activeRegionIdx_ >= (int)regions.size()) return 0.0;

    const auto& r     = regions[(size_t)activeRegionIdx_];
    double regionLen  = r.endSec - r.startSec;
    if (regionLen <= 0.0) return 0.0;

    double totalDur = (double)buf->getNumSamples() / vocalSampleRate_;
    if (totalDur  <= 0.0) return 0.0;

    return (r.startSec + (double)playheadSample_ / vocalSampleRate_) / totalDur;
}

void VocalChopperProcessor::getStateInformation(juce::MemoryBlock& dest)
{
    auto state = params.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, dest);
}

void VocalChopperProcessor::setStateInformation(const void* data, int size)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, size));
    if (xml && xml->hasTagName(params.state.getType()))
        params.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VocalChopperProcessor();
}
