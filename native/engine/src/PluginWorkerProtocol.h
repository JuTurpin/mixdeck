#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace mixdeck {

// Story 4.4.1 — wire format shared by the main process (IsolatedPluginHost,
// juce::ChildProcessCoordinator) and MixDeckPluginWorker (juce::
// ChildProcessWorker). Every message is a juce::MemoryBlock built with these
// helpers: one leading byte selects the type below, the rest is type-specific
// payload. JUCE's own ping/kill/handshake messages never reach
// handleMessageFromWorker/handleMessageFromCoordinator (filtered internally
// by ChildProcessCoordinator/Worker), so callers only ever see messages built
// by these helpers on the other side — no need to defend against malformed
// input here beyond a basic empty-block guard.
enum class WorkerMessageType : uint8_t {
    // Coordinator -> worker
    loadPlugin = 1,
    showEditor = 2,
    hideEditor = 3,
    // Worker -> coordinator
    pluginLoaded = 10,
    pluginLoadFailed = 11,
};

WorkerMessageType peekMessageType(const juce::MemoryBlock& message);

// loadPlugin: the full PluginDescription travels across the IPC boundary
// (serialized via its own createXml()/loadFromXml(), the same mechanism
// KnownPluginList uses to persist plugins to disk) since the worker's own
// AudioPluginFormatManager never scanned anything and has no list to look an
// identifier up in — it needs every field to instantiate directly.
juce::MemoryBlock makeLoadPluginMessage(const juce::PluginDescription& description, double sampleRate, int blockSize);
bool readLoadPluginMessage(const juce::MemoryBlock& message, juce::PluginDescription& outDescription,
                            double& outSampleRate, int& outBlockSize);

// showEditor / hideEditor / pluginLoaded carry no payload beyond the type byte.
juce::MemoryBlock makeSimpleMessage(WorkerMessageType type);

juce::MemoryBlock makePluginLoadFailedMessage(const juce::String& error);
juce::String readErrorMessage(const juce::MemoryBlock& message);

} // namespace mixdeck
