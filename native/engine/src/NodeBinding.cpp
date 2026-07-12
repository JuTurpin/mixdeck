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
            InstanceMethod("deckSetFilter", &NativeEngine::DeckSetFilter),
            InstanceMethod("deckSetPitch", &NativeEngine::DeckSetPitch),
            InstanceMethod("deckSetPitchMode", &NativeEngine::DeckSetPitchMode),
            InstanceMethod("mixerSetDeckVolume", &NativeEngine::MixerSetDeckVolume),
            InstanceMethod("mixerSetCrossfaderPosition", &NativeEngine::MixerSetCrossfaderPosition),
            InstanceMethod("mixerSetCrossfaderCurve", &NativeEngine::MixerSetCrossfaderCurve),
        });

        exports.Set("NativeEngine", func);
        return exports;
    }

    explicit NativeEngine(const Napi::CallbackInfo& info) : Napi::ObjectWrap<NativeEngine>(info) {}

private:
    mixdeck::Engine engine;

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
};

Napi::Object InitAll(Napi::Env env, Napi::Object exports) {
    return NativeEngine::Init(env, exports);
}

} // namespace

NODE_API_MODULE(mixdeck_bridge, InitAll)
