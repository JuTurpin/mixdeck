#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "../src/Deck.h"
#include "../src/Mixer.h"
#include "DeckPanel.h"

namespace mixdeck {

/** Test harness: two independent decks summed to the default output device,
    with per-deck volume and a crossfader (Story 1.2, gain math in Mixer). */
class MainComponent : public juce::AudioAppComponent {
public:
    MainComponent();
    ~MainComponent() override;

    // juce::AudioAppComponent
    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void releaseResources() override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;

    // juce::Component
    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void crossfaderCurveChanged();

    juce::AudioFormatManager formatManager;
    Deck deckA { formatManager };
    Deck deckB { formatManager };
    juce::MixerAudioSource audioMixer; // sums deckA/deckB to the output; gain itself lives on each Deck (see Mixer)
    Mixer mixer { deckA, deckB };

    DeckPanel deckPanelA { "DECK A", juce::Colour(0xff4fd1c5), deckA };
    DeckPanel deckPanelB { "DECK B", juce::Colour(0xfff0955a), deckB };

    juce::Slider crossfaderSlider { juce::Slider::LinearHorizontal, juce::Slider::NoTextBox };
    juce::ComboBox crossfaderCurveBox;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};

} // namespace mixdeck
