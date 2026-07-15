#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "../src/Deck.h"
#include "../src/MasterBus.h"
#include "../src/Mixer.h"
#include "../src/PluginHost.h"
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
    void updatePluginResultsDisplay(); // Story 4.1 — called back on the message thread once scanForPlugins() completes
    void pluginFileChosen(const juce::File& file); // Story 4.2
    juce::String getFirstAvailablePluginIdentifier() const; // Story 4.3 — "" if none scanned/added yet

    // Story 3.3 — pourcentage à appliquer pour que ownBpm rejoigne
    // otherEffectiveBpm (BPM effectif actuel de l'autre deck), clampé à la
    // plage du slider de pitch existant. Même formule que côté TS
    // (DeckController::computeSyncPitch).
    static float computeSyncPitch(float ownBpm, float otherEffectiveBpm);

    juce::AudioFormatManager formatManager;
    // Story 3.2 — shared BPM-analysis pool (and plugin instantiation, Story
    // 4.3); declared before deckA/deckB so it's constructed first (member
    // init order follows declaration order).
    juce::ThreadPool analysisPool { 2 };
    // Story 4.1 — declared before deckA/deckB: they need it at construction.
    PluginHost pluginHost;
    Deck deckA { formatManager, analysisPool, pluginHost };
    Deck deckB { formatManager, analysisPool, pluginHost };
    juce::MixerAudioSource audioMixer; // sums deckA/deckB to the output; gain itself lives on each Deck (see Mixer)
    Mixer mixer { deckA, deckB };
    // Story 4.3 — wraps audioMixer with its own effects chain; getNextAudioBlock
    // pulls from this instead of audioMixer directly (see MainComponent.cpp).
    MasterBus masterBus { audioMixer, pluginHost, analysisPool };

    DeckPanel deckPanelA { "DECK A", juce::Colour(0xff4fd1c5), deckA };
    DeckPanel deckPanelB { "DECK B", juce::Colour(0xfff0955a), deckB };

    // Story 3.3 — mémorise le dernier pitch appliqué à chaque deck (setPitch
    // est write-only côté Deck ; l'UI est la seule à connaître sa propre
    // valeur courante), mis à jour dans onPitchChanged et par le Sync lui-même.
    float pitchA = 0.0f;
    float pitchB = 0.0f;

    juce::Slider crossfaderSlider { juce::Slider::LinearHorizontal, juce::Slider::NoTextBox };
    juce::ComboBox crossfaderCurveBox;

    // Story 4.1 — découverte de plugins ; pas spécifique à un deck, câblé
    // directement ici plutôt que dans DeckPanel (pluginHost lui-même est
    // déclaré plus haut, avant deckA/deckB qui en ont besoin).
    juce::TextButton pluginScanButton { "Scanner les plugins" };
    juce::TextButton pluginBrowseButton { "Parcourir un plugin..." }; // Story 4.2 — VST3 uniquement
    juce::TextEditor pluginResultsEditor;
    std::unique_ptr<juce::FileChooser> pluginFileChooser;

    // Story 4.3 — validation minimale du bus master (chaîne par deck testée
    // via les boutons de DeckPanel). juce::AudioPluginInstance n'a pas
    // d'identifiant "premier trouvé" tout fait : on relit
    // pluginHost.getAvailablePlugins()[0] au clic.
    juce::TextButton masterAddPluginButton { "+ Plugin (Master)" };
    juce::TextButton masterPluginEditorButton { "Editeur plugin (Master)" };
    bool deckAEditorVisible = false;
    bool deckBEditorVisible = false;
    bool masterEditorVisible = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};

} // namespace mixdeck
