#include "MainComponent.h"

namespace mixdeck {

MainComponent::MainComponent() {
    formatManager.registerBasicFormats();

    mixer.addInputSource(&deckA, false);
    mixer.addInputSource(&deckB, false);

    addAndMakeVisible(deckPanelA);
    addAndMakeVisible(deckPanelB);

    setSize(640, 360);
    setAudioChannels(0, 2); // no input, stereo output
}

MainComponent::~MainComponent() {
    shutdownAudio();
    mixer.removeAllInputs();
}

void MainComponent::prepareToPlay(int samplesPerBlockExpected, double sampleRate) {
    mixer.prepareToPlay(samplesPerBlockExpected, sampleRate);
}

void MainComponent::releaseResources() {
    mixer.releaseResources();
}

void MainComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) {
    mixer.getNextAudioBlock(bufferToFill);
}

void MainComponent::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(0xff0d0e11));
}

void MainComponent::resized() {
    auto area = getLocalBounds().reduced(12);
    const auto panelWidth = (area.getWidth() - 12) / 2;

    deckPanelA.setBounds(area.removeFromLeft(panelWidth));
    area.removeFromLeft(12);
    deckPanelB.setBounds(area);
}

} // namespace mixdeck
