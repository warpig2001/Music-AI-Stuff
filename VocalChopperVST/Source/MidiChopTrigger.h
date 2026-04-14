#pragma once
#include <JuceHeader.h>
#include "ChopEngine.h"

// Maps MIDI notes to chop regions and fires them from the processBlock.
// Default mapping: C2 (MIDI 36) = region 1, C#2 = region 2, etc.
// (Matches the default mapping of Akai MPC / FL Studio FPC pads)

struct MidiChopMapping
{
    int midiNote   = 36;   // C2
    int regionIdx  = 0;
    float velocity = 1.f;  // Scales the output gain
    bool  active   = true;
};

class MidiChopTrigger
{
public:
    MidiChopTrigger();

    // Set/get mappings
    void  setMapping(int slot, int midiNote, int regionIdx);
    const std::vector<MidiChopMapping>& getMappings() const { return mappings_; }

    // Rebuild default mappings from C2 upward
    void buildDefaultMappings(int numRegions);

    // Call from processBlock — returns list of regions triggered this block
    // along with their velocity. Clears triggered state afterward.
    struct TriggerEvent { int regionIdx; float velocity; int sampleOffset; };
    std::vector<TriggerEvent> processMidi(const juce::MidiBuffer& midi,
                                           const ChopEngine& engine);

    // Returns note name for display (e.g. "C2")
    static juce::String midiNoteToName(int note);

    // MIDI learn: call startLearning(slot), then next MIDI note will bind
    void startLearning(int slot);
    void stopLearning();
    bool isLearning() const { return learningSlot_ >= 0; }
    int  getLearningSlot() const { return learningSlot_; }

    // Callback: fired when MIDI learn binds a new note
    std::function<void(int slot, int newNote)> onLearnComplete;

private:
    std::vector<MidiChopMapping> mappings_;
    int learningSlot_ = -1;

    void processLearnNote(int note, int slot);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiChopTrigger)
};
