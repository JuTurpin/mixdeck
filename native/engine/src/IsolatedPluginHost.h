#pragma once

#include "PluginWorkerProtocol.h"
#include <juce_events/juce_events.h>
#include <functional>

namespace mixdeck {

/** Story 4.4.1 — main-process side of a single isolated VST3 plugin instance:
    owns a juce::ChildProcessCoordinator driving a dedicated MixDeckPluginWorker
    process (one process per isolated plugin instance, so one crashing plugin
    can never take another one down with it). This story covers lifecycle only
    (load, crash detection, editor window) — no audio flows through this yet;
    PluginChain treats an isolated slot as bypassed on the real-time path
    until Story 4.4.2 adds the actual audio IPC. */
class IsolatedPluginHost : public juce::ChildProcessCoordinator {
public:
    IsolatedPluginHost() = default;

    // Async — spawns the worker process (if not already running) and asks it
    // to load the plugin. onResult is called on the message thread once the
    // worker replies, or immediately if the process fails to even start —
    // same "in progress"/error-string convention as PluginChain::addPlugin.
    void load(const juce::File& workerExecutable, const juce::PluginDescription& description, double sampleRate,
              int blockSize, std::function<void(bool success, juce::String error)> onResult);

    void showEditor();
    void hideEditor();

    // True once the worker process has died unexpectedly (handleConnectionLost).
    bool hasCrashed() const { return crashed; }
    // Called on the message thread when that happens — PluginChain uses this
    // to mark the slot as failed instead of silently doing nothing.
    std::function<void()> onCrashed;

private:
    void handleMessageFromWorker(const juce::MemoryBlock& message) override;
    void handleConnectionLost() override;

    std::function<void(bool, juce::String)> pendingLoadResult;
    bool crashed = false;

    JUCE_DECLARE_WEAK_REFERENCEABLE(IsolatedPluginHost)
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(IsolatedPluginHost)
};

} // namespace mixdeck
