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
        return {};
    }

    return "Format de fichier non reconnu : " + file.getFileName();
}

void Deck::play() {
    if (readerSource != nullptr)
        transportSource.start();
}

void Deck::stop() {
    transportSource.stop();
    transportSource.setPosition(0.0);
}

bool Deck::isPlaying() const {
    return transportSource.isPlaying();
}

double Deck::getPositionSeconds() const {
    return transportSource.getCurrentPosition();
}

double Deck::getLengthSeconds() const {
    return transportSource.getLengthInSeconds();
}

void Deck::prepareToPlay(int samplesPerBlockExpected, double sampleRate) {
    transportSource.prepareToPlay(samplesPerBlockExpected, sampleRate);
}

void Deck::releaseResources() {
    transportSource.releaseResources();
}

void Deck::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) {
    if (readerSource == nullptr) {
        bufferToFill.clearActiveBufferRegion();
        return;
    }

    transportSource.getNextAudioBlock(bufferToFill);
}

} // namespace mixdeck
