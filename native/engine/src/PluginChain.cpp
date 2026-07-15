#include "PluginChain.h"

namespace mixdeck {

PluginChain::EditorHost::EditorHost(std::unique_ptr<juce::AudioProcessorEditor> editorToOwn, juce::String pluginName)
    : editor(std::move(editorToOwn)), name(std::move(pluginName)) {
    addAndMakeVisible(*editor);
    addAndMakeVisible(minimizeButton);
    minimizeButton.setButtonText(juce::String::fromUTF8("\xE2\x94\x80")); // "─"
    minimizeButton.onClick = [this] { setVisible(false); };
    setSize(editor->getWidth(), editor->getHeight() + headerHeight);
}

void PluginChain::EditorHost::resized() {
    auto area = getLocalBounds();
    auto header = area.removeFromTop(headerHeight);
    minimizeButton.setBounds(header.removeFromRight(headerHeight).reduced(3));
    editor->setBounds(area);
}

void PluginChain::EditorHost::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(0xff1d2026));
    g.setColour(juce::Colours::white);
    g.setFont(juce::FontOptions(12.0f));
    g.drawText(name, getLocalBounds().removeFromTop(headerHeight).withTrimmedLeft(8),
                juce::Justification::centredLeft);
}

void PluginChain::EditorHost::childBoundsChanged(juce::Component* child) {
    if (child == editor.get())
        setSize(editor->getWidth(), editor->getHeight() + headerHeight);
}

PluginChain::PluginChain(PluginHost& pluginHostToUse, juce::ThreadPool& backgroundPoolToUse)
    : pluginHost(pluginHostToUse), backgroundPool(backgroundPoolToUse) {
    auto initial = std::make_shared<const ChainSnapshot>();
    activeChain.store(initial.get());
    generations.push_back(std::move(initial));
}

void PluginChain::prepare(double sampleRate, int maxBlockSize) {
    currentSampleRate = sampleRate;
    currentBlockSize = maxBlockSize;

    const auto snapshot = activeChain.load();
    for (const auto& slot : snapshot->slots)
        slot->instance->prepareToPlay(sampleRate, maxBlockSize);
}

void PluginChain::releaseResources() {
    const auto snapshot = activeChain.load();
    for (const auto& slot : snapshot->slots)
        slot->instance->releaseResources();
}

void PluginChain::process(const juce::AudioSourceChannelInfo& bufferToFill) {
    const auto snapshot = activeChain.load(); // single atomic load — no lock, no allocation

    if (snapshot->slots.empty())
        return;

    juce::AudioBuffer<float> view(bufferToFill.buffer->getArrayOfWritePointers(),
                                   bufferToFill.buffer->getNumChannels(),
                                   bufferToFill.startSample,
                                   bufferToFill.numSamples);

    for (const auto& slot : snapshot->slots) {
        if (slot->bypassed.load())
            continue;
        slot->instance->processBlock(view, scratchMidiBuffer);
        scratchMidiBuffer.clear(); // a plugin may emit MIDI even without receiving any
    }
}

void PluginChain::addPlugin(const juce::String& pluginIdentifier) {
    if (addingPlugin.exchange(true))
        return; // an add is already in flight

    {
        std::lock_guard<std::mutex> lock(addErrorMutex);
        lastAddError = {};
    }

    backgroundPool.addJob([this, pluginIdentifier] {
        juce::String error;
        auto instance = pluginHost.instantiatePlugin(pluginIdentifier, currentSampleRate, currentBlockSize, error);

        if (instance != nullptr) {
            instance->prepareToPlay(currentSampleRate, currentBlockSize);

            auto slot = std::make_shared<Slot>();
            slot->name = instance->getName();
            slot->instance = std::move(instance);

            std::lock_guard<std::mutex> lock(controlMutex);
            const auto current = activeChain.load();
            auto next = std::make_shared<ChainSnapshot>(*current);
            next->slots.push_back(std::move(slot));
            publishLocked(std::move(next));
        } else {
            std::lock_guard<std::mutex> lock(addErrorMutex);
            lastAddError = error.isNotEmpty() ? error : "Impossible de charger le plugin.";
        }

        addingPlugin = false;
    });
}

juce::String PluginChain::getLastAddError() const {
    std::lock_guard<std::mutex> lock(addErrorMutex);
    return lastAddError;
}

