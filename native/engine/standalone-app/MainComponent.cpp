#include "MainComponent.h"

namespace mixdeck {

MainComponent::MainComponent() {
    formatManager.registerBasicFormats();

    audioMixer.addInputSource(&deckA, false);
    audioMixer.addInputSource(&deckB, false);

    deckPanelA.onVolumeChanged = [this](float volume) { mixer.setDeckVolume(0, volume); };
    deckPanelB.onVolumeChanged = [this](float volume) { mixer.setDeckVolume(1, volume); };
    deckPanelA.onFilterChanged = [this](float value) { deckA.setFilterKnob(value); };
    deckPanelB.onFilterChanged = [this](float value) { deckB.setFilterKnob(value); };
    deckPanelA.onPitchChanged = [this](float percent) { deckA.setPitch(percent); };
    deckPanelB.onPitchChanged = [this](float percent) { deckB.setPitch(percent); };
    addAndMakeVisible(deckPanelA);
    addAndMakeVisible(deckPanelB);

    crossfaderSlider.setRange(0.0, 1.0);
    crossfaderSlider.setValue(0.5, juce::dontSendNotification);
    crossfaderSlider.onValueChange = [this] {
        mixer.setCrossfaderPosition(static_cast<float>(crossfaderSlider.getValue()));
    };
    addAndMakeVisible(crossfaderSlider);

    crossfaderCurveBox.addItem("Lineaire", 1);
    crossfaderCurveBox.addItem("Smooth", 2);
    crossfaderCurveBox.addItem("Cut", 3);
    crossfaderCurveBox.setSelectedId(2, juce::dontSendNotification); // matches Mixer's default curve
    crossfaderCurveBox.onChange = [this] { crossfaderCurveChanged(); };
    addAndMakeVisible(crossfaderCurveBox);

    setSize(700, 490);
    setAudioChannels(0, 2); // no input, stereo output
}

MainComponent::~MainComponent() {
    shutdownAudio();
    audioMixer.removeAllInputs();
}

void MainComponent::prepareToPlay(int samplesPerBlockExpected, double sampleRate) {
    audioMixer.prepareToPlay(samplesPerBlockExpected, sampleRate);
}

void MainComponent::releaseResources() {
    audioMixer.releaseResources();
}

void MainComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) {
    audioMixer.getNextAudioBlock(bufferToFill);
}

void MainComponent::crossfaderCurveChanged() {
    switch (crossfaderCurveBox.getSelectedId()) {
        case 1: mixer.setCrossfaderCurve(CrossfaderCurve::Linear); break;
        case 3: mixer.setCrossfaderCurve(CrossfaderCurve::Cut); break;
        default: mixer.setCrossfaderCurve(CrossfaderCurve::Smooth); break;
    }
}

void MainComponent::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(0xff0d0e11));
}

void MainComponent::resized() {
    auto area = getLocalBounds().reduced(12);

    constexpr int middleWidth = 160;
    const auto sideWidth = (area.getWidth() - middleWidth - 24) / 2;

    deckPanelA.setBounds(area.removeFromLeft(sideWidth));
    area.removeFromLeft(12);
    auto middleArea = area.removeFromLeft(middleWidth);
    area.removeFromLeft(12);
    deckPanelB.setBounds(area);

    middleArea.removeFromTop(20);
    crossfaderCurveBox.setBounds(middleArea.removeFromTop(24));
    middleArea.removeFromTop(16);
    crossfaderSlider.setBounds(middleArea.removeFromTop(40));
}

} // namespace mixdeck
