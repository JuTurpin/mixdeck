#include "PluginHost.h"
#include <algorithm>

namespace mixdeck {

namespace {
// Adds/replaces entries in `into` (keyed by PluginDescription::createIdentifierString())
// without touching entries not present in `found` — so a fresh scan never wipes out a
// plugin added manually from outside the standard folders (Story 4.2), and adding the
// same plugin twice never creates a duplicate.
void mergeDescriptions(std::vector<juce::PluginDescription>& into,
                       const std::vector<juce::PluginDescription>& found) {
    for (const auto& description : found) {
        const auto identifier = description.createIdentifierString();
        const auto existing = std::find_if(into.begin(), into.end(), [&](const auto& d) {
            return d.createIdentifierString() == identifier;
        });
        if (existing != into.end())
            *existing = description;
        else
            into.push_back(description);
    }
}
} // namespace

PluginHost::PluginHost() {
    juce::addDefaultFormatsToManager(formatManager);
}

void PluginHost::scanForPlugins() {
    // Local to this scan — never shared, so no locking needed while the
    // (potentially slow) scan itself runs. Only the final merge below needs
    // the mutex.
    juce::KnownPluginList scratchList;

    for (int i = 0; i < formatManager.getNumFormats(); ++i) {
        auto* format = formatManager.getFormat(i);
        juce::PluginDirectoryScanner scanner(scratchList,
                                              *format,
                                              format->getDefaultLocationsToSearch(),
                                              true,          // searchRecursively
                                              juce::File()); // no deadMansPedalFile (4.4 will add isolation)

        juce::String pluginBeingScanned;
        while (scanner.scanNextFile(true, pluginBeingScanned)) {
        }
    }

    const auto types = scratchList.getTypes();
    const std::vector<juce::PluginDescription> found(types.begin(), types.end());

    std::lock_guard<std::mutex> lock(mutex);
    mergeDescriptions(availablePlugins, found);
}

std::vector<juce::PluginDescription> PluginHost::getAvailablePlugins() const {
    std::lock_guard<std::mutex> lock(mutex);
    return availablePlugins;
}

juce::String PluginHost::addPluginFromPath(const juce::String& path) {
    for (int i = 0; i < formatManager.getNumFormats(); ++i) {
        auto* format = formatManager.getFormat(i);
        if (!format->fileMightContainThisPluginType(path))
            continue;

        juce::OwnedArray<juce::PluginDescription> found;
        format->findAllTypesForFile(found, path);
        if (found.isEmpty())
            continue;

        std::vector<juce::PluginDescription> descriptions;
        for (auto* description : found)
            descriptions.push_back(*description);

        std::lock_guard<std::mutex> lock(mutex);
        mergeDescriptions(availablePlugins, descriptions);
        return {};
    }

    return "Format de plugin non reconnu : " + juce::File(path).getFileName();
}

bool PluginHost::findPluginDescription(const juce::String& pluginIdentifier,
                                        juce::PluginDescription& outDescription) const {
    std::lock_guard<std::mutex> lock(mutex);
    const auto found = std::find_if(availablePlugins.begin(), availablePlugins.end(), [&](const auto& d) {
        return d.createIdentifierString() == pluginIdentifier;
    });
    if (found == availablePlugins.end())
        return false;
    outDescription = *found;
    return true;
}

juce::File PluginHost::guessDefaultWorkerExecutablePath() {
    // Correct when running as MixDeckStandalone (built alongside
    // MixDeckPluginWorker in the same CMake tree: <buildRoot>/
    // MixDeckStandalone_artefacts/Release/MixDeck Standalone.app/Contents/
    // MacOS/MixDeck Standalone, walking up 5 levels reaches <buildRoot>).
    // Wrong for the Node/Electron bridge, whose own "current executable" is
    // Node/Electron itself, not mixdeck_bridge.node — Electron overrides this
    // explicitly at startup instead (setWorkerExecutablePath), same reasoning
    // as hostWindowHandle being injected rather than auto-discovered.
    const auto exe = juce::File::getSpecialLocation(juce::File::currentExecutableFile);
    const auto buildRoot = exe.getParentDirectory()  // MacOS
                                .getParentDirectory() // Contents
                                .getParentDirectory() // *.app
                                .getParentDirectory() // Release
                                .getParentDirectory(); // <Target>_artefacts
    return buildRoot.getChildFile(
        "MixDeckPluginWorker_artefacts/Release/MixDeck Plugin Worker.app/Contents/MacOS/MixDeck Plugin Worker");
}

std::unique_ptr<juce::AudioPluginInstance> PluginHost::instantiatePlugin(
    const juce::String& pluginIdentifier, double sampleRate, int blockSize, juce::String& error) {
    juce::PluginDescription description;
    if (!findPluginDescription(pluginIdentifier, description)) {
        error = "Plugin inconnu : " + pluginIdentifier;
        return nullptr;
    }

    auto instance = formatManager.createPluginInstance(description, sampleRate, blockSize, error);
    if (instance == nullptr)
        return nullptr;

    juce::AudioProcessor::BusesLayout stereoInOut;
    stereoInOut.inputBuses.add(juce::AudioChannelSet::stereo());
    stereoInOut.outputBuses.add(juce::AudioChannelSet::stereo());
    if (!instance->checkBusesLayoutSupported(stereoInOut) || !instance->setBusesLayout(stereoInOut)) {
        error = "Ce plugin ne supporte pas un bus stereo standard (entree + sortie).";
        return nullptr;
    }

    return instance;
}

} // namespace mixdeck
