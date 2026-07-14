#include "BpmAnalyzer.h"
#include <BPMDetect.h>

namespace mixdeck {

namespace {

constexpr int blockSizeFrames = 65536; // per BPMDetect.h's own advice: "smallish chunks of a few kilosamples"

// Autocorrelation-based detection (BPMDetect) can lock onto a harmonic of the
// real tempo instead of the tempo itself — half or double are the common
// cases, since a repeating pattern at X BPM also repeats at X/2 and 2X. Fold
// the result into the common dance-music range by octave (doubling/halving,
// never anything finer) rather than trying to guess which harmonic is
// "right" — a heuristic Julien asked for after seeing a real half-tempo
// misdetection; it can itself misfire on a genuinely slow track.
constexpr float typicalBpmRangeMin = 90.0f;
constexpr float typicalBpmRangeMax = 180.0f;

float foldIntoTypicalRange(float bpm) {
    while (bpm > 0.0f && bpm < typicalBpmRangeMin && bpm * 2.0f <= typicalBpmRangeMax)
        bpm *= 2.0f;
    while (bpm > typicalBpmRangeMax && bpm / 2.0f >= typicalBpmRangeMin)
        bpm /= 2.0f;
    return bpm;
}

void interleave(const juce::AudioBuffer<float>& source, float* destination, int numFrames, int numChannels) {
    for (int frame = 0; frame < numFrames; ++frame)
        for (int channel = 0; channel < numChannels; ++channel)
            destination[frame * numChannels + channel] = source.getReadPointer(channel)[frame];
}

} // namespace

BpmAnalysis analyzeBpm(const juce::File& file, juce::AudioFormatManager& formatManager) {
    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
    if (reader == nullptr || reader->numChannels == 0)
        return {};

    const int numChannels = (int) reader->numChannels;
    soundtouch::BPMDetect bpmDetect(numChannels, (int) reader->sampleRate);

    juce::AudioBuffer<float> scratch(numChannels, blockSizeFrames);
    std::vector<float> interleaved((size_t) blockSizeFrames * numChannels);

    juce::int64 position = 0;
    while (position < reader->lengthInSamples) {
        const auto framesToRead =
            (int) juce::jmin<juce::int64>(blockSizeFrames, reader->lengthInSamples - position);
        reader->read(&scratch, 0, framesToRead, position, true, true);
        interleave(scratch, interleaved.data(), framesToRead, numChannels);
        bpmDetect.inputSamples(interleaved.data(), framesToRead);
        position += framesToRead;
    }

    BpmAnalysis result;
    result.bpm = foldIntoTypicalRange(bpmDetect.getBpm());

    const auto numBeats = bpmDetect.getBeats(nullptr, nullptr, 0);
    if (numBeats > 0) {
        std::vector<float> positions((size_t) numBeats);
        std::vector<float> strengths((size_t) numBeats);
        bpmDetect.getBeats(positions.data(), strengths.data(), numBeats);
        result.beatGridSeconds = std::move(positions);
    }

    return result;
}

} // namespace mixdeck
