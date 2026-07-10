#include "Mixer.h"

namespace mixdeck {

namespace {
constexpr float cutCurveWidth = 0.08f;

void computeCrossfaderGains(float x, CrossfaderCurve curve, float& gainA, float& gainB) {
    x = juce::jlimit(0.0f, 1.0f, x);

    switch (curve) {
        case CrossfaderCurve::Linear:
            gainA = 1.0f - x;
            gainB = x;
            break;

        case CrossfaderCurve::Smooth:
            gainA = std::cos(x * juce::MathConstants<float>::halfPi);
            gainB = std::sin(x * juce::MathConstants<float>::halfPi);
            break;

        case CrossfaderCurve::Cut:
            gainA = x <= 1.0f - cutCurveWidth ? 1.0f : (1.0f - x) / cutCurveWidth;
            gainB = x >= cutCurveWidth ? 1.0f : x / cutCurveWidth;
            break;
    }
}
} // namespace

Mixer::Mixer(Deck& deckAToControl, Deck& deckBToControl)
    : deckA(deckAToControl), deckB(deckBToControl) {
    recomputeGains();
}

void Mixer::setDeckVolume(int deckIndex, float volume01) {
    volume01 = juce::jlimit(0.0f, 1.0f, volume01);

    if (deckIndex == 0)
        volumeA = volume01;
    else
        volumeB = volume01;

    recomputeGains();
}

void Mixer::setCrossfaderPosition(float position01) {
    crossfaderPosition = juce::jlimit(0.0f, 1.0f, position01);
    recomputeGains();
}

void Mixer::setCrossfaderCurve(CrossfaderCurve newCurve) {
    curve = newCurve;
    recomputeGains();
}

void Mixer::recomputeGains() {
    float crossfaderGainA = 1.0f;
    float crossfaderGainB = 1.0f;
    computeCrossfaderGains(crossfaderPosition, curve, crossfaderGainA, crossfaderGainB);

    deckA.setGain(volumeA * crossfaderGainA);
    deckB.setGain(volumeB * crossfaderGainB);
}

} // namespace mixdeck
