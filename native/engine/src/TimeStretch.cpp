#include "TimeStretch.h"
#include <algorithm>

namespace mixdeck {

namespace {

// SoundTouch processes input internally in "sequence"-sized batches (tens of
// milliseconds' worth of samples), not proportionally to each putSamples()
// call — a single JUCE block's worth of input doesn't reliably yield a
// proportional slice of new output straight away (see SoundTouch.h's own
// notes on SETTING_NOMINAL_INPUT/OUTPUT_SEQUENCE). Feeding only just enough
// for the current call starves it every time the backlog drains, causing
// audible dropouts on a repeating cycle. Keeping a much larger standing
// backlog (independent of the JUCE block size) absorbs that batch
// granularity so steady-state playback never catches SoundTouch empty-handed.
constexpr int targetBacklogFrames = 8192; // ~185ms at 44.1kHz

// Bounds the feed loop below: enough iterations to reach targetBacklogFrames
// from empty (e.g. right after reset()/a mode switch) even with a small JUCE
// block size, without looping unbounded.
constexpr int maxFeedIterations = 32;

void interleave(const juce::AudioBuffer<float>& source, float* destination, int numFrames) {
    const int channels = source.getNumChannels();
    for (int frame = 0; frame < numFrames; ++frame)
        for (int channel = 0; channel < channels; ++channel)
            destination[frame * channels + channel] = source.getReadPointer(channel)[frame];
}

void deinterleave(const float* source, juce::AudioBuffer<float>& destination,
                   int startSample, int numFrames, int channels) {
    for (int frame = 0; frame < numFrames; ++frame)
        for (int channel = 0; channel < channels; ++channel)
            destination.getWritePointer(channel)[startSample + frame] = source[frame * channels + channel];
}

} // namespace

void TimeStretch::prepare(double sampleRate, int maxBlockSize) {
    feedChunkFrames = maxBlockSize;
    inputScratch.setSize(numChannels, maxBlockSize);
    interleavedInput.assign((size_t) maxBlockSize * numChannels, 0.0f);
    interleavedOutput.assign((size_t) maxBlockSize * numChannels, 0.0f);

    soundTouch.setSampleRate((unsigned int) sampleRate);
    soundTouch.setChannels((unsigned int) numChannels);
    soundTouch.setPitchSemiTones(0); // documents intent: pitch never moves in this mode
    soundTouch.setTempo(1.0);
}

void TimeStretch::releaseResources() {
    soundTouch.clear();
}

void TimeStretch::setTempoPercent(float percent) {
    soundTouch.setTempoChange(juce::jlimit(-50.0f, 50.0f, percent));
}

void TimeStretch::reset() {
    soundTouch.clear();
}

void TimeStretch::getNextAudioBlock(juce::AudioSource& input,
                                     const juce::AudioSourceChannelInfo& bufferToFill) {
    const int numOut = bufferToFill.numSamples;

    // Top up the backlog first — independently of numOut — so SoundTouch's
    // internal batch processing always has a comfortable head start. Most
    // calls in steady state find the backlog already full and skip this
    // entirely (0 iterations); it only runs after reset() or a rare deep dip.
    for (int iteration = 0;
         (int) soundTouch.numSamples() < targetBacklogFrames && iteration < maxFeedIterations;
         ++iteration) {
        juce::AudioSourceChannelInfo inputInfo(&inputScratch, 0, feedChunkFrames);
        input.getNextAudioBlock(inputInfo);
        interleave(inputScratch, interleavedInput.data(), feedChunkFrames);
        soundTouch.putSamples(interleavedInput.data(), (unsigned int) feedChunkFrames);
    }

    const auto got = soundTouch.receiveSamples(interleavedOutput.data(), (unsigned int) numOut);
    if ((int) got < numOut)
        std::fill(interleavedOutput.begin() + (size_t) got * numChannels,
                  interleavedOutput.begin() + (size_t) numOut * numChannels,
                  0.0f);

    deinterleave(interleavedOutput.data(), *bufferToFill.buffer, bufferToFill.startSample, numOut, numChannels);
}

} // namespace mixdeck
