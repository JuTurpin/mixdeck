#include "IsolatedPluginHost.h"

namespace mixdeck {

// ---- BlockFifo -------------------------------------------------------------

void IsolatedPluginHost::BlockFifo::prepare(int numSlots, int numChannelsToUse, int blockSizeToUse) {
    numChannels = numChannelsToUse;
    blockSize = blockSizeToUse;
    storage.allocate((size_t) numSlots * (size_t) numChannels * (size_t) blockSize, true);
    fifo.setTotalSize(numSlots);
}

void IsolatedPluginHost::BlockFifo::release() {
    fifo.reset();
    storage.free();
    numChannels = 0;
    blockSize = 0;
}

float* IsolatedPluginHost::BlockFifo::slotChannelPointer(int slotIndex, int channel) {
    return storage.getData() + (size_t) slotIndex * (size_t) numChannels * (size_t) blockSize
           + (size_t) channel * (size_t) blockSize;
}

bool IsolatedPluginHost::BlockFifo::push(const juce::AudioBuffer<float>& buffer) {
    int start1, size1, start2, size2;
    fifo.prepareToWrite(1, start1, size1, start2, size2);
    if (size1 == 0)
        return false; // full — caller drops this block rather than waiting

    const auto samplesToCopy = juce::jmin(blockSize, buffer.getNumSamples());
    const auto channelsToCopy = juce::jmin(numChannels, buffer.getNumChannels());
    for (int channel = 0; channel < channelsToCopy; ++channel)
        std::memcpy(slotChannelPointer(start1, channel), buffer.getReadPointer(channel),
                    (size_t) samplesToCopy * sizeof(float));

    fifo.finishedWrite(1);
    return true;
}

bool IsolatedPluginHost::BlockFifo::pop(juce::AudioBuffer<float>& buffer) {
    int start1, size1, start2, size2;
    fifo.prepareToRead(1, start1, size1, start2, size2);
    if (size1 == 0)
        return false; // nothing ready yet

    const auto samplesToCopy = juce::jmin(blockSize, buffer.getNumSamples());
    const auto channelsToCopy = juce::jmin(numChannels, buffer.getNumChannels());
    for (int channel = 0; channel < channelsToCopy; ++channel)
        std::memcpy(buffer.getWritePointer(channel), slotChannelPointer(start1, channel),
                    (size_t) samplesToCopy * sizeof(float));

    fifo.finishedRead(1);
    return true;
}

// ---- SendPumpThread ---------------------------------------------------------

IsolatedPluginHost::SendPumpThread::SendPumpThread(IsolatedPluginHost& ownerToUse)
    : Thread("MixDeck Plugin Worker Pump"), owner(ownerToUse) {}

void IsolatedPluginHost::SendPumpThread::run() {
    while (!threadShouldExit()) {
        while (owner.toWorkerFifo.pop(owner.outgoingScratch)) {
            if (threadShouldExit())
                return;
            owner.sendMessageToWorker(makeAudioBlockMessage(owner.outgoingScratch));
        }
        wait(1);
    }
}

// ---- IsolatedPluginHost ------------------------------------------------------

IsolatedPluginHost::~IsolatedPluginHost() {
    // Stop the pump thread before anything else — including before the base
    // ChildProcessCoordinator (destroyed right after this body runs) starts
    // tearing down the connection it would otherwise still be calling into.
    releaseAudio();
}

void IsolatedPluginHost::prepareAudio(int numChannels, int maxBlockSize) {
    releaseAudio(); // idempotent — tears down any previous session first (e.g. audio device restarted)

    toWorkerFifo.prepare(audioFifoSlots, numChannels, maxBlockSize);
    fromWorkerFifo.prepare(audioFifoSlots, numChannels, maxBlockSize);
    outgoingScratch.setSize(numChannels, maxBlockSize);
    incomingScratch.setSize(numChannels, maxBlockSize);

    pumpThread = std::make_unique<SendPumpThread>(*this);
    pumpThread->startThread(juce::Thread::Priority::high);
}

void IsolatedPluginHost::releaseAudio() {
    if (pumpThread != nullptr) {
        pumpThread->signalThreadShouldExit();
        pumpThread->stopThread(2000);
        pumpThread.reset();
    }
    toWorkerFifo.release();
    fromWorkerFifo.release();
}

void IsolatedPluginHost::pushInputBlock(const juce::AudioBuffer<float>& buffer) {
    toWorkerFifo.push(buffer);
}

bool IsolatedPluginHost::popOutputBlock(juce::AudioBuffer<float>& buffer) {
    return fromWorkerFifo.pop(buffer);
}

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
    } else if (type == WorkerMessageType::audioBlock) {
        // Story 4.4.2 — deliberately NOT marshalled via callAsync: this is
        // the real-time reply path (a processed audio block), and
        // MessageManager::callAsync would reintroduce message-thread
        // scheduling jitter into the audio pipeline it exists to avoid.
        // Always called on this connection's own dedicated IPC delivery
        // thread (never the audio thread, never concurrently with itself),
        // so decoding straight into incomingScratch and publishing through
        // the lock-free fromWorkerFifo is safe without further locking.
        if (readAudioBlockMessage(message, incomingScratch))
            fromWorkerFifo.push(incomingScratch);
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
