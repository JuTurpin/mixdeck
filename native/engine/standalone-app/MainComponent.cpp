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
    deckPanelA.onPitchChanged = [this](float percent) {
        pitchA = percent;
        deckA.setPitch(percent);
    };
    deckPanelB.onPitchChanged = [this](float percent) {
        pitchB = percent;
        deckB.setPitch(percent);
    };
    // Story 3.3 — ponctuel : calcule le pourcentage cible à partir du BPM
    // effectif actuel de l'autre deck, l'applique, et reflète le résultat
    // sur le slider (setPitchDisplay ne redéclenche pas onPitchChanged).
    deckPanelA.onSyncRequested = [this] {
        const auto ownBpm = deckA.getBpm();
        const auto otherBpm = deckB.getBpm();
        if (ownBpm <= 0.0f || otherBpm <= 0.0f)
            return;
        const auto otherEffectiveBpm = otherBpm * (1.0f + pitchB / 100.0f);
        pitchA = computeSyncPitch(ownBpm, otherEffectiveBpm);
        deckA.setPitch(pitchA);
        deckPanelA.setPitchDisplay(pitchA);
    };
    deckPanelB.onSyncRequested = [this] {
        const auto ownBpm = deckB.getBpm();
        const auto otherBpm = deckA.getBpm();
        if (ownBpm <= 0.0f || otherBpm <= 0.0f)
            return;
        const auto otherEffectiveBpm = otherBpm * (1.0f + pitchA / 100.0f);
        pitchB = computeSyncPitch(ownBpm, otherEffectiveBpm);
        deckB.setPitch(pitchB);
        deckPanelB.setPitchDisplay(pitchB);
    };
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

float MainComponent::computeSyncPitch(float ownBpm, float otherEffectiveBpm) {
    const auto raw = (otherEffectiveBpm / ownBpm - 1.0f) * 100.0f;
    return juce::jlimit(-50.0f, 50.0f, raw);
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
