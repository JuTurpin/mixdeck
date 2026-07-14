#include "PluginHost.h"

namespace mixdeck {

PluginHost::PluginHost() {
    juce::addDefaultFormatsToManager(formatManager);
}

void PluginHost::scanForPlugins() {
    // Local to this scan — never shared, so no locking needed while the
    // (potentially slow) scan itself runs. Only the final publication of
    // results below needs the mutex.
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
    std::vector<juce::PluginDescription> results(types.begin(), types.end());

    std::lock_guard<std::mutex> lock(mutex);
    availablePlugins = std::move(results);
}

std::vector<juce::PluginDescription> PluginHost::getAvailablePlugins() const {
    std::lock_guard<std::mutex> lock(mutex);
    return availablePlugins;
}

} // namespace mixdeck
