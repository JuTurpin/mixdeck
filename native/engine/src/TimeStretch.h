#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <SoundTouch.h>
#include <vector>

namespace mixdeck {

/** Independent-tempo time-stretch (Story 3.1, ADR-006): wraps SoundTouch so the
    pitch slider changes tempo without shifting pitch (SoundTouch's own pitch
    stays pinned at 0 semitones, set once in prepare()). Adapts SoundTouch's
    putSamples/receiveSamples FIFO (variable yield per call) to JUCE's
    fixed-size getNextAudioBlock pull model, without allocating on the audio
    thread — every scratch buffer is sized once in prepare(). Stereo only,
    like the rest of Deck's chain. */
class TimeStretch {
public:
    TimeStretch() = default; // needed: the deleted copy ctor below suppresses the implicit one

    void prepare(double sampleRate, int maxBlockSize);
    void releaseResources();

    // -50..+50 (%), 0 = normal speed — same range/clamp as Deck::setPitch's
    // linked-speed ratio, kept independent of pitch.
    void setTempoPercent(float percent);

    // Discards SoundTouch's internal FIFO/algorithm state. Call on
    // seek/stop/unload/mode-switch to avoid stale audio bleeding in after a
    // playback discontinuity.
    void reset();

    // Pulls exactly bufferToFill.numSamples stretched frames, pulling raw
    // input from `input` (Deck's transportSource) as needed. `input` must
    // already be prepared by the caller (Deck owns/prepares it).
    void getNextAudioBlock(juce::AudioSource& input,
                            const juce::AudioSourceChannelInfo& bufferToFill);

private:
    static constexpr int numChannels = 2;

    soundtouch::SoundTouch soundTouch;

    juce::AudioBuffer<float> inputScratch;   // pulled from `input`, de-interleaved
    std::vector<float> interleavedInput;     // preallocated, sized in prepare()
    std::vector<float> interleavedOutput;    // preallocated, sized in prepare()

    int feedChunkFrames = 0;                 // == maxBlockSize, reused every feed iteration

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TimeStretch)
};

} // namespace mixdeck
