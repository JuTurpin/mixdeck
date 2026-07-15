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

    // Story 4.3 — instantiates a real plugin instance from a plugin found by
    // scanForPlugins()/addPluginFromPath() (looked up by
    // PluginDescription::createIdentifierString()), ready to process audio at
    // the given sample rate/block size. Blocking (see AudioPluginFormatManager
    // ::createPluginInstance) — safe off the message thread for VST3 and
    // classic AU (AUv2); callers run this on a background thread (PluginChain
    // uses the shared analysisPool) rather than the IPC/message thread.
    // Rejects plugins that don't support a plain stereo in/out layout (e.g.
    // instruments with no audio input) — returns nullptr + error in that case.
    std::unique_ptr<juce::AudioPluginInstance> instantiatePlugin(
        const juce::String& pluginIdentifier, double sampleRate, int blockSize, juce::String& error);

    // Story 4.4.1 — looked up by PluginChain when a plugin needs to be handed
    // off to an isolated worker process (VST3 only) instead of instantiated
    // in-process: the full description travels across the IPC boundary (see
    // PluginWorkerProtocol), since the worker's own PluginHost never scanned
    // anything and has no list to look the identifier up in.
    bool findPluginDescription(const juce::String& pluginIdentifier, juce::PluginDescription& outDescription) const;

    // Story 4.4.1 — where to find the MixDeckPluginWorker executable used for
    // VST3 isolation. Defaults to a best-effort guess relative to this
    // process's own executable (correct for the standalone harness, which is
    // built in the same CMake tree); Electron overrides this explicitly at
    // startup (setPluginWorkerExecutablePath, NodeBinding.cpp) since a Node/
    // Electron host's own executable path has nothing to do with the CMake
    // build layout, the same reasoning as why hostWindowHandle is injected
    // rather than auto-discovered.
    juce::File getWorkerExecutablePath() const { return workerExecutablePath; }
    void setWorkerExecutablePath(const juce::File& path) { workerExecutablePath = path; }

private:
    static juce::File guessDefaultWorkerExecutablePath();

    juce::AudioPluginFormatManager formatManager;
    mutable std::mutex mutex;
    std::vector<juce::PluginDescription> availablePlugins; // guarded by mutex
    juce::File workerExecutablePath { guessDefaultWorkerExecutablePath() };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginHost)
};

} // namespace mixdeck
