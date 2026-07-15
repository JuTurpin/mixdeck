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
    for (const auto& slot : snapshot->slots) {
        if (slot->instance != nullptr) // null for an isolated slot
            slot->instance->prepareToPlay(sampleRate, maxBlockSize);
        // Story 4.4.2 — MixDeck is stereo throughout (Deck/Mixer/MasterBus),
        // hardcoded here same as the worker side (PluginWorkerMain.cpp).
        if (slot->isolatedHost != nullptr)
            slot->isolatedHost->prepareAudio(2, maxBlockSize);
    }
}

void PluginChain::releaseResources() {
    const auto snapshot = activeChain.load();
    for (const auto& slot : snapshot->slots) {
        if (slot->instance != nullptr)
            slot->instance->releaseResources();
        if (slot->isolatedHost != nullptr)
            slot->isolatedHost->releaseAudio();
    }
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

        if (slot->isolated) {
            // Story 4.4.2 — never touches the IPC connection directly (see
            // IsolatedPluginHost): pushInputBlock/popOutputBlock only read/
            // write small preallocated lock-free queues, never block, never
            // allocate. If nothing is ready yet (just added, or the worker
            // has crashed/fallen behind), popOutputBlock leaves `view`
            // untouched — the dry signal keeps passing through rather than
            // cutting to silence.
            if (slot->isolatedHost != nullptr) {
                slot->isolatedHost->pushInputBlock(view);
                slot->isolatedHost->popOutputBlock(view);
            }
            continue;
        }

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

    juce::PluginDescription description;
    if (!pluginHost.findPluginDescription(pluginIdentifier, description)) {
        std::lock_guard<std::mutex> lock(addErrorMutex);
        lastAddError = "Plugin inconnu : " + pluginIdentifier;
        addingPlugin = false;
        return;
    }

    // Story 4.4.1 (ADR-005) — VST3 is isolated in its own worker process; AU
    // stays in-process, as it generally must on macOS anyway (AppKit
    // constraints). Only the in-process path needs the background pool:
    // instantiation there can be genuinely slow (plugin scan/init on disk).
    // The isolated path's own instantiation happens inside the worker
    // process and reports back asynchronously regardless of what thread
    // kicks it off.
    if (description.pluginFormatName == "VST3")
        addIsolatedPlugin(description);
    else
        addInProcessPlugin(pluginIdentifier);
}

void PluginChain::addInProcessPlugin(const juce::String& pluginIdentifier) {
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

void PluginChain::addIsolatedPlugin(const juce::PluginDescription& description) {
    auto slot = std::make_shared<Slot>();
    slot->name = description.name;
    slot->isolated = true;
    slot->isolatedHost = std::make_unique<IsolatedPluginHost>();

    // Raw pointer, not a captured shared_ptr<Slot>: onCrashed is a member of
    // isolatedHost, which is itself owned by this same Slot — capturing the
    // Slot's own shared_ptr here would create an ownership cycle (Slot ->
    // owns -> IsolatedPluginHost -> closure -> shared_ptr -> Slot) that would
    // leak both the Slot and its worker process. Safe as a raw pointer: the
    // closure is destroyed together with isolatedHost, which is destroyed
    // together with the Slot — it can never fire after the Slot is gone.
    auto* slotPtr = slot.get();
    slot->isolatedHost->onCrashed = [slotPtr] { slotPtr->crashed = true; };

    // Keeps the Slot alive between here and the load callback below — it has
    // no other owner yet (not published into a ChainSnapshot until success).
    pendingIsolatedSlot = slot;

    slot->isolatedHost->load(pluginHost.getWorkerExecutablePath(), description, currentSampleRate, currentBlockSize,
        [this](bool success, juce::String error) {
            auto resolvedSlot = std::exchange(pendingIsolatedSlot, nullptr);

            if (success) {
                // Story 4.4.2 — self-prepares using the chain's current
                // sample rate/block size, same reasoning as
                // addInProcessPlugin's instance->prepareToPlay() call right
                // below: a plugin can be added while already playing, well
                // after PluginChain::prepare() last ran, so it can't wait for
                // the next one.
                resolvedSlot->isolatedHost->prepareAudio(2, currentBlockSize);

                std::lock_guard<std::mutex> lock(controlMutex);
                const auto current = activeChain.load();
                auto next = std::make_shared<ChainSnapshot>(*current);
                next->slots.push_back(std::move(resolvedSlot));
                publishLocked(std::move(next));
            } else {
                std::lock_guard<std::mutex> lock(addErrorMutex);
                lastAddError = error.isNotEmpty() ? error : "Impossible de charger le plugin isole.";
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

    // Editor/worker teardown is message-thread-only (like any juce::Component,
    // and IsolatedPluginHost's own coordinator/worker plumbing) — do it before
    // publishing a snapshot without this slot. removePlugin() is only ever
    // called from the message thread (via the Bridge), so this is safe.
    // Exactly one of the two is ever non-null; resetting both unconditionally
    // avoids branching on `isolated` here.
    current->slots[(size_t) index]->editorHost.reset();
    current->slots[(size_t) index]->isolatedHost.reset(); // kills the worker process (ChildProcessCoordinator dtor)

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

    // Story 4.4.1 — an isolated plugin's editor lives in its own process: it
    // opens as an independent top-level window there (see PluginWorkerMain),
    // not incrusted here — hostWindowHandle is irrelevant for this path.
    if (slot.isolated) {
        if (slot.isolatedHost != nullptr)
            slot.isolatedHost->showEditor();
        return;
    }

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

    auto& slot = *current->slots[(size_t) index];
    if (slot.isolated) {
        if (slot.isolatedHost != nullptr)
            slot.isolatedHost->hideEditor();
        return;
    }

    if (auto& host = slot.editorHost)
        host->setVisible(false);
}

std::vector<PluginChain::SlotInfo> PluginChain::getSlots() const {
    const auto snapshot = activeChain.load();
    std::vector<SlotInfo> result;
    result.reserve(snapshot->slots.size());
    for (const auto& slot : snapshot->slots)
        result.push_back({ slot->name, slot->bypassed.load(), slot->isolated, slot->crashed.load() });
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
