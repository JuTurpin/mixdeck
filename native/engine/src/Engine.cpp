#include "Engine.h"

namespace mixdeck {

Engine::Engine() {
    formatManager.registerBasicFormats();

    audioMixer.addInputSource(&deckA, false);
    audioMixer.addInputSource(&deckB, false);

    audioSourcePlayer.setSource(&audioMixer);
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

} // namespace mixdeck
