#pragma once

#include <juce_audio_formats/juce_audio_formats.h>
#include <vector>

namespace mixdeck {

struct BpmAnalysis {
    float bpm = 0.0f;                      // 0 = detection failed
    std::vector<float> beatGridSeconds;     // detected beat positions, in seconds
};

/** Story 3.2 (roadmap "détection BPM à l'import") — analyzes a whole audio
    file via SoundTouch's BPMDetect (vendored in Story 3.1). Opens its own
    AudioFormatReader, independent of any reader a Deck may already be using
    for playback, and reads it block by block until exhausted. Pure CPU/file
    I/O with no JUCE audio-thread assumptions — meant to run on a background
    thread (Engine's analysisPool), never the real-time audio callback. */
BpmAnalysis analyzeBpm(const juce::File& file, juce::AudioFormatManager& formatManager);

} // namespace mixdeck
