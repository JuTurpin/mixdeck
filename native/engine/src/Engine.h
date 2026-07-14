#pragma once

#include "Deck.h"
#include "Mixer.h"
#include "PluginHost.h"
#include <atomic>

namespace mixdeck {

/** Headless equivalent of the standalone-app's MainComponent audio graph
    (deckA/deckB -> MixerAudioSource -> output), without any JUCE GUI
    Component — used by the Node Bridge (Story 2.2), which has no window of
    its own. MainComponent is untouched and keeps driving the JUCE test
    harness. */
class Engine {
public:
    Engine();
    ~Engine();

    Deck& getDeck(int index); // 0 = A, 1 = B
    Mixer& getMixer() { return mixer; }

    // Story 4.1 — discovery only, no loading into a chain yet (Story 4.3).
    // Runs on the shared analysisPool, same as BPM analysis (Story 3.2).
    void startPluginScan();
    bool isPluginScanInProgress() const { return pluginScanInProgress; }
    std::vector<juce::PluginDescription> getAvailablePlugins() const { return pluginHost.getAvailablePlugins(); }

private:
    juce::AudioDeviceManager deviceManager;
    juce::AudioFormatManager formatManager;
    // Story 3.2 — shared by both decks: at most 2 BPM analyses run at once,
    // one per deck. Declared before deckA/deckB so it's constructed first
    // (member init order follows declaration order, not the list below).
    juce::ThreadPool analysisPool { 2 };
    Deck deckA { formatManager, analysisPool };
    Deck deckB { formatManager, analysisPool };
    juce::MixerAudioSource audioMixer;
    Mixer mixer { deckA, deckB };
    juce::AudioSourcePlayer audioSourcePlayer;

    PluginHost pluginHost;
    std::atomic<bool> pluginScanInProgress { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Engine)
};

} // namespace mixdeck
