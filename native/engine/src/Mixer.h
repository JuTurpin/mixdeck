#pragma once

#include "Deck.h"

namespace mixdeck {

enum class CrossfaderCurve { Linear, Smooth, Cut };

/** Computes the combined gain (per-deck fader x crossfader) and pushes it to
    each Deck via Deck::setGain(). Not an AudioSource itself: audio summing
    stays on the plain juce::MixerAudioSource in MainComponent — this class
    only owns the gain math. */
class Mixer {
public:
    Mixer(Deck& deckAToControl, Deck& deckBToControl);

    void setDeckVolume(int deckIndex, float volume01); // 0 = deckA, 1 = deckB
    void setCrossfaderPosition(float position01);       // 0 = full A, 1 = full B
    void setCrossfaderCurve(CrossfaderCurve newCurve);

private:
    void recomputeGains();

    Deck& deckA;
    Deck& deckB;

    float volumeA = 1.0f;
    float volumeB = 1.0f;
    float crossfaderPosition = 0.5f;
    CrossfaderCurve curve = CrossfaderCurve::Smooth;
};

} // namespace mixdeck
