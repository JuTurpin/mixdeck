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

    // Story 3.3 — pourcentage à appliquer pour que ownBpm rejoigne
    // otherEffectiveBpm (BPM effectif actuel de l'autre deck), clampé à la
    // plage du slider de pitch existant. Même formule que côté TS
    // (DeckController::computeSyncPitch).
    static float computeSyncPitch(float ownBpm, float otherEffectiveBpm);

    juce::AudioFormatManager formatManager;
    // Story 3.2 — shared BPM-analysis pool; declared before deckA/deckB so
    // it's constructed first (member init order follows declaration order).
    juce::ThreadPool analysisPool { 2 };
    Deck deckA { formatManager, analysisPool };
    Deck deckB { formatManager, analysisPool };
    juce::MixerAudioSource audioMixer; // sums deckA/deckB to the output; gain itself lives on each Deck (see Mixer)
    Mixer mixer { deckA, deckB };

    DeckPanel deckPanelA { "DECK A", juce::Colour(0xff4fd1c5), deckA };
    DeckPanel deckPanelB { "DECK B", juce::Colour(0xfff0955a), deckB };

    // Story 3.3 — mémorise le dernier pitch appliqué à chaque deck (setPitch
    // est write-only côté Deck ; l'UI est la seule à connaître sa propre
    // valeur courante), mis à jour dans onPitchChanged et par le Sync lui-même.
    float pitchA = 0.0f;
    float pitchB = 0.0f;

    juce::Slider crossfaderSlider { juce::Slider::LinearHorizontal, juce::Slider::NoTextBox };
    juce::ComboBox crossfaderCurveBox;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};

} // namespace mixdeck
