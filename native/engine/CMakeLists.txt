cmake_minimum_required(VERSION 3.22)

project(MixDeckEngine VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(JUCE)

# SoundTouch (Story 3.1, ADR-006) — vendorisé en submodule comme JUCE, pinné
# au tag 2.4.1. On ne veut que la lib SoundTouch elle-même : pas l'utilitaire
# CLI soundstretch, pas le wrapper C SoundTouchDLL. INTEGER_SAMPLES=OFF est
# déjà la valeur par défaut de SoundTouch (échantillons float, cohérent avec
# les buffers JUCE) ; forcé ici pour ne pas dépendre d'un futur changement de
# défaut en amont.
set(SOUNDSTRETCH OFF CACHE BOOL "" FORCE)
set(SOUNDTOUCH_DLL OFF CACHE BOOL "" FORCE)
set(INTEGER_SAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(SoundTouch)

# Shared engine code (Deck/Mixer/FilterDSP/Engine/TimeStretch/PluginHost).
# Engine.cpp (2.2) est l'équivalent headless du graphe audio de
# MainComponent, utilisé par le Node Bridge plus bas.
add_library(engine-core STATIC
    src/Deck.cpp
    src/Mixer.cpp
    src/FilterDSP.cpp
    src/Engine.cpp
    src/TimeStretch.cpp
    src/BpmAnalyzer.cpp
    src/PluginWindowSpike.cpp
    src/PluginHost.cpp
)

target_include_directories(engine-core PUBLIC src)

target_link_libraries(engine-core
    PUBLIC
        juce::juce_audio_utils
        juce::juce_dsp
        SoundTouch
    PRIVATE
        juce::juce_recommended_config_flags
)

target_compile_definitions(engine-core
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_STRICT_REFCOUNTEDPOINTER=1
        # Story 4.1 (ADR-003) — active le hosting VST3/AU dans
        # juce_audio_processors (module déjà lié transitivement via
        # juce_audio_utils) ; sans ces flags, KnownPluginList/PluginDirectoryScanner
        # ne trouveraient silencieusement aucun plugin.
        JUCE_PLUGINHOST_VST3=1
        JUCE_PLUGINHOST_AU=1
)

# Story 1.1 test harness: standalone JUCE GUI app, no Electron/N-API involved.
# This validates the DSP/playback in isolation before the Epic 2 bridge.
juce_add_gui_app(MixDeckStandalone
    PRODUCT_NAME "MixDeck Standalone"
    COMPANY_NAME "MixDeck"
)

target_sources(MixDeckStandalone PRIVATE
    standalone-app/Main.cpp
    standalone-app/MainComponent.cpp
    standalone-app/DeckPanel.cpp
)

target_link_libraries(MixDeckStandalone
    PRIVATE
        engine-core
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

# Story 2.2 — Node addon (Bridge, ADR-002/ADR-014). Only built when configured
# via cmake-js (`npm run build` in this directory); the plain standalone build
# above (`cmake -S native/engine -B native/engine/build -G Ninja`) is
# unaffected and needs nothing Node-related installed.
if(CMAKE_JS_VERSION)
    execute_process(
        COMMAND node -p "require('node-addon-api').include"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE NODE_ADDON_API_DIR
    )
    string(REPLACE "\n" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})
    string(REPLACE "\"" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})

    add_library(mixdeck_bridge SHARED src/NodeBinding.cpp ${CMAKE_JS_SRC})

    target_include_directories(mixdeck_bridge PRIVATE ${CMAKE_JS_INC} ${NODE_ADDON_API_DIR})
    target_link_libraries(mixdeck_bridge PRIVATE engine-core ${CMAKE_JS_LIB})
    target_compile_definitions(mixdeck_bridge PRIVATE NAPI_DISABLE_CPP_EXCEPTIONS)

    set_target_properties(mixdeck_bridge PROPERTIES
        PREFIX ""
        SUFFIX ".node"
    )
endif()
