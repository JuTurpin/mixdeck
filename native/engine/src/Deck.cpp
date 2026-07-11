#include "Deck.h"

namespace mixdeck {

namespace {
constexpr int readAheadBufferSize = 32768;
}

Deck::Deck(juce::AudioFormatManager& formatManagerToUse)
    : formatManager(formatManagerToUse) {
    readAheadThread.startThread(juce::Thread::Priority::normal);
}

Deck::~Deck() {
    transportSource.setSource(nullptr);
    readAheadThread.stopThread(2000);
}

juce::String Deck::loadFile(const juce::File& file) {
    state = DeckState::Loading;

    if (auto* reader = formatManager.createReaderFor(file)) {
        auto newSource = std::make_unique<juce::AudioFormatReaderSource>(reader, true);
        transportSource.stop();
        transportSource.setSource(newSource.get(),
                                   readAheadBufferSize,
                                   &readAheadThread,
                                   reader->sampleRate,
                                   reader->numChannels);
        readerSource = std::move(newSource);
        loadedFileName = file.getFileName();
        state = DeckState::Ready;
        return {};
    }

    state = DeckState::Error;
    return "Format de fichier non reconnu : " + file.getFileName();
}

void Deck::unloadTrack() {
    transportSource.stop();
    transportSource.setSource(nullptr);
    readerSource.reset();
    loadedFileName = {};
    state = DeckState::Empty;
}

void Deck::play() {
    if (readerSource != nullptr) {
        transportSource.start();
        state = DeckState::Playing;
    }
}

void Deck::pause() {
    transportSource.stop();
    state = DeckState::Paused;
}

void Deck::stop() {
    transportSource.stop();
    transportSource.setPosition(0.0);
    state = DeckState::Stopped;
}

void Deck::seek(double seconds) {
    transportSource.setPosition(juce::jlimit(0.0, transportSource.getLengthInSeconds(), seconds));
}

bool Deck::isPlaying() const {
    return transportSource.isPlaying();
}

DeckState Deck::getState() const {
    if (state == DeckState::Playing && !transportSource.isPlaying())
        state = DeckState::Stopped; // natural end of track reached

    return state;
}

void Deck::setGain(float gain) {
    transportSource.setGain(gain);
}

void Deck::setFilterKnob(float value) {
    filter.setKnobPosition(value);
}

void Deck::setPitch(float percent) {
    const auto ratio = juce::jlimit(0.5, 1.5, 1.0 + (double) percent / 100.0);
    pitchResampler.setResamplingRatio(ratio);
}

double Deck::getPositionSeconds() const {
    return transportSource.getCurrentPosition();
}

double Deck::getLengthSeconds() const {
    return transportSource.getLengthInSeconds();
}

void Deck::prepareToPlay(int samplesPerBlockExpected, double sampleRate) {
    transportSource.prepareToPlay(samplesPerBlockExpected, sampleRate);
    pitchResampler.prepareToPlay(samplesPerBlockExpected, sampleRate);
    filter.prepare({ sampleRate, (juce::uint32) samplesPerBlockExpected, 2 });
}

void Deck::releaseResources() {
    transportSource.releaseResources();
    pitchResampler.releaseResources();
}

void Deck::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) {
    if (readerSource == nullptr) {
        bufferToFill.clearActiveBufferRegion();
        return;
    }

    pitchResampler.getNextAudioBlock(bufferToFill);

    auto block = juce::dsp::AudioBlock<float>(*bufferToFill.buffer)
                     .getSubBlock((size_t) bufferToFill.startSample, (size_t) bufferToFill.numSamples);
    filter.process(block);
}

} // namespace mixdeck
