#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "../src/Deck.h"
#include "DeckPanel.h"

namespace mixdeck {

/** Story 1.1 test harness: two independent decks summed to the default output
    device. No gain/crossfader yet (Story 1.2 replaces this raw summing mixer). */
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
    juce::AudioFormatManager formatManager;
    Deck deckA { formatManager };
    Deck deckB { formatManager };
    juce::MixerAudioSource mixer;

    DeckPanel deckPanelA { "DECK A", juce::Colour(0xff4fd1c5), deckA };
    DeckPanel deckPanelB { "DECK B", juce::Colour(0xfff0955a), deckB };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};

} // namespace mixdeck
