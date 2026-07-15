#pragma once

#include "PluginHost.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include <memory>
#include <vector>

namespace mixdeck {

/** Story 4.3 — an ordered chain of real plugin instances, shared by Deck (per
    deck) and MasterBus (bus master). Real-time-safe by construction: the
    audio thread only ever does a single atomic load of a raw
    `const ChainSnapshot*` and then processes whatever it got — never a lock,
    never an allocation, never a shared_ptr refcount touch. Ownership of every
    "generation" of the chain is kept separately, in a control-side-only
    vector of shared_ptr (`generations`); a generation is only actually freed
    once at least maxRetiredGenerations newer ones have been published, which
    gives any audio callback that grabbed the raw pointer just before a
    mutation ample time (many buffers' worth) to finish using it — so
    destruction (which can be arbitrarily slow for a misbehaving plugin) never
    happens on the audio thread. This grace-period scheme is used instead of
    the "obvious" std::atomic<std::shared_ptr<const ChainSnapshot>> (a
    standard C++20 specialization) because Apple's shipped libc++ does not yet
    implement that partial specialization: it silently falls back to the
    primary atomic<T> template, which requires T to be trivially copyable and
    fails to compile for shared_ptr. Confirmed by trying it first and hitting
    exactly that static_assert. See Story 4.3 plan for the full reasoning. */
class PluginChain {
public:
    PluginChain(PluginHost& pluginHostToUse, juce::ThreadPool& backgroundPoolToUse);

    void prepare(double sampleRate, int maxBlockSize);
    void releaseResources();

    // Audio thread. No locks, no allocation.
    void process(const juce::AudioSourceChannelInfo& bufferToFill);

    // Async: instantiation can be slow (disk I/O, plugin init) — runs on the
    // shared background pool, never the audio or message/IPC thread. Same
    // "in progress" polling convention as PluginHost::scanForPlugins (3.2/4.1).
    void addPlugin(const juce::String& pluginIdentifier);
    bool isAddingPlugin() const { return addingPlugin; }
    juce::String getLastAddError() const;

    // Synchronous — cheap mutation of the existing snapshot, no instantiation.
    void removePlugin(int index);
    void movePlugin(int fromIndex, int toIndex);
    void setPluginBypassed(int index, bool bypassed);

    // Message-thread only, like any juce::Component. Reuses the embedding
    // mechanism proven in Story 4.0 (Component::addToDesktop with a foreign
    // host window handle).
    void showPluginEditor(int index, void* hostWindowHandle);
    void hidePluginEditor(int index);

    struct SlotInfo {
        juce::String name;
        bool bypassed;
    };
    std::vector<SlotInfo> getSlots() const;

private:
    // Wraps a real AudioProcessorEditor with a thin native header bar (a real
    // juce::TextButton, not a DOM element) carrying a "minimize" button.
    // Necessary because the editor is embedded as a child NSView directly on
    // Electron's window content (Story 4.0's addToDesktop(0, hostWindowHandle)
    // mechanism): that view composites ABOVE Chromium's rendering surface
    // regardless of CSS z-index, so it can visually cover and steal clicks
    // from our own DOM UI (including a DOM-drawn minimize button) whenever
    // the plugin's editor is large. Only a control living in the same native
    // view hierarchy as the editor is guaranteed reachable. The minimize
    // button needs no external callback/captured index (and so no risk of
    // acting on a stale index after a reorder) — it just hides itself.
    class EditorHost : public juce::Component {
    public:
        EditorHost(std::unique_ptr<juce::AudioProcessorEditor> editorToOwn, juce::String pluginName);

        void resized() override;
        void paint(juce::Graphics& g) override;
        // Some plugins resize their own editor (e.g. a drag handle) — without
        // this, EditorHost would stay at its creation size and clip/underfill.
        void childBoundsChanged(juce::Component* child) override;

    private:
        static constexpr int headerHeight = 24;
        std::unique_ptr<juce::AudioProcessorEditor> editor;
        juce::String name; // AudioProcessorEditor's own Component name isn't the plugin's name — kept separately for the header label
        juce::TextButton minimizeButton;
    };

    struct Slot {
        std::unique_ptr<juce::AudioPluginInstance> instance;
        std::unique_ptr<EditorHost> editorHost;
        std::atomic<bool> bypassed { false }; // read by the audio thread, written by the control/message thread
        juce::String name;
    };
    struct ChainSnapshot {
        std::vector<std::shared_ptr<Slot>> slots;
    };

    // Publishes `next`, retiring the oldest generation once the grace period
    // has elapsed. Assumes controlMutex is already held by the caller —
    // addPlugin()'s background-pool completion and removePlugin()/
    // movePlugin() (called on the message thread via the Bridge) can
    // otherwise race on activeChain/generations, since they run on genuinely
    // different threads (the audio thread is never involved here, only two
    // possible control-side callers).
    void publishLocked(std::shared_ptr<const ChainSnapshot> next);

    PluginHost& pluginHost;
    juce::ThreadPool& backgroundPool;

    // Audio thread reads this — a plain lock-free pointer load, no refcount
    // traffic. Never dereferenced after the object it points to could have
    // been freed (see `generations` below).
    std::atomic<const ChainSnapshot*> activeChain { nullptr };
    std::mutex controlMutex; // serializes mutations against each other — never touched by the audio thread
    // Owns every generation still possibly in use, oldest first; control-side
    // only, never touched by the audio thread. Trimmed from the front in
    // publishLocked() once more than maxRetiredGenerations newer generations
    // exist, which is the grace period backing activeChain's raw pointer.
    std::vector<std::shared_ptr<const ChainSnapshot>> generations;
    static constexpr size_t maxRetiredGenerations = 2;

    std::atomic<bool> addingPlugin { false };
    mutable std::mutex addErrorMutex;
    juce::String lastAddError;

    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;

    juce::MidiBuffer scratchMidiBuffer; // always empty — never fed, so never reallocates

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginChain)
};

} // namespace mixdeck
