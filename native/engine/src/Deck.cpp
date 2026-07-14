#include "Deck.h"
#include "BpmAnalyzer.h"

namespace mixdeck {

namespace {
constexpr int readAheadBufferSize = 32768;
}

juce::String toString(DeckState state) {
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

juce::String toString(PitchMode mode) {
    switch (mode) {
        case PitchMode::LinkedSpeed:  return "linked";
        case PitchMode::Independent:  return "independent";
    }
    return {};
}

Deck::Deck(juce::AudioFormatManager& formatManagerToUse, juce::ThreadPool& analysisPoolToUse)
    : formatManager(formatManagerToUse), analysisPool(analysisPoolToUse) {
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
        startBpmAnalysis(file);
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
    timeStretch.reset();
    ++loadGeneration; // invalidate any BPM analysis job still in flight
    {
        std::lock_guard<std::mutex> lock(analysisMutex);
        bpm = 0.0f;
        beatGrid.clear();
    }
    state = DeckState::Empty;
}

void Deck::startBpmAnalysis(const juce::File& file) {
    const auto generation = ++loadGeneration;
    {
        std::lock_guard<std::mutex> lock(analysisMutex);
        bpm = 0.0f;
        beatGrid.clear();
    }

    analysisPool.addJob([this, file, generation] {
        auto result = analyzeBpm(file, formatManager);

        std::lock_guard<std::mutex> lock(analysisMutex);
        if (generation == loadGeneration.load()) {
            bpm = result.bpm;
            beatGrid = std::move(result.beatGridSeconds);
        }
    });
}

float Deck::getBpm() const {
    std::lock_guard<std::mutex> lock(analysisMutex);
    return bpm;
}

std::vector<float> Deck::getBeatGrid() const {
    std::lock_guard<std::mutex> lock(analysisMutex);
    return beatGrid;
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
    timeStretch.reset(); // discontinuity: don't let buffered stretched audio bleed in
    state = DeckState::Stopped;
}

void Deck::seek(double seconds) {
    transportSource.setPosition(juce::jlimit(0.0, transportSource.getLengthInSeconds(), seconds));
    timeStretch.reset(); // discontinuity: don't let buffered stretched audio bleed in
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
    // Both paths are kept in sync even when only one is active, so switching
    // modes needs no extra resynchronization.
    const auto ratio = juce::jlimit(0.5, 1.5, 1.0 + (double) percent / 100.0);
    pitchResampler.setResamplingRatio(ratio);
    timeStretch.setTempoPercent(percent);
}

void Deck::setPitchMode(PitchMode mode) {
    if (mode == pitchMode)
        return;

    pitchMode = mode;
    timeStretch.reset(); // flush stale FIFO before the new path (re)activates
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
    timeStretch.prepare(sampleRate, samplesPerBlockExpected);
    filter.prepare({ sampleRate, (juce::uint32) samplesPerBlockExpected, 2 });
}

void Deck::releaseResources() {
    transportSource.releaseResources();
    pitchResampler.releaseResources();
    timeStretch.releaseResources();
}

void Deck::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) {
    if (readerSource == nullptr) {
        bufferToFill.clearActiveBufferRegion();
        return;
    }

    if (pitchMode == PitchMode::Independent)
        timeStretch.getNextAudioBlock(transportSource, bufferToFill);
    else
        pitchResampler.getNextAudioBlock(bufferToFill);

    auto block = juce::dsp::AudioBlock<float>(*bufferToFill.buffer)
                     .getSubBlock((size_t) bufferToFill.startSample, (size_t) bufferToFill.numSamples);
    filter.process(block);
}

} // namespace mixdeck
