#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <atomic>
#include <mutex>
#include "FilterDSP.h"
#include "TimeStretch.h"

namespace mixdeck {

// Engine API state machine (ADR-016) — single source of truth for the UI.
enum class DeckState { Empty, Loading, Ready, Playing, Paused, Stopped, Error };

// "EMPTY"/"LOADING"/... — shared by the standalone harness (DeckPanel) and the
// Bridge (NodeBinding, Story 2.2).
juce::String toString(DeckState state);

// Story 3.1 (ADR-006) — which algorithm interprets the pitch slider:
// LinkedSpeed (Story 1.4, default) changes speed and pitch together, like a
// vinyl. Independent (SoundTouch) changes tempo while keeping pitch fixed.
enum class PitchMode { LinkedSpeed, Independent };

// "linked"/"independent" — shared by the standalone harness (DeckPanel) and
// the Bridge (NodeBinding).
juce::String toString(PitchMode mode);

/** One independent playback deck: loads a single audio file, plays/pauses/stops
    it, applies a gain set by Mixer (fader x crossfader, Story 1.2), a per-deck
    resonant filter (Story 1.3), and a pitch slider that can follow either mode:
    "linked speed" via resampling (Story 1.4 — pitch follows speed, like a
    vinyl) or independent time-stretch via SoundTouch (Story 3.1 — tempo
    changes without shifting pitch). Exposes a DeckState (2.1, ADR-016) so the
    future UI never has to reconstruct status from ad hoc flags. */
class Deck : public juce::AudioSource {
public:
    // analysisPoolToUse: shared background pool for BPM analysis (Story 3.2),
    // owned by Engine/MainComponent — at most one job per Deck in flight.
    Deck(juce::AudioFormatManager& formatManagerToUse, juce::ThreadPool& analysisPoolToUse);
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

    // -50..+50 (%), 0 = normal speed. Interpreted according to pitchMode:
    // LinkedSpeed (resampling, like a vinyl) or Independent (SoundTouch).
    void setPitch(float percent);

    // Switches which algorithm interprets setPitch()'s percent (Story 3.1).
    void setPitchMode(PitchMode mode);
    PitchMode getPitchMode() const { return pitchMode; }

    double getPositionSeconds() const;
    double getLengthSeconds() const;
    juce::String getLoadedFileName() const { return loadedFileName; }

    // Story 3.2 — detected at load time on a background thread (see
    // analysisPool). 0 = not yet analyzed (or detection failed).
    float getBpm() const;

    // Beat positions in seconds. C++-only for now: no Bridge/UI consumer yet
    // (no waveform to draw it on).
    std::vector<float> getBeatGrid() const;

    // juce::AudioSource
    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void releaseResources() override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;

private:
    void startBpmAnalysis(const juce::File& file);

    juce::AudioFormatManager& formatManager;
    juce::ThreadPool& analysisPool;
    juce::AudioTransportSource transportSource;
    juce::ResamplingAudioSource pitchResampler { &transportSource, false, 2 };
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
    juce::TimeSliceThread readAheadThread { "mixdeck-deck-read-ahead" };
    juce::String loadedFileName;
    FilterDSP filter;
    TimeStretch timeStretch;
    PitchMode pitchMode = PitchMode::LinkedSpeed; // default = unchanged Epic 1 behaviour
    mutable DeckState state = DeckState::Empty;

    // Story 3.2 — written by a background analysis job, read by getBpm()/
    // getBeatGrid() (called from the Node poll loop, ~5Hz — not the audio
    // thread, so a plain mutex is fine here, no lock-free trickery needed).
    mutable std::mutex analysisMutex;
    float bpm = 0.0f;
    std::vector<float> beatGrid;
    // Incremented on every loadFile()/unloadTrack(): a job only commits its
    // result if this hasn't moved on since it started, so a stale analysis
    // from a quickly-replaced track can't clobber the current one's state.
    std::atomic<int> loadGeneration { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Deck)
};

} // namespace mixdeck
