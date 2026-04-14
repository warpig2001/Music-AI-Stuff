#pragma once
#include <JuceHeader.h>
#include <vector>
#include <functional>

// Snapshot of the chop engine state — enough to fully restore it
struct ChopSnapshot
{
    std::vector<double> chopPoints;
    juce::String        description;   // e.g. "Auto transients", "Added chop at 1.23s"
};

class UndoHistory
{
public:
    static constexpr int kMaxHistory = 40;

    UndoHistory() { history_.reserve(kMaxHistory + 2); }

    // Push a new snapshot. Clears any redo states beyond current position.
    void push(const ChopSnapshot& snap)
    {
        // Trim redo tail
        if (cursor_ < (int)history_.size() - 1)
            history_.erase(history_.begin() + cursor_ + 1, history_.end());

        history_.push_back(snap);

        // Evict oldest if over limit
        if ((int)history_.size() > kMaxHistory)
        {
            history_.erase(history_.begin());
        }
        cursor_ = (int)history_.size() - 1;

        if (onHistoryChanged) onHistoryChanged();
    }

    bool canUndo() const { return cursor_ > 0; }
    bool canRedo() const { return cursor_ < (int)history_.size() - 1; }

    const ChopSnapshot& undo()
    {
        if (canUndo()) --cursor_;
        if (onHistoryChanged) onHistoryChanged();
        return history_[(size_t)cursor_];
    }

    const ChopSnapshot& redo()
    {
        if (canRedo()) ++cursor_;
        if (onHistoryChanged) onHistoryChanged();
        return history_[(size_t)cursor_];
    }

    const ChopSnapshot& current() const
    {
        jassert(!history_.empty());
        return history_[(size_t)cursor_];
    }

    int  size()   const { return (int)history_.size(); }
    int  cursor() const { return cursor_; }

    const std::vector<ChopSnapshot>& getHistory() const { return history_; }

    void clear()
    {
        history_.clear();
        cursor_ = -1;
        if (onHistoryChanged) onHistoryChanged();
    }

    std::function<void()> onHistoryChanged;

private:
    std::vector<ChopSnapshot> history_;
    int cursor_ = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UndoHistory)
};
