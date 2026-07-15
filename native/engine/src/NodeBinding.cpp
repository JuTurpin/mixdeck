#include <napi.h>
#include "Engine.h"

namespace {

// N-API entry point (ADR-002/ADR-014): pure translation JS <-> C++, no
// business logic — each method is a direct call into Engine/Deck/Mixer.
class NativeEngine : public Napi::ObjectWrap<NativeEngine> {
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports) {
        Napi::Function func = DefineClass(env, "NativeEngine", {
            InstanceMethod("deckLoadTrack", &NativeEngine::DeckLoadTrack),
            InstanceMethod("deckUnloadTrack", &NativeEngine::DeckUnloadTrack),
            InstanceMethod("deckPlay", &NativeEngine::DeckPlay),
            InstanceMethod("deckPause", &NativeEngine::DeckPause),
            InstanceMethod("deckStop", &NativeEngine::DeckStop),
            InstanceMethod("deckSeek", &NativeEngine::DeckSeek),
            InstanceMethod("deckGetState", &NativeEngine::DeckGetState),
            InstanceMethod("deckGetPosition", &NativeEngine::DeckGetPosition),
            InstanceMethod("deckGetLength", &NativeEngine::DeckGetLength),
            InstanceMethod("deckGetBpm", &NativeEngine::DeckGetBpm),
            InstanceMethod("deckSetFilter", &NativeEngine::DeckSetFilter),
            InstanceMethod("deckSetPitch", &NativeEngine::DeckSetPitch),
            InstanceMethod("deckSetPitchMode", &NativeEngine::DeckSetPitchMode),
            InstanceMethod("mixerSetDeckVolume", &NativeEngine::MixerSetDeckVolume),
            InstanceMethod("mixerSetCrossfaderPosition", &NativeEngine::MixerSetCrossfaderPosition),
            InstanceMethod("mixerSetCrossfaderCurve", &NativeEngine::MixerSetCrossfaderCurve),
            InstanceMethod("setHostWindowHandle", &NativeEngine::SetHostWindowHandle),
            InstanceMethod("startPluginScan", &NativeEngine::StartPluginScan),
            InstanceMethod("isPluginScanInProgress", &NativeEngine::IsPluginScanInProgress),
            InstanceMethod("getAvailablePlugins", &NativeEngine::GetAvailablePlugins),
            InstanceMethod("addPluginFromPath", &NativeEngine::AddPluginFromPath),
            // Story 4.3 — per-deck effects chain.
            InstanceMethod("deckAddPlugin", &NativeEngine::DeckAddPlugin),
            InstanceMethod("deckIsAddingPlugin", &NativeEngine::DeckIsAddingPlugin),
            InstanceMethod("deckGetLastPluginAddError", &NativeEngine::DeckGetLastPluginAddError),
            InstanceMethod("deckRemovePlugin", &NativeEngine::DeckRemovePlugin),
            InstanceMethod("deckMovePlugin", &NativeEngine::DeckMovePlugin),
            InstanceMethod("deckSetPluginBypassed", &NativeEngine::DeckSetPluginBypassed),
            InstanceMethod("deckShowPluginEditor", &NativeEngine::DeckShowPluginEditor),
            InstanceMethod("deckHidePluginEditor", &NativeEngine::DeckHidePluginEditor),
            InstanceMethod("deckGetPluginChain", &NativeEngine::DeckGetPluginChain),
            // Story 4.3 — bus master effects chain (same shape, no deck index).
            InstanceMethod("masterAddPlugin", &NativeEngine::MasterAddPlugin),
            InstanceMethod("masterIsAddingPlugin", &NativeEngine::MasterIsAddingPlugin),
            InstanceMethod("masterGetLastPluginAddError", &NativeEngine::MasterGetLastPluginAddError),
            InstanceMethod("masterRemovePlugin", &NativeEngine::MasterRemovePlugin),
            InstanceMethod("masterMovePlugin", &NativeEngine::MasterMovePlugin),
            InstanceMethod("masterSetPluginBypassed", &NativeEngine::MasterSetPluginBypassed),
            InstanceMethod("masterShowPluginEditor", &NativeEngine::MasterShowPluginEditor),
            InstanceMethod("masterHidePluginEditor", &NativeEngine::MasterHidePluginEditor),
            InstanceMethod("masterGetPluginChain", &NativeEngine::MasterGetPluginChain),
        });

        exports.Set("NativeEngine", func);
        return exports;
    }

