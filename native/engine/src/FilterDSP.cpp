#include "FilterDSP.h"

namespace mixdeck {

namespace {
constexpr float deadZone = 0.02f;

float expInterp(float from, float to, float t) {
    return from * std::pow(to / from, t);
}
} // namespace

void FilterDSP::prepare(const juce::dsp::ProcessSpec& spec) {
    filter.prepare(spec);
}

void FilterDSP::setKnobPosition(float value) {
    value = juce::jlimit(-1.0f, 1.0f, value);

    if (std::abs(value) <= deadZone) {
        if (!bypassed)
            filter.reset();
        bypassed = true;
        return;
    }

    bypassed = false;

    if (value > 0.0f) {
        const auto t = (value - deadZone) / (1.0f - deadZone);
        filter.setType(juce::dsp::StateVariableTPTFilterType::highpass);
        filter.setCutoffFrequency(expInterp(20.0f, 8000.0f, t));
        filter.setResonance(juce::jmap(t, 0.7071f, 4.0f));
    } else {
        const auto t = (-value - deadZone) / (1.0f - deadZone);
        filter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
        filter.setCutoffFrequency(expInterp(20000.0f, 200.0f, t));
        filter.setResonance(juce::jmap(t, 0.7071f, 4.0f));
    }
}

void FilterDSP::process(juce::dsp::AudioBlock<float>& block) {
    if (bypassed)
        return;

    juce::dsp::ProcessContextReplacing<float> context(block);
    filter.process(context);
}

} // namespace mixdeck
