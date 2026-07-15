#pragma once

#include "PluginWorkerProtocol.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_events/juce_events.h>
#include <functional>
#include <memory>

namespace mixdeck {

/** Story 4.4.1 — main-process side of a single isolated VST3 plugin instance:
    owns a juce::ChildProcessCoordinator driving a dedicated MixDeckPluginWorker
    process (one process per isolated plugin instance, so one crashing plugin
    can never take another one down with it). Story 4.4.2 adds the real-time
    audio path: the audio thread never touches the IPC connection directly
    (its message channel copies/allocates per send, unsuitable for the audio
    thread) — instead it only ever reads/writes two small preallocated
    lock-free queues (pushInputBlock/popOutputBlock); a dedicated background
    "pump" thread drains the outgoing queue and does the actual
    sendMessageToWorker() calls, and incoming replies are decoded straight
    into the other queue from JUCE's own IPC delivery thread
    (handleMessageFromWorker) — neither of those is the audio thread. */
class IsolatedPluginHost : public juce::ChildProcessCoordinator {
public:
    IsolatedPluginHost() = default;
    ~IsolatedPluginHost() override;

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

    // Story 4.4.2 — allocates the audio pipeline (fixed-capacity lock-free
    // queues + scratch buffers, all preallocated here, never inside
    // pushInputBlock/popOutputBlock) and starts the send-pump thread.
    // Callable more than once — e.g. the audio device restarts with a
    // different block size — tears down any previous session first.
    void prepareAudio(int numChannels, int maxBlockSize);
    void releaseAudio();

    // Audio thread only, both non-blocking and non-allocating.
    // pushInputBlock drops the block if the queue to the worker is already
    // full (the worker isn't keeping up) rather than ever waiting.
    // popOutputBlock returns false — leaving `buffer` untouched, so whatever
    // dry signal was already there just passes through — if no processed
    // block is ready yet (nothing sent so far, or the worker has crashed).
    void pushInputBlock(const juce::AudioBuffer<float>& buffer);
    bool popOutputBlock(juce::AudioBuffer<float>& buffer);

private:
    void handleMessageFromWorker(const juce::MemoryBlock& message) override;
    void handleConnectionLost() override;

    std::function<void(bool, juce::String)> pendingLoadResult;
    bool crashed = false;

    // A lock-free single-reader/single-writer queue of whole audio blocks.
    // juce::AbstractFifo manages only the read/write indices; the actual
    // sample storage is a flat preallocated array here, sized once in
    // prepare() — never resized or allocated from push()/pop().
    class BlockFifo {
    public:
        void prepare(int numSlots, int numChannelsToUse, int blockSizeToUse);
        void release();

        bool push(const juce::AudioBuffer<float>& buffer);
        bool pop(juce::AudioBuffer<float>& buffer);

    private:
        float* slotChannelPointer(int slotIndex, int channel);

        juce::AbstractFifo fifo { 1 };
        juce::HeapBlock<float> storage;
        int numChannels = 0;
        int blockSize = 0;
    };

    // Drains toWorkerFifo and does the actual sendMessageToWorker() calls —
    // the only thread allowed to touch the IPC connection for audio data.
    // Safe off the message thread: ChildProcessCoordinator's own internal
    // ping thread already calls sendMessageToWorker() from its own thread
    // (see juce_ConnectedChildProcess.cpp), proving the API is designed for
    // exactly this.
    class SendPumpThread : public juce::Thread {
    public:
        explicit SendPumpThread(IsolatedPluginHost& ownerToUse);
        void run() override;

    private:
        IsolatedPluginHost& owner;
    };

    static constexpr int audioFifoSlots = 4; // ~4 blocks of slack before pushInputBlock starts dropping

    BlockFifo toWorkerFifo;   // audio thread writes, SendPumpThread reads
    BlockFifo fromWorkerFifo; // handleMessageFromWorker (IPC thread) writes, audio thread reads
    juce::AudioBuffer<float> outgoingScratch; // SendPumpThread only
    juce::AudioBuffer<float> incomingScratch; // IPC delivery thread only (handleMessageFromWorker)
    std::unique_ptr<SendPumpThread> pumpThread;

    JUCE_DECLARE_WEAK_REFERENCEABLE(IsolatedPluginHost)
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(IsolatedPluginHost)
};

} // namespace mixdeck
