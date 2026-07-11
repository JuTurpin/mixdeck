cmake_minimum_required(VERSION 3.22)

project(MixDeckEngine VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(JUCE)

# Shared engine code (Deck/Mixer/FilterDSP/Engine here; TimeStretch/PluginHost
# will be added to this same target in later stories, not created as empty
# stubs now). Engine.cpp (2.2) is the headless equivalent of MainComponent's
# audio graph, used by the Node Bridge below.
add_library(engine-core STATIC
    src/Deck.cpp
    src/Mixer.cpp
    src/FilterDSP.cpp
    src/Engine.cpp
)

target_include_directories(engine-core PUBLIC src)

target_link_libraries(engine-core
    PUBLIC
        juce::juce_audio_utils
        juce::juce_dsp
    PRIVATE
        juce::juce_recommended_config_flags
)

target_compile_definitions(engine-core
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_STRICT_REFCOUNTEDPOINTER=1
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