    explicit NativeEngine(const Napi::CallbackInfo& info) : Napi::ObjectWrap<NativeEngine>(info) {}

private:
    // Story 4.0 — lets JUCE GUI classes (now real plugin editors, Story 4.3)
    // live inside Electron's process without a full JUCEApplication.
    // Declared BEFORE engine — C++ destroys members in reverse declaration
    // order, so this must outlive engine (whose Decks/MasterBus can hold
    // open plugin editors, real juce::Component-derived GUI). Getting this
    // backwards (engine declared first) was harmless in Story 4.0 (the dummy
    // PluginWindowSpike had no real teardown dependency on it) but would be a
    // real use-after-teardown risk now that engine can own live editors.
    juce::ScopedJuceInitialiser_GUI juceGuiInit;
    mixdeck::Engine engine;
    void* hostWindowHandle = nullptr; // NSView* (macOS) — Electron owns the real view

    mixdeck::Deck& DeckFromArg(const Napi::CallbackInfo& info, size_t argIndex) {
        const auto index = info[argIndex].As<Napi::Number>().Int32Value();
        return engine.getDeck(index);
    }

    Napi::Value DeckLoadTrack(const Napi::CallbackInfo& info) {
        auto& deck = DeckFromArg(info, 0);
        const auto path = info[1].As<Napi::String>().Utf8Value();
        const auto error = deck.loadFile(juce::File(path));
        return Napi::String::New(info.Env(), error.toStdString());
    }

    Napi::Value DeckUnloadTrack(const Napi::CallbackInfo& info) {
        DeckFromArg(info, 0).unloadTrack();
        return info.Env().Undefined();
    }

    Napi::Value DeckPlay(const Napi::CallbackInfo& info) {
        DeckFromArg(info, 0).play();
        return info.Env().Undefined();
    }

    Napi::Value DeckPause(const Napi::CallbackInfo& info) {
        DeckFromArg(info, 0).pause();
        return info.Env().Undefined();
    }

    Napi::Value DeckStop(const Napi::CallbackInfo& info) {
        DeckFromArg(info, 0).stop();
        return info.Env().Undefined();
    }

    Napi::Value DeckSeek(const Napi::CallbackInfo& info) {
        DeckFromArg(info, 0).seek(info[1].As<Napi::Number>().DoubleValue());
        return info.Env().Undefined();
    }

    Napi::Value DeckGetState(const Napi::CallbackInfo& info) {
        const auto state = DeckFromArg(info, 0).getState();
        return Napi::String::New(info.Env(), mixdeck::toString(state).toStdString());
    }

    Napi::Value DeckGetPosition(const Napi::CallbackInfo& info) {
        return Napi::Number::New(info.Env(), DeckFromArg(info, 0).getPositionSeconds());
    }

    Napi::Value DeckGetLength(const Napi::CallbackInfo& info) {
        return Napi::Number::New(info.Env(), DeckFromArg(info, 0).getLengthSeconds());
    }

    Napi::Value DeckGetBpm(const Napi::CallbackInfo& info) {
        return Napi::Number::New(info.Env(), DeckFromArg(info, 0).getBpm());
    }

    Napi::Value DeckSetFilter(const Napi::CallbackInfo& info) {
        DeckFromArg(info, 0).setFilterKnob(static_cast<float>(info[1].As<Napi::Number>().DoubleValue()));
        return info.Env().Undefined();
    }

    Napi::Value DeckSetPitch(const Napi::CallbackInfo& info) {
        DeckFromArg(info, 0).setPitch(static_cast<float>(info[1].As<Napi::Number>().DoubleValue()));
        return info.Env().Undefined();
    }

    Napi::Value DeckSetPitchMode(const Napi::CallbackInfo& info) {
        const auto name = info[1].As<Napi::String>().Utf8Value();
        auto mode = mixdeck::PitchMode::LinkedSpeed;
        if (name == "independent")
            mode = mixdeck::PitchMode::Independent;
        DeckFromArg(info, 0).setPitchMode(mode);
        return info.Env().Undefined();
    }

