#include "DeckPanel.h"

namespace mixdeck {

namespace {
juce::String formatSeconds(double seconds) {
    if (seconds < 0.0 || std::isnan(seconds))
        seconds = 0.0;

    const auto totalSeconds = static_cast<int>(seconds);
    return juce::String::formatted("%02d:%02d", totalSeconds / 60, totalSeconds % 60);
}

juce::String deckStateToString(DeckState state) {
    switch (state) {
        case DeckState::Empty:   return "EMPTY";
        case DeckState::Loading: return "LOADING";
        case DeckState::Ready:   return "READY";
        case DeckState::Playing: return "PLAYING";
        case DeckState::Paused:  return "PAUSED";
        case DeckState::Stopped: return "STOPPED";
        case DeckState::Error:   return "ERROR";
    }
    return {};
}
} // namespace

DeckPanel::DeckPanel(juce::String labelText, juce::Colour accentColour, Deck& deckToControl)
    : label(std::move(labelText)), accent(accentColour), deck(deckToControl) {
    addAndMakeVisible(loadButton);
    addAndMakeVisible(playButton);
    addAndMakeVisible(pauseButton);
    addAndMakeVisible(stopButton);
    addAndMakeVisible(unloadButton);
    addAndMakeVisible(trackLabel);
    addAndMakeVisible(stateLabel);
    addAndMakeVisible(positionLabel);
    addAndMakeVisible(positionSlider);
    addAndMakeVisible(volumeSlider);

    stateLabel.setJustificationType(juce::Justification::centred);
    stateLabel.setFont(juce::FontOptions(11.0f));

    positionSlider.onValueChange = [this] { deck.seek(positionSlider.getValue()); };

    volumeSlider.setRange(0.0, 1.0);
    volumeSlider.setValue(1.0, juce::dontSendNotification);
    volumeSlider.onValueChange = [this] {
        if (onVolumeChanged)
            onVolumeChanged(static_cast<float>(volumeSlider.getValue()));
    };

    addAndMakeVisible(filterKnob);
    addAndMakeVisible(filterLabel);
    filterKnob.setRange(-1.0, 1.0);
    filterKnob.setValue(0.0, juce::dontSendNotification);
    filterKnob.onValueChange = [this] {
        if (onFilterChanged)
            onFilterChanged(static_cast<float>(filterKnob.getValue()));
    };
    filterLabel.setText("Filtre", juce::dontSendNotification);
    filterLabel.setJustificationType(juce::Justification::centred);
    filterLabel.setFont(juce::FontOptions(11.0f));

    addAndMakeVisible(pitchSlider);
    addAndMakeVisible(pitchLabel);
    pitchSlider.setRange(-50.0, 50.0);
    pitchSlider.setValue(0.0, juce::dontSendNotification);
    pitchSlider.onValueChange = [this] {
        if (onPitchChanged)
            onPitchChanged(static_cast<float>(pitchSlider.getValue()));
    };
    pitchLabel.setText("Pitch", juce::dontSendNotification);
    pitchLabel.setJustificationType(juce::Justification::centred);
    pitchLabel.setFont(juce::FontOptions(11.0f));

    trackLabel.setText("Aucune piste chargee", juce::dontSendNotification);
    trackLabel.setJustificationType(juce::Justification::centred);
    positionLabel.setText("00:00 / 00:00", juce::dontSendNotification);
    positionLabel.setJustificationType(juce::Justification::centred);

    loadButton.onClick = [this] { loadFileClicked(); };
    playButton.onClick = [this] { deck.play(); };
    pauseButton.onClick = [this] { deck.pause(); };
    stopButton.onClick = [this] { deck.stop(); };
    unloadButton.onClick = [this] {
        deck.unloadTrack();
        trackLabel.setText("Aucune piste chargee", juce::dontSendNotification);
    };

    updateTransportButtons();
    startTimerHz(15);
}

DeckPanel::~DeckPanel() {
    stopTimer();
}

void DeckPanel::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(0xff16181d));
    g.setColour(accent);
    g.fillRect(0, 0, getWidth(), 4);
    g.setColour(juce::Colours::white);
    g.setFont(juce::FontOptions(16.0f, juce::Font::bold));
    g.drawText(label, getLocalBounds().removeFromTop(30).withTrimmedTop(6), juce::Justification::centred);
}

void DeckPanel::resized() {
    auto area = getLocalBounds().reduced(10);
    area.removeFromTop(30); // deck label, drawn in paint()

    loadButton.setBounds(area.removeFromTop(28));
    area.removeFromTop(8);
    trackLabel.setBounds(area.removeFromTop(24));
    area.removeFromTop(4);
    stateLabel.setBounds(area.removeFromTop(16));
    area.removeFromTop(8);
    positionLabel.setBounds(area.removeFromTop(18));
    area.removeFromTop(4);
    positionSlider.setBounds(area.removeFromTop(22));
    area.removeFromTop(8);

    auto transportArea = area.removeFromTop(28);
    const auto buttonWidth = transportArea.getWidth() / 4;
    playButton.setBounds(transportArea.removeFromLeft(buttonWidth).reduced(3, 0));
    pauseButton.setBounds(transportArea.removeFromLeft(buttonWidth).reduced(3, 0));
    stopButton.setBounds(transportArea.removeFromLeft(buttonWidth).reduced(3, 0));
    unloadButton.setBounds(transportArea.reduced(3, 0));

    area.removeFromTop(8);
    pitchLabel.setBounds(area.removeFromTop(14));
    pitchSlider.setBounds(area.removeFromTop(24));
    area.removeFromTop(8);

    auto filterArea = area.removeFromRight(70);
    filterLabel.setBounds(filterArea.removeFromTop(16));
    filterKnob.setBounds(filterArea.removeFromTop(70));

    area.removeFromRight(8);
    volumeSlider.setBounds(area);
}

void DeckPanel::loadFileClicked() {
    fileChooser = std::make_unique<juce::FileChooser>(
        "Charger un fichier audio pour " + label,
        juce::File(),
        "*.wav;*.mp3;*.flac;*.aif;*.aiff");

    constexpr auto chooserFlags = juce::FileBrowserComponent::openMode
                                 | juce::FileBrowserComponent::canSelectFiles;

    fileChooser->launchAsync(chooserFlags, [this](const juce::FileChooser& chooser) {
        const auto file = chooser.getResult();
        if (file == juce::File())
            return;

        const auto error = deck.loadFile(file);
        if (error.isNotEmpty())
            trackLabel.setText(error, juce::dontSendNotification);
        else
            trackLabel.setText(deck.getLoadedFileName(), juce::dontSendNotification);
    });
}

void DeckPanel::updateTransportButtons() {
    const auto hasTrack = deck.getLoadedFileName().isNotEmpty();
    playButton.setEnabled(hasTrack);
    pauseButton.setEnabled(hasTrack);
    stopButton.setEnabled(hasTrack);
    unloadButton.setEnabled(hasTrack);
}

void DeckPanel::timerCallback() {
    updateTransportButtons();
    stateLabel.setText(deckStateToString(deck.getState()), juce::dontSendNotification);
    positionLabel.setText(formatSeconds(deck.getPositionSeconds()) + " / "
                               + formatSeconds(deck.getLengthSeconds()),
                           juce::dontSendNotification);

    positionSlider.setRange(0.0, juce::jmax(0.01, deck.getLengthSeconds()));
    positionSlider.setValue(deck.getPositionSeconds(), juce::dontSendNotification);
}

} // namespace mixdeck
