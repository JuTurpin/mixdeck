#include "IsolatedPluginHost.h"

namespace mixdeck {

void IsolatedPluginHost::load(const juce::File& workerExecutable, const juce::PluginDescription& description,
                               double sampleRate, int blockSize,
                               std::function<void(bool, juce::String)> onResult) {
    pendingLoadResult = std::move(onResult);

    if (!launchWorkerProcess(workerExecutable, "mixdeckpluginworker", 8000)) {
        if (auto callback = std::exchange(pendingLoadResult, nullptr))
            callback(false, "Impossible de demarrer le process worker (" + workerExecutable.getFullPathName() + ").");
        return;
    }

    sendMessageToWorker(makeLoadPluginMessage(description, sampleRate, blockSize));
}

void IsolatedPluginHost::showEditor() {
    sendMessageToWorker(makeSimpleMessage(WorkerMessageType::showEditor));
}

void IsolatedPluginHost::hideEditor() {
    sendMessageToWorker(makeSimpleMessage(WorkerMessageType::hideEditor));
}

void IsolatedPluginHost::handleMessageFromWorker(const juce::MemoryBlock& message) {
    // "Probably a background thread" per juce::ChildProcessCoordinator's own
    // docs — this object can be destroyed (e.g. the user removes the plugin)
    // before a queued callAsync fires, so guard with a WeakReference rather
    // than capturing `this` directly.
    const auto type = peekMessageType(message);

    if (type == WorkerMessageType::pluginLoaded) {
        juce::WeakReference<IsolatedPluginHost> safeThis(this);
        juce::MessageManager::callAsync([safeThis] {
            if (auto* self = safeThis.get())
                if (auto callback = std::exchange(self->pendingLoadResult, nullptr))
                    callback(true, {});
        });
    } else if (type == WorkerMessageType::pluginLoadFailed) {
        const auto error = readErrorMessage(message);
        juce::WeakReference<IsolatedPluginHost> safeThis(this);
        juce::MessageManager::callAsync([safeThis, error] {
            if (auto* self = safeThis.get())
                if (auto callback = std::exchange(self->pendingLoadResult, nullptr))
                    callback(false, error);
        });
    }
}

void IsolatedPluginHost::handleConnectionLost() {
    juce::WeakReference<IsolatedPluginHost> safeThis(this);
    juce::MessageManager::callAsync([safeThis] {
        if (auto* self = safeThis.get()) {
            self->crashed = true;
            // A crash mid-load means the worker will never reply — resolve
            // the pending callback here instead of leaving it dangling forever.
            if (auto callback = std::exchange(self->pendingLoadResult, nullptr))
                callback(false, "Le process du plugin a plante pendant le chargement.");
            if (self->onCrashed)
                self->onCrashed();
        }
    });
}

} // namespace mixdeck