    Napi::Value MixerSetDeckVolume(const Napi::CallbackInfo& info) {
        const auto index = info[0].As<Napi::Number>().Int32Value();
        const auto volume = static_cast<float>(info[1].As<Napi::Number>().DoubleValue());
        engine.getMixer().setDeckVolume(index, volume);
        return info.Env().Undefined();
    }

    Napi::Value MixerSetCrossfaderPosition(const Napi::CallbackInfo& info) {
        engine.getMixer().setCrossfaderPosition(static_cast<float>(info[0].As<Napi::Number>().DoubleValue()));
        return info.Env().Undefined();
    }

    Napi::Value MixerSetCrossfaderCurve(const Napi::CallbackInfo& info) {
        const auto name = info[0].As<Napi::String>().Utf8Value();
        auto curve = mixdeck::CrossfaderCurve::Smooth;
        if (name == "linear")
            curve = mixdeck::CrossfaderCurve::Linear;
        else if (name == "cut")
            curve = mixdeck::CrossfaderCurve::Cut;
        engine.getMixer().setCrossfaderCurve(curve);
        return info.Env().Undefined();
    }

    // Story 4.0 — info[0] is the Buffer returned by Electron's
    // BrowserWindow.getNativeWindowHandle(): its BYTES are the native handle
    // (NSView* on macOS), not its .Data() pointer itself — the buffer must be
    // dereferenced as a pointer, a detail that has caused real crashes for
    // others attempting this (see Story 4.0 plan notes).
    Napi::Value SetHostWindowHandle(const Napi::CallbackInfo& info) {
        const auto buffer = info[0].As<Napi::Buffer<uint8_t>>();
        if (buffer.ByteLength() >= sizeof(void*))
            hostWindowHandle = *reinterpret_cast<void* const*>(buffer.Data());
        return info.Env().Undefined();
    }

    Napi::Value StartPluginScan(const Napi::CallbackInfo& info) {
        engine.startPluginScan();
        return info.Env().Undefined();
    }

    Napi::Value IsPluginScanInProgress(const Napi::CallbackInfo& info) {
        return Napi::Boolean::New(info.Env(), engine.isPluginScanInProgress());
    }

    Napi::Value GetAvailablePlugins(const Napi::CallbackInfo& info) {
        const auto plugins = engine.getAvailablePlugins();
        auto result = Napi::Array::New(info.Env(), plugins.size());
        for (size_t i = 0; i < plugins.size(); ++i) {
            auto entry = Napi::Object::New(info.Env());
            entry.Set("name", plugins[i].name.toStdString());
            entry.Set("manufacturerName", plugins[i].manufacturerName.toStdString());
            entry.Set("pluginFormatName", plugins[i].pluginFormatName.toStdString());
            entry.Set("fileOrIdentifier", plugins[i].fileOrIdentifier.toStdString());
            entry.Set("identifierString", plugins[i].createIdentifierString().toStdString());
            result[i] = entry;
        }
        return result;
    }

    Napi::Value AddPluginFromPath(const Napi::CallbackInfo& info) {
        const auto path = info[0].As<Napi::String>().Utf8Value();
        const auto error = engine.addPluginFromPath(juce::String(path));
        return Napi::String::New(info.Env(), error.toStdString());
    }

    // Story 4.3 — converts a PluginChain snapshot into { name, bypassed }[],
    // reused for both deck and master chains.
    static Napi::Value SlotsToArray(Napi::Env env, const std::vector<mixdeck::PluginChain::SlotInfo>& slots) {
        auto result = Napi::Array::New(env, slots.size());
        for (size_t i = 0; i < slots.size(); ++i) {
            auto entry = Napi::Object::New(env);
            entry.Set("name", slots[i].name.toStdString());
            entry.Set("bypassed", slots[i].bypassed);
            result[i] = entry;
        }
        return result;
    }

    // Story 4.3 — per-deck effects chain (deckIndex is info[0], plugin args follow).
    Napi::Value DeckAddPlugin(const Napi::CallbackInfo& info) {
        const auto identifier = info[1].As<Napi::String>().Utf8Value();
        DeckFromArg(info, 0).addPlugin(juce::String(identifier));
        return info.Env().Undefined();
    }

    Napi::Value DeckIsAddingPlugin(const Napi::CallbackInfo& info) {
        return Napi::Boolean::New(info.Env(), DeckFromArg(info, 0).isAddingPlugin());
    }

