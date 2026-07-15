#include "Engine.h"

namespace mixdeck {

Engine::Engine() {
    formatManager.registerBasicFormats();

    audioMixer.addInputSource(&deckA, false);
    audioMixer.addInputSource(&deckB, false);

    audioSourcePlayer.setSource(&masterBus); // Story 4.3 — routes through the master effects chain
    deviceManager.initialiseWithDefaultDevices(0, 2);
    deviceManager.addAudioCallback(&audioSourcePlayer);
}

Engine::~Engine() {
    deviceManager.removeAudioCallback(&audioSourcePlayer);
    audioSourcePlayer.setSource(nullptr);
    audioMixer.removeAllInputs();
}

Deck& Engine::getDeck(int index) {
    return index == 0 ? deckA : deckB;
}

void Engine::startPluginScan() {
    if (pluginScanInProgress.exchange(true))
        return; // a scan is already running

    analysisPool.addJob([this] {
        pluginHost.scanForPlugins();
        pluginScanInProgress = false;
    });
}

} // namespace mixdeck
