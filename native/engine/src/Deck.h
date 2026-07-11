#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "FilterDSP.h"

namespace mixdeck {

// Engine API state machine (ADR-016) — single source of truth for the UI.
enum class DeckState { Empty, Loading, Ready, Playing, Paused, Stopped, Error };

// "EMPTY"/"LOADING"/... — shared by the standalone harness (DeckPanel) and the
// Bridge (NodeBinding, Story 2.2).
juce::String toString(DeckState state);

/** One independent playback deck: loads a single audio file, plays/pauses/stops
    it, applies a gain set by Mixer (fader x crossfader, Story 1.2), a per-deck
    resonant filter (Story 1.3), and a "linked speed" pitch via resampling
    (Story 1.4 — pitch follows speed, like a vinyl). Exposes a DeckState (2.1,
    ADR-016) so the future UI never has to reconstruct status from ad hoc flags. */
class Deck : public juce::AudioSource {
public:
    explicit Deck(juce::AudioFormatManager& formatManagerToUse);
    ~Deck() override;

    // Returns an error message on failure, or an empty string on success.
    juce::String loadFile(const juce::File& file);
    void unloadTrack();

    void play();
    void pause();
    void stop();
    void seek(double seconds);
    bool isPlaying() const;

    // Lazily detects natural end-of-track before returning the current state.
    DeckState getState() const;

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
    mutable DeckState state = DeckState::Empty;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Deck)
};

} // namespace mixdeck
