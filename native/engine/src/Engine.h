#pragma once

#include "Deck.h"
#include "Mixer.h"

namespace mixdeck {

/** Headless equivalent of the standalone-app's MainComponent audio graph
    (deckA/deckB -> MixerAudioSource -> output), without any JUCE GUI
    Component — used by the Node Bridge (Story 2.2), which has no window of
    its own. MainComponent is untouched and keeps driving the JUCE test
    harness. */
class Engine {
public:
    Engine();
    ~Engine();

    Deck& getDeck(int index); // 0 = A, 1 = B
    Mixer& getMixer() { return mixer; }

private:
    juce::AudioDeviceManager deviceManager;
    juce::AudioFormatManager formatManager;
    Deck deckA { formatManager };
    Deck deckB { formatManager };
    juce::MixerAudioSource audioMixer;
    Mixer mixer { deckA, deckB };
    juce::AudioSourcePlayer audioSourcePlayer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Engine)
};

} // namespace mixdeck