    Napi::Value DeckGetLastPluginAddError(const Napi::CallbackInfo& info) {
        return Napi::String::New(info.Env(), DeckFromArg(info, 0).getLastPluginAddError().toStdString());
    }

    Napi::Value DeckRemovePlugin(const Napi::CallbackInfo& info) {
        DeckFromArg(info, 0).removePlugin(info[1].As<Napi::Number>().Int32Value());
        return info.Env().Undefined();
    }

    Napi::Value DeckMovePlugin(const Napi::CallbackInfo& info) {
        DeckFromArg(info, 0).movePlugin(info[1].As<Napi::Number>().Int32Value(), info[2].As<Napi::Number>().Int32Value());
        return info.Env().Undefined();
    }

    Napi::Value DeckSetPluginBypassed(const Napi::CallbackInfo& info) {
        DeckFromArg(info, 0).setPluginBypassed(info[1].As<Napi::Number>().Int32Value(), info[2].As<Napi::Boolean>().Value());
        return info.Env().Undefined();
    }

    Napi::Value DeckShowPluginEditor(const Napi::CallbackInfo& info) {
        DeckFromArg(info, 0).showPluginEditor(info[1].As<Napi::Number>().Int32Value(), hostWindowHandle);
        return info.Env().Undefined();
    }

    Napi::Value DeckHidePluginEditor(const Napi::CallbackInfo& info) {
        DeckFromArg(info, 0).hidePluginEditor(info[1].As<Napi::Number>().Int32Value());
        return info.Env().Undefined();
    }

    Napi::Value DeckGetPluginChain(const Napi::CallbackInfo& info) {
        return SlotsToArray(info.Env(), DeckFromArg(info, 0).getPluginChainSlots());
    }

    // Story 4.3 — bus master effects chain (same shape, no deck index).
    Napi::Value MasterAddPlugin(const Napi::CallbackInfo& info) {
        const auto identifier = info[0].As<Napi::String>().Utf8Value();
        engine.getMasterBus().getPluginChain().addPlugin(juce::String(identifier));
        return info.Env().Undefined();
    }

    Napi::Value MasterIsAddingPlugin(const Napi::CallbackInfo& info) {
        return Napi::Boolean::New(info.Env(), engine.getMasterBus().getPluginChain().isAddingPlugin());
    }

    Napi::Value MasterGetLastPluginAddError(const Napi::CallbackInfo& info) {
        return Napi::String::New(info.Env(), engine.getMasterBus().getPluginChain().getLastAddError().toStdString());
    }

    Napi::Value MasterRemovePlugin(const Napi::CallbackInfo& info) {
        engine.getMasterBus().getPluginChain().removePlugin(info[0].As<Napi::Number>().Int32Value());
        return info.Env().Undefined();
    }

    Napi::Value MasterMovePlugin(const Napi::CallbackInfo& info) {
        engine.getMasterBus().getPluginChain().movePlugin(info[0].As<Napi::Number>().Int32Value(), info[1].As<Napi::Number>().Int32Value());
        return info.Env().Undefined();
    }

    Napi::Value MasterSetPluginBypassed(const Napi::CallbackInfo& info) {
        engine.getMasterBus().getPluginChain().setPluginBypassed(info[0].As<Napi::Number>().Int32Value(), info[1].As<Napi::Boolean>().Value());
        return info.Env().Undefined();
    }

    Napi::Value MasterShowPluginEditor(const Napi::CallbackInfo& info) {
        engine.getMasterBus().getPluginChain().showPluginEditor(info[0].As<Napi::Number>().Int32Value(), hostWindowHandle);
        return info.Env().Undefined();
    }

    Napi::Value MasterHidePluginEditor(const Napi::CallbackInfo& info) {
        engine.getMasterBus().getPluginChain().hidePluginEditor(info[0].As<Napi::Number>().Int32Value());
        return info.Env().Undefined();
    }

    Napi::Value MasterGetPluginChain(const Napi::CallbackInfo& info) {
        return SlotsToArray(info.Env(), engine.getMasterBus().getPluginChain().getSlots());
    }
};

Napi::Object InitAll(Napi::Env env, Napi::Object exports) {
    return NativeEngine::Init(env, exports);
}

} // namespace

NODE_API_MODULE(mixdeck_bridge, InitAll)
