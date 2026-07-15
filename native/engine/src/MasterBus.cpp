#include "MasterBus.h"

namespace mixdeck {

MasterBus::MasterBus(juce::AudioSource& sourceToWrap, PluginHost& pluginHost, juce::ThreadPool& backgroundPool)
    : wrapped(sourceToWrap), pluginChain(pluginHost, backgroundPool) {}

void MasterBus::prepareToPlay(int samplesPerBlockExpected, double sampleRate) {
    wrapped.prepareToPlay(samplesPerBlockExpected, sampleRate);
    pluginChain.prepare(sampleRate, samplesPerBlockExpected);
}

void MasterBus::releaseResources() {
    wrapped.releaseResources();
    pluginChain.releaseResources();
}

void MasterBus::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) {
    wrapped.getNextAudioBlock(bufferToFill);
    pluginChain.process(bufferToFill);
}

} // namespace mixdeck
