#include "MidiChopTrigger.h"

MidiChopTrigger::MidiChopTrigger()
{
    mappings_.reserve(32);
}

void MidiChopTrigger::setMapping(int slot, int midiNote, int regionIdx)
{
    while ((int)mappings_.size() <= slot)
        mappings_.push_back({36 + (int)mappings_.size(), (int)mappings_.size(), 1.f, true});

    mappings_[(size_t)slot].midiNote  = midiNote;
    mappings_[(size_t)slot].regionIdx = regionIdx;
}

void MidiChopTrigger::buildDefaultMappings(int numRegions)
{
    mappings_.clear();
    // Map regions starting at C2 (MIDI 36), matching FL Studio FPC pad layout
    for (int i = 0; i < numRegions; ++i)
    {
        MidiChopMapping m;
        m.midiNote  = 36 + i;
        m.regionIdx = i;
        m.velocity  = 1.f;
        m.active    = true;
        mappings_.push_back(m);
    }
}

std::vector<MidiChopTrigger::TriggerEvent>
MidiChopTrigger::processMidi(const juce::MidiBuffer& midi, const ChopEngine& engine)
{
    std::vector<TriggerEvent> events;
    const auto& regions = engine.getRegions();

    for (const auto meta : midi)
    {
        auto msg = meta.getMessage();

        // MIDI learn mode
        if (learningSlot_ >= 0 && msg.isNoteOn())
        {
            processLearnNote(msg.getNoteNumber(), learningSlot_);
            learningSlot_ = -1;
            continue;
        }

        if (!msg.isNoteOn()) continue;

        int note = msg.getNoteNumber();
        float vel = msg.getVelocity() / 127.f;

        // Find mapping for this note
        for (const auto& m : mappings_)
        {
            if (m.active && m.midiNote == note && m.regionIdx < (int)regions.size())
            {
                TriggerEvent ev;
                ev.regionIdx   = m.regionIdx;
                ev.velocity    = vel * m.velocity;
                ev.sampleOffset = meta.samplePosition;
                events.push_back(ev);
                break;
            }
        }
    }

    return events;
}

void MidiChopTrigger::processLearnNote(int note, int slot)
{
    if (slot < 0 || slot >= (int)mappings_.size()) return;
    int oldNote = mappings_[(size_t)slot].midiNote;
    mappings_[(size_t)slot].midiNote = note;
    (void)oldNote;
    if (onLearnComplete) onLearnComplete(slot, note);
}

void MidiChopTrigger::startLearning(int slot)
{
    learningSlot_ = slot;
}

void MidiChopTrigger::stopLearning()
{
    learningSlot_ = -1;
}

juce::String MidiChopTrigger::midiNoteToName(int note)
{
    static const char* names[] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
    int octave = note / 12 - 1;
    return juce::String(names[note % 12]) + juce::String(octave);
}
