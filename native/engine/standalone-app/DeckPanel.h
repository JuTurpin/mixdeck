#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../src/Deck.h"

namespace mixdeck {

/** Minimal transport + volume + filter + pitch controls for one Deck: load /
    play / pause / stop / unload, a DeckState label (2.1, ADR-016), a seekable
    position slider, a volume fader (1.2), a filter knob (1.3), a pitch slider
    (1.4), a pitch-mode toggle (3.1 — linked speed vs. independent
    time-stretch, ADR-006), a BPM label (3.2, updated once background
    analysis completes), a Sync button (3.3 — one-shot tempo match against
    the other deck, computed by MainComponent) and minimal plugin-chain
    validation buttons (4.3 — add the first known plugin, toggle its real
    editor). Test harness only — not the final Electron/React UI (§6). */
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

    // Called with -50..+50 (%) whenever the pitch slider moves (wired to Deck::setPitch by MainComponent).
    std::function<void(float)> onPitchChanged;

    // Story 3.3 — called when the Sync button is clicked (MainComponent computes
    // the target percent and calls setPitchDisplay() back to reflect it here).
    std::function<void()> onSyncRequested;

    // Updates the pitch slider's displayed value without re-triggering onPitchChanged
    // (Story 3.3 — reflects a sync applied programmatically, not by dragging).
    void setPitchDisplay(float percent);

    // Story 4.3 — minimal validation, not a real chain UI (see Electron for
    // that): adds the first plugin PluginHost knows about, and opens/hides
    // the first slot's real editor as its own top-level window (no Electron
    // window to embed into here).
    std::function<void()> onAddFirstPluginRequested;
    std::function<void()> onTogglePluginEditorRequested;

private:
    void loadFileClicked();
    void updateTransportButtons();
    void timerCallback() override;

    juce::String label;
    juce::Colour accent;
    Deck& deck;

    juce::TextButton loadButton { "Charger un fichier..." };
    juce::TextButton playButton { "Play" };
    juce::TextButton pauseButton { "Pause" };
    juce::TextButton stopButton { "Stop" };
    juce::TextButton unloadButton { "Unload" };
    juce::Label trackLabel;
    juce::Label stateLabel;
    juce::Label bpmLabel; // Story 3.2 — "-- BPM" until analysis completes
    juce::Label positionLabel;
    juce::Slider positionSlider { juce::Slider::LinearHorizontal, juce::Slider::NoTextBox };
    juce::Slider volumeSlider { juce::Slider::LinearVertical, juce::Slider::NoTextBox };
    juce::Slider filterKnob { juce::Slider::RotaryVerticalDrag, juce::Slider::NoTextBox };
    juce::Label filterLabel;
    juce::Slider pitchSlider { juce::Slider::LinearHorizontal, juce::Slider::NoTextBox };
    juce::Label pitchLabel;
    juce::TextButton pitchModeButton { "Vitesse liee" };
    juce::TextButton syncButton { "Sync" };
    juce::TextButton addPluginButton { "+ Plugin" };
    juce::TextButton pluginEditorButton { "Editeur plugin" };

    std::unique_ptr<juce::FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DeckPanel)
};

} // namespace mixdeck
