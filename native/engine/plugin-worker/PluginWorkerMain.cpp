#include <juce_gui_extra/juce_gui_extra.h>
#include "../src/PluginWorkerProtocol.h"

namespace {

// Story 4.4.1 — child-process side of plugin isolation: loads exactly one
// VST3 plugin (told which one via a LoadPlugin message from
// IsolatedPluginHost) and hosts its real editor as an independent top-level
// window when asked — not incrusted into Electron's window like an in-process
// (AU) plugin, since embedding a view that lives in a different process isn't
// achievable on macOS without fragile private APIs (see Story 4.4.1 plan). If
// this process crashes, only this plugin instance is lost: MixDeck itself
// (and every other deck/plugin) keeps running — that's the whole point.
class PluginWorker : public juce::ChildProcessWorker {
public:
    PluginWorker() { juce::addDefaultFormatsToManager(formatManager); }

    void handleMessageFromCoordinator(const juce::MemoryBlock& message) override {
        using mixdeck::WorkerMessageType;

        switch (mixdeck::peekMessageType(message)) {
            case WorkerMessageType::loadPlugin: {
                juce::PluginDescription description;
                double sampleRate = 44100.0;
                int blockSize = 512;
                if (!mixdeck::readLoadPluginMessage(message, description, sampleRate, blockSize)) {
                    sendMessageToCoordinator(mixdeck::makePluginLoadFailedMessage("Message de chargement invalide."));
                    return;
                }

                // Safe off the message thread for VST3 (see PluginHost::instantiatePlugin,
                // Story 4.3) — this is exactly the format this worker exists for.
                juce::String error;
                instance = formatManager.createPluginInstance(description, sampleRate, blockSize, error);
                if (instance == nullptr) {
                    sendMessageToCoordinator(
                        mixdeck::makePluginLoadFailedMessage(error.isNotEmpty() ? error : "Impossible de charger le plugin."));
                    return;
                }

                // Mirrors PluginHost::instantiatePlugin's in-process check (Story 4.3):
                // MixDeck is stereo throughout (Deck/Mixer/MasterBus), and the
                // audioBlock wire format (Story 4.4.2) assumes a fixed 2-channel
                // shape agreed once here, not re-negotiated per block.
                juce::AudioProcessor::BusesLayout stereoInOut;
                stereoInOut.inputBuses.add(juce::AudioChannelSet::stereo());
                stereoInOut.outputBuses.add(juce::AudioChannelSet::stereo());
                if (!instance->checkBusesLayoutSupported(stereoInOut) || !instance->setBusesLayout(stereoInOut)) {
                    instance.reset();
                    sendMessageToCoordinator(mixdeck::makePluginLoadFailedMessage(
                        "Ce plugin ne supporte pas un bus stereo standard (entree + sortie)."));
                    return;
                }

                instance->prepareToPlay(sampleRate, blockSize);
                scratchAudio.setSize(2, blockSize);
                sendMessageToCoordinator(mixdeck::makeSimpleMessage(WorkerMessageType::pluginLoaded));
                break;
            }
            case WorkerMessageType::showEditor:
                juce::MessageManager::callAsync([this] { showEditorOnMessageThread(); });
                break;
            case WorkerMessageType::hideEditor:
                juce::MessageManager::callAsync([this] {
                    if (editor != nullptr)
                        editor->setVisible(false);
                });
                break;
            case WorkerMessageType::audioBlock:
                // Story 4.4.2 — processed synchronously right here, already off
                // the message thread (see class comment); no dedicated
                // processing thread needed on this side, unlike the real
                // real-time audio thread on the coordinator's side.
                if (instance != nullptr && mixdeck::readAudioBlockMessage(message, scratchAudio)) {
                    instance->processBlock(scratchAudio, scratchMidi);
                    scratchMidi.clear(); // a plugin may emit MIDI even without receiving any
                    sendMessageToCoordinator(mixdeck::makeAudioBlockMessage(scratchAudio));
                }
                break;
            case WorkerMessageType::pluginLoaded:
            case WorkerMessageType::pluginLoadFailed:
                // Worker -> coordinator only; never sent to a worker.
                break;
        }
    }

    void handleConnectionLost() override {
        // The coordinator (main process) is gone or removed this plugin —
        // nothing left for this process to do.
        juce::MessageManager::callAsync([] {
            if (auto* app = juce::JUCEApplication::getInstance())
                app->systemRequestedQuit();
        });
    }

private:
    void showEditorOnMessageThread() {
        if (instance == nullptr)
            return;
        if (editor == nullptr)
            editor.reset(instance->createEditorAndMakeActive());
        if (editor == nullptr)
            return;

        // Real decorated top-level window (title bar, close button) — the
        // same styleFlags PluginChain::showPluginEditor already uses for the
        // standalone-harness case (no foreign window to embed into there
        // either), just relocated to this process.
        editor->addToDesktop(juce::ComponentPeer::windowHasTitleBar | juce::ComponentPeer::windowHasCloseButton
                              | juce::ComponentPeer::windowAppearsOnTaskbar);
        editor->setVisible(true);
        editor->toFront(true);
        // This process is a background helper, not the active app — toFront()
        // alone (Peer::toFront -> [window makeKeyAndOrderFront:]) orders the
        // window front within this app's own layer, but macOS won't switch
        // foreground focus away from Electron just for that: the window can
        // end up created and visible yet stuck behind Electron's window with
        // no way to reach it. makeForegroundProcess() (-> [NSApp
        // activateIgnoringOtherApps:YES]) is the explicit step that actually
        // steals focus so the editor is reachable.
        juce::Process::makeForegroundProcess();
    }

    juce::AudioPluginFormatManager formatManager;
    std::unique_ptr<juce::AudioPluginInstance> instance;
    std::unique_ptr<juce::AudioProcessorEditor> editor;
    // Story 4.4.2 — reused for every audioBlock message (sized once the
    // plugin loads, see loadPlugin above) rather than reallocated per block.
    juce::AudioBuffer<float> scratchAudio;
    juce::MidiBuffer scratchMidi; // always cleared after each processBlock call, never fed
};

class MixDeckPluginWorkerApplication : public juce::JUCEApplication {
public:
    const juce::String getApplicationName() override { return "MixDeckPluginWorker"; }
    const juce::String getApplicationVersion() override { return "0.1.0"; }
    bool moreThanOneInstanceAllowed() override { return true; } // one process per isolated plugin instance

    void initialise(const juce::String& commandLine) override {
        worker = std::make_unique<PluginWorker>();
        if (!worker->initialiseFromCommandLine(commandLine, "mixdeckpluginworker")) {
            // Not launched by IsolatedPluginHost (e.g. opened directly by
            // double-clicking it) — this exe has no purpose on its own.
            quit();
        }
    }

    void shutdown() override { worker = nullptr; }

private:
    std::unique_ptr<PluginWorker> worker;
};

} // namespace

START_JUCE_APPLICATION(MixDeckPluginWorkerApplication)
