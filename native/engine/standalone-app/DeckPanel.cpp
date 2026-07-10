#include "DeckPanel.h"

namespace mixdeck {

namespace {
juce::String formatSeconds(double seconds) {
    if (seconds < 0.0 || std::isnan(seconds))
        seconds = 0.0;

    const auto totalSeconds = static_cast<int>(seconds);
    return juce::String::formatted("%02d:%02d", totalSeconds / 60, totalSeconds % 60);
}
} // namespace

DeckPanel::DeckPanel(juce::String labelText, juce::Colour accentColour, Deck& deckToControl)
    : label(std::move(labelText)), accent(accentColour), deck(deckToControl) {
    addAndMakeVisible(loadButton);
    addAndMakeVisible(playButton);
    addAndMakeVisible(stopButton);
    addAndMakeVisible(trackLabel);
    addAndMakeVisible(positionLabel);
    addAndMakeVisible(volumeSlider);

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

    trackLabel.setText("Aucune piste chargee", juce::dontSendNotification);
    trackLabel.setJustificationType(juce::Justification::centred);
    positionLabel.setText("00:00 / 00:00", juce::dontSendNotification);
    positionLabel.setJustificationType(juce::Justification::centred);

    loadButton.onClick = [this] { loadFileClicked(); };
    playButton.onClick = [this] { deck.play(); };
    stopButton.onClick = [this] { deck.stop(); };

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
    area.removeFromTop(8);
    positionLabel.setBounds(area.removeFromTop(20));
    area.removeFromTop(8);

    auto transportArea = area.removeFromTop(28);
    playButton.setBounds(transportArea.removeFromLeft(transportArea.getWidth() / 2).reduced(4, 0));
    stopButton.setBounds(transportArea.reduced(4, 0));

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
    stopButton.setEnabled(hasTrack);
}

void DeckPanel::timerCallback() {
    updateTransportButtons();
    positionLabel.setText(formatSeconds(deck.getPositionSeconds()) + " / "
                               + formatSeconds(deck.getLengthSeconds()),
                           juce::dontSendNotification);
}

} // namespace mixdeck
