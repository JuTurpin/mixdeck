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

} // namespace mixdeck
