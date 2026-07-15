#include "PluginWorkerProtocol.h"

namespace mixdeck {

WorkerMessageType peekMessageType(const juce::MemoryBlock& message) {
    jassert(message.getSize() > 0);
    return static_cast<WorkerMessageType>(*static_cast<const uint8_t*>(message.getData()));
}

juce::MemoryBlock makeLoadPluginMessage(const juce::PluginDescription& description, double sampleRate,
                                         int blockSize) {
    juce::MemoryBlock block;
    juce::MemoryOutputStream out(block, false);
    out.writeByte(static_cast<char>(WorkerMessageType::loadPlugin));
    out.writeString(description.createXml()->toString());
    out.writeDouble(sampleRate);
    out.writeInt(blockSize);
    return block;
}

bool readLoadPluginMessage(const juce::MemoryBlock& message, juce::PluginDescription& outDescription,
                            double& outSampleRate, int& outBlockSize) {
    juce::MemoryInputStream in(message, false);
    in.readByte(); // type, already dispatched on by the caller via peekMessageType

    const auto xml = juce::parseXML(in.readString());
    if (xml == nullptr || !outDescription.loadFromXml(*xml))
        return false;

    outSampleRate = in.readDouble();
    outBlockSize = in.readInt();
    return true;
}

juce::MemoryBlock makeSimpleMessage(WorkerMessageType type) {
    juce::MemoryBlock block;
    juce::MemoryOutputStream out(block, false);
    out.writeByte(static_cast<char>(type));
    return block;
}

juce::MemoryBlock makePluginLoadFailedMessage(const juce::String& error) {
    juce::MemoryBlock block;
    juce::MemoryOutputStream out(block, false);
    out.writeByte(static_cast<char>(WorkerMessageType::pluginLoadFailed));
    out.writeString(error);
    return block;
}

juce::String readErrorMessage(const juce::MemoryBlock& message) {
    juce::MemoryInputStream in(message, false);
    in.readByte(); // type
    return in.readString();
}

juce::MemoryBlock makeAudioBlockMessage(const juce::AudioBuffer<float>& buffer) {
    juce::MemoryBlock block;
    juce::MemoryOutputStream out(block, false);
    out.writeByte(static_cast<char>(WorkerMessageType::audioBlock));
    out.writeInt(buffer.getNumChannels());
    out.writeInt(buffer.getNumSamples());
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        out.write(buffer.getReadPointer(channel), (size_t) buffer.getNumSamples() * sizeof(float));
    return block;
}

bool readAudioBlockMessage(const juce::MemoryBlock& message, juce::AudioBuffer<float>& outBuffer) {
    juce::MemoryInputStream in(message, false);
    in.readByte(); // type

    const auto numChannels = in.readInt();
    const auto numSamples = in.readInt();
    if (numChannels != outBuffer.getNumChannels() || numSamples != outBuffer.getNumSamples())
        return false; // both sides agreed on this shape when the plugin was loaded — anything else is malformed

    for (int channel = 0; channel < numChannels; ++channel) {
        const auto bytesRead = in.read(outBuffer.getWritePointer(channel), (size_t) numSamples * sizeof(float));
        if (bytesRead != (size_t) numSamples * sizeof(float))
            return false;
    }
    return true;
}

} // namespace mixdeck
