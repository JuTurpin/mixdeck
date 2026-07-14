#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <mutex>
#include <vector>

namespace mixdeck {

/** Story 4.1 — discovery only (no loading into a chain yet, see Story 4.3).
    Scans the standard VST3/AU folders (ADR-003) via JUCE's own
    AudioPluginFormat::getDefaultLocationsToSearch(), no hardcoded paths. */
class PluginHost {
public:
    PluginHost(); // registers VST3 + AU formats (JUCE_PLUGINHOST_VST3/AU, CMakeLists.txt)

    // Blocking — caller is responsible for running this off the audio/UI
    // thread (see Engine::startPluginScan, which uses the shared analysisPool
    // already used for BPM analysis, Story 3.2). No deadMansPedalFile / no
    // out-of-process isolation yet: an unstable plugin could crash the scan —
    // accepted for this story, isolation is explicitly Story 4.4.
    void scanForPlugins();

    // Thread-safe snapshot; empty until the first scan completes.
    std::vector<juce::PluginDescription> getAvailablePlugins() const;

    // Story 4.2 (ADR-004) — manual path selection. Synchronous, like
    // Deck::loadFile(): a single file to inspect, not a folder scan. Only
    // VST3 genuinely supports loading from an arbitrary path (Audio Unit
    // plugins can only be identified once already registered with macOS's
    // Component Manager — confirmed in JUCE's own doc comments — so AU stays
    // scan-only, Story 4.1). Returns an error message, "" on success.
    juce::String addPluginFromPath(const juce::String& path);

private:
    juce::AudioPluginFormatManager formatManager;
    mutable std::mutex mutex;
    std::vector<juce::PluginDescription> availablePlugins; // guarded by mutex

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginHost)
};

} // namespace mixdeck
