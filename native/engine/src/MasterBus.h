#pragma once

#include "PluginChain.h"
#include <juce_audio_basics/juce_audio_basics.h>

namespace mixdeck {

/** Story 4.3 — wraps the existing deckA/deckB mix (a plain juce::AudioSource,
    e.g. Engine's juce::MixerAudioSource) and applies a PluginChain after
    pulling from it. Engine/MainComponent point their audioSourcePlayer at
    this instead of at the mixer directly. */
class MasterBus : public juce::AudioSource {
public:
    MasterBus(juce::AudioSource& sourceToWrap, PluginHost& pluginHost, juce::ThreadPool& backgroundPool);

    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void releaseResources() override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;

    PluginChain& getPluginChain() { return pluginChain; }

private:
    juce::AudioSource& wrapped;
    PluginChain pluginChain;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MasterBus)
};

} // namespace mixdeck
