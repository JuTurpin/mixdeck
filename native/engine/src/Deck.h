#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "FilterDSP.h"

namespace mixdeck {

/** One independent playback deck: loads a single audio file, plays/stops it,
    applies a gain set by Mixer (fader x crossfader, Story 1.2), a per-deck
    resonant filter (Story 1.3), and a "linked speed" pitch via resampling
    (Story 1.4 — pitch follows speed, like a vinyl). */
class Deck : public juce::AudioSource {
public:
    explicit Deck(juce::AudioFormatManager& formatManagerToUse);
    ~Deck() override;

    // Returns an error message on failure, or an empty string on success.
    juce::String loadFile(const juce::File& file);

    void play();
    void stop();
    bool isPlaying() const;

    // Combined gain applied by Mixer (fader x crossfader) — see Mixer.h.
    void setGain(float gain);

    // -1..+1, 0 = neutral — see FilterDSP.h.
    void setFilterKnob(float value);

    // -50..+50 (%), 0 = normal speed. Pitch follows speed (resampling), like a vinyl.
    void setPitch(float percent);

    double getPositionSeconds() const;
    double getLengthSeconds() const;
    juce::String getLoadedFileName() const { return loadedFileName; }

    // juce::AudioSource
    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void releaseResources() override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;

private:
    juce::AudioFormatManager& formatManager;
    juce::AudioTransportSource transportSource;
    juce::ResamplingAudioSource pitchResampler { &transportSource, false, 2 };
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
    juce::TimeSliceThread readAheadThread { "mixdeck-deck-read-ahead" };
    juce::String loadedFileName;
    FilterDSP filter;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Deck)
};

} // namespace mixdeck
