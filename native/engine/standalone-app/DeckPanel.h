#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../src/Deck.h"

namespace mixdeck {

/** Minimal transport controls for one Deck: load / play / stop + status labels.
    Test harness only for Story 1.1 — not the final Electron/React UI (see
    architecture.md §6 for the target design). */
class DeckPanel : public juce::Component, private juce::Timer {
public:
    DeckPanel(juce::String labelText, juce::Colour accentColour, Deck& deckToControl);
    ~DeckPanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void loadFileClicked();
    void updateTransportButtons();
    void timerCallback() override;

    juce::String label;
    juce::Colour accent;
    Deck& deck;

    juce::TextButton loadButton { "Charger un fichier..." };
    juce::TextButton playButton { "Play" };
    juce::TextButton stopButton { "Stop" };
    juce::Label trackLabel;
    juce::Label positionLabel;

    std::unique_ptr<juce::FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DeckPanel)
};

} // namespace mixdeck
