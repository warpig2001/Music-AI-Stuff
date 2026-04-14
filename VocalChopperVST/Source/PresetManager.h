#pragma once
#include <JuceHeader.h>

// ─────────────────────────────────────────────────────────────────────────────
// PresetManager
// Saves/loads plugin state as XML files in the user's preset folder.
// Also manages a built-in factory preset list.
// ─────────────────────────────────────────────────────────────────────────────
class PresetManager
{
public:
    explicit PresetManager(juce::AudioProcessorValueTreeState& apvts)
        : apvts_(apvts)
    {
        // Create preset directory if it doesn't exist
        getPresetDir().createDirectory();
        refreshPresetList();
    }

    // ── Directory ─────────────────────────────────────────────────────────
    static juce::File getPresetDir()
    {
        return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                   .getChildFile("VocalChopper")
                   .getChildFile("Presets");
    }

    // ── Preset list ───────────────────────────────────────────────────────
    void refreshPresetList()
    {
        presetFiles_.clear();

        // Factory presets first (built-in)
        for (const auto& name : getFactoryPresetNames())
            presetFiles_.push_back({ name, {}, true });

        // User presets from disk
        for (const auto& f : getPresetDir().findChildFiles(
                 juce::File::findFiles, false, "*.vcpreset"))
        {
            presetFiles_.push_back({ f.getFileNameWithoutExtension(), f, false });
        }

        if (onPresetListChanged) onPresetListChanged();
    }

    int  getNumPresets()              const { return (int)presetFiles_.size(); }
    int  getCurrentIndex()            const { return currentIndex_; }

    juce::String getPresetName(int i) const
    {
        if (i < 0 || i >= (int)presetFiles_.size()) return "—";
        return presetFiles_[(size_t)i].name;
    }

    juce::String getCurrentName() const { return getPresetName(currentIndex_); }

    // ── Load ──────────────────────────────────────────────────────────────
    bool loadPreset(int index)
    {
        if (index < 0 || index >= (int)presetFiles_.size()) return false;
        const auto& entry = presetFiles_[(size_t)index];

        if (entry.isFactory)
            return loadFactoryPreset(entry.name);

        if (!entry.file.existsAsFile()) return false;

        auto xml = juce::XmlDocument::parse(entry.file);
        if (!xml) return false;

        if (xml->hasTagName("VocalChopperPreset"))
        {
            if (auto* params = xml->getChildByName("Parameters"))
            {
                auto tree = juce::ValueTree::fromXml(*params);
                apvts_.replaceState(tree);
            }
        }

        currentIndex_ = index;
        if (onPresetChanged) onPresetChanged(entry.name);
        return true;
    }

    void loadPrev() { if (getNumPresets() > 0) loadPreset((currentIndex_ - 1 + getNumPresets()) % getNumPresets()); }
    void loadNext() { if (getNumPresets() > 0) loadPreset((currentIndex_ + 1) % getNumPresets()); }

    // ── Save ──────────────────────────────────────────────────────────────
    bool savePreset(const juce::String& name)
    {
        if (name.isEmpty()) return false;

        juce::String safeName = name.replaceCharacters("\\/:*?\"<>|", "_________");
        juce::File dest = getPresetDir().getChildFile(safeName + ".vcpreset");

        auto root  = std::make_unique<juce::XmlElement>("VocalChopperPreset");
        root->setAttribute("name",    name);
        root->setAttribute("version", "1");

        // Save APVTS parameters
        auto state = apvts_.copyState();
        if (auto paramsXml = state.createXml())
        {
            paramsXml->setTagName("Parameters");
            root->addChildElement(paramsXml.release());
        }

        if (!root->writeTo(dest)) return false;

        refreshPresetList();

        // Select the newly saved preset
        for (int i = 0; i < getNumPresets(); ++i)
            if (presetFiles_[(size_t)i].name == name)
                { currentIndex_ = i; break; }

        if (onPresetChanged) onPresetChanged(name);
        return true;
    }

    bool deleteCurrentPreset()
    {
        if (currentIndex_ < 0 || currentIndex_ >= (int)presetFiles_.size()) return false;
        auto& entry = presetFiles_[(size_t)currentIndex_];
        if (entry.isFactory) return false;   // never delete factory presets
        entry.file.deleteFile();
        refreshPresetList();
        loadPreset(juce::jlimit(0, getNumPresets() - 1, currentIndex_));
        return true;
    }

    // ── Callbacks ─────────────────────────────────────────────────────────
    std::function<void(const juce::String&)> onPresetChanged;
    std::function<void()>                    onPresetListChanged;

private:
    juce::AudioProcessorValueTreeState& apvts_;
    int currentIndex_ = 0;

    struct PresetEntry {
        juce::String name;
        juce::File   file;
        bool         isFactory = false;
    };
    std::vector<PresetEntry> presetFiles_;

    // ── Factory presets ───────────────────────────────────────────────────
    static std::vector<juce::String> getFactoryPresetNames()
    {
        return {
            "Default",
            "Stutter 8th",
            "Trap 16th",
            "Half-Time Verse",
            "Quick Chops",
            "Slow Fade",
        };
    }

    bool loadFactoryPreset(const juce::String& name)
    {
        // Reset all params to defaults first
        for (auto* p : apvts_.processor.getParameters())
            if (auto* rp = dynamic_cast<juce::RangedAudioParameter*>(p))
                rp->setValueNotifyingHost(rp->getDefaultValue());

        // Apply preset-specific overrides
        auto set = [&](const juce::String& id, float v) {
            if (auto* p = apvts_.getRawParameterValue(id)) *p = v;
        };

        if (name == "Stutter 8th")
        {
            set("fadeInMs",  5.f);
            set("fadeOutMs", 8.f);
            set("bpm",      140.f);
        }
        else if (name == "Trap 16th")
        {
            set("fadeInMs",  3.f);
            set("fadeOutMs", 6.f);
            set("bpm",      140.f);
        }
        else if (name == "Half-Time Verse")
        {
            set("fadeInMs",  20.f);
            set("fadeOutMs", 40.f);
            set("bpm",       70.f);
        }
        else if (name == "Quick Chops")
        {
            set("fadeInMs",  2.f);
            set("fadeOutMs", 4.f);
        }
        else if (name == "Slow Fade")
        {
            set("fadeInMs",  400.f);
            set("fadeOutMs", 600.f);
        }

        currentIndex_ = 0;
        for (int i = 0; i < getNumPresets(); ++i)
            if (presetFiles_[(size_t)i].name == name)
                { currentIndex_ = i; break; }

        if (onPresetChanged) onPresetChanged(name);
        return true;
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetManager)
};
