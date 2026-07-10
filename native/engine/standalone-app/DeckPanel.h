#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../src/Deck.h"

namespace mixdeck {

/** Minimal transport + volume + filter controls for one Deck: load / play / stop,
    status labels, a volume fader (Story 1.2) and a filter knob (Story 1.3). Test
    harness only — not the final Electron/React UI (see architecture.md §6). */
class DeckPanel : public juce::Component, private juce::Timer {
public:
    DeckPanel(juce::String labelText, juce::Colour accentColour, Deck& deckToControl);
    ~DeckPanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // Called with 0..1 whenever the volume fader moves (wired to Mixer::setDeckVolume by MainComponent).
    std::function<void(float)> onVolumeChanged;

    // Called with -1..+1 whenever the filter knob moves (wired to Deck::setFilterKnob by MainComponent).
    std::function<void(float)> onFilterChanged;

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
    juce::Slider volumeSlider { juce::Slider::LinearVertical, juce::Slider::NoTextBox };
    juce::Slider filterKnob { juce::Slider::RotaryVerticalDrag, juce::Slider::NoTextBox };
    juce::Label filterLabel;

    std::unique_ptr<juce::FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DeckPanel)
};

} // namespace mixdeck
