#pragma once

#include "Deck.h"
#include "MasterBus.h"
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

    // Story 4.2 (ADR-004) — manual path selection, VST3 only (see PluginHost).
    juce::String addPluginFromPath(const juce::String& path) { return pluginHost.addPluginFromPath(path); }

    // Story 4.4.1 — where to find the MixDeckPluginWorker executable used for
    // VST3 isolation (see PluginHost::setWorkerExecutablePath). Electron
    // injects this explicitly at startup; the standalone harness relies on
    // PluginHost's own best-effort default instead.
    void setPluginWorkerExecutablePath(const juce::String& path) {
        pluginHost.setWorkerExecutablePath(juce::File(path));
    }

    // Story 4.3 — bus master effects chain (after deckA/deckB are mixed).
    MasterBus& getMasterBus() { return masterBus; }

private:
    juce::AudioDeviceManager deviceManager;
    juce::AudioFormatManager formatManager;
    // Story 3.2 — shared by both decks (and plugin instantiation, Story 4.3):
    // at most 2 BPM analyses / plugin loads run at once. Declared before
    // deckA/deckB/pluginHost's dependents so it's constructed first (member
    // init order follows declaration order, not the list below).
    juce::ThreadPool analysisPool { 2 };
    // Story 4.1 — discovery/instantiation shared by both decks and the
    // master bus. Declared before deckA/deckB: they need it at construction.
    PluginHost pluginHost;
    Deck deckA { formatManager, analysisPool, pluginHost };
    Deck deckB { formatManager, analysisPool, pluginHost };
    juce::MixerAudioSource audioMixer;
    Mixer mixer { deckA, deckB };
    // Story 4.3 — wraps audioMixer with its own effects chain; audioSourcePlayer
    // pulls from this instead of audioMixer directly (see Engine.cpp).
    MasterBus masterBus { audioMixer, pluginHost, analysisPool };
    juce::AudioSourcePlayer audioSourcePlayer;

    std::atomic<bool> pluginScanInProgress { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Engine)
};

} // namespace mixdeck