void PluginChain::removePlugin(int index) {
    std::lock_guard<std::mutex> lock(controlMutex);
    const auto current = activeChain.load();
    if (index < 0 || (size_t) index >= current->slots.size())
        return;

    // Editor teardown is message-thread-only (like any juce::Component) — do
    // it before publishing a snapshot without this slot. removePlugin() is
    // only ever called from the message thread (via the Bridge), so this is safe.
    current->slots[(size_t) index]->editorHost.reset();

    auto next = std::make_shared<ChainSnapshot>();
    for (size_t i = 0; i < current->slots.size(); ++i)
        if (i != (size_t) index)
            next->slots.push_back(current->slots[i]);
    publishLocked(std::move(next));
}

void PluginChain::movePlugin(int fromIndex, int toIndex) {
    std::lock_guard<std::mutex> lock(controlMutex);
    const auto current = activeChain.load();
    const auto count = current->slots.size();
    if (fromIndex < 0 || (size_t) fromIndex >= count || toIndex < 0 || (size_t) toIndex >= count
        || fromIndex == toIndex)
        return;

    auto slots = current->slots; // copy of the shared_ptr vector — slots themselves aren't duplicated
    auto moved = slots[(size_t) fromIndex];
    slots.erase(slots.begin() + fromIndex);
    slots.insert(slots.begin() + toIndex, moved);

    auto next = std::make_shared<ChainSnapshot>();
    next->slots = std::move(slots);
    publishLocked(std::move(next));
}

void PluginChain::setPluginBypassed(int index, bool bypassed) {
    const auto current = activeChain.load();
    if (index < 0 || (size_t) index >= current->slots.size())
        return;
    // No new snapshot needed: bypassed is itself atomic, mutated in place.
    current->slots[(size_t) index]->bypassed.store(bypassed);
}

void PluginChain::showPluginEditor(int index, void* hostWindowHandle) {
    const auto current = activeChain.load();
    if (index < 0 || (size_t) index >= current->slots.size())
        return;

    auto& slot = *current->slots[(size_t) index];
    if (slot.editorHost == nullptr) {
        std::unique_ptr<juce::AudioProcessorEditor> editor(slot.instance->createEditorAndMakeActive());
        if (editor == nullptr)
            return;
        slot.editorHost = std::make_unique<EditorHost>(std::move(editor), slot.name);
    }

    // hostWindowHandle == nullptr => a real top-level window (harness, no
    // foreign parent to embed into); non-null => embedded child view, the
    // mechanism proven in Story 4.0 (Electron's window).
    const auto styleFlags = hostWindowHandle == nullptr
                                 ? (juce::ComponentPeer::windowHasTitleBar
                                    | juce::ComponentPeer::windowHasCloseButton
                                    | juce::ComponentPeer::windowAppearsOnTaskbar)
                                 : 0;
    // (16, 40) instead of the top-left corner: on Electron, hostWindowHandle
    // is the whole window's content view, and (0,0) is exactly where the
    // frameless-window fake title bar (TitleBar.tsx) draws its close/
    // minimize/maximize dots — starting the editor there would bury them
    // under this embedded view on every open.
    slot.editorHost->setTopLeftPosition(16, 40);
    slot.editorHost->addToDesktop(styleFlags, hostWindowHandle);
    slot.editorHost->setVisible(true);
    slot.editorHost->toFront(false);
}

void PluginChain::hidePluginEditor(int index) {
    const auto current = activeChain.load();
    if (index < 0 || (size_t) index >= current->slots.size())
        return;
    if (auto& host = current->slots[(size_t) index]->editorHost)
        host->setVisible(false);
}

std::vector<PluginChain::SlotInfo> PluginChain::getSlots() const {
    const auto snapshot = activeChain.load();
    std::vector<SlotInfo> result;
    result.reserve(snapshot->slots.size());
    for (const auto& slot : snapshot->slots)
        result.push_back({ slot->name, slot->bypassed.load() });
    return result;
}

void PluginChain::publishLocked(std::shared_ptr<const ChainSnapshot> next) {
    activeChain.store(next.get());
    generations.push_back(std::move(next)); // keeps the new generation alive; also keeps the old one(s) alive via the entries already in generations

    // Only the oldest generation(s) beyond the grace period are actually
    // freed — by which point activeChain has moved on and any audio callback
    // that had grabbed the old pointer has long since returned.
    while (generations.size() > maxRetiredGenerations + 1)
        generations.erase(generations.begin());
}

} // namespace mixdeck
