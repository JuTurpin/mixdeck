#pragma once

#include <juce_dsp/juce_dsp.h>

namespace mixdeck {

/** Single-knob resonant filter, DJ-mixer style: center = neutral (true bypass,
    no coloration), left = lowpass closing in, right = highpass opening up. */
class FilterDSP {
public:
    void prepare(const juce::dsp::ProcessSpec& spec);

    // -1..+1, 0 = neutral.
    void setKnobPosition(float value);

    void process(juce::dsp::AudioBlock<float>& block);

private:
    juce::dsp::StateVariableTPTFilter<float> filter;
    bool bypassed = true;
};

} // namespace mixdeck
