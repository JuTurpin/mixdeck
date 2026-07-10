#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_utils/juce_audio_utils.h>

namespace mixdeck {

/** One independent playback deck: loads a single audio file, plays/stops it, and
    applies a gain set by Mixer (fader x crossfader, Story 1.2). Filter (1.3) and
    pitch (1.4) extend this class in later stories. */
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
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
    juce::TimeSliceThread readAheadThread { "mixdeck-deck-read-ahead" };
    juce::String loadedFileName;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Deck)
};

} // namespace mixdeck
