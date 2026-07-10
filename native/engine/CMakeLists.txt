cmake_minimum_required(VERSION 3.22)

project(MixDeckEngine VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(JUCE)

# Shared engine code (Deck.cpp here; Mixer/FilterDSP/TimeStretch/PluginHost will
# be added to this same target in later stories, not created as empty stubs now).
add_library(engine-core STATIC
    src/Deck.cpp
    src/Mixer.cpp
)

target_include_directories(engine-core PUBLIC src)

target_link_libraries(engine-core
    PUBLIC
        juce::juce_audio_utils
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
