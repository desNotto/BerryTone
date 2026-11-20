cmake_minimum_required(VERSION 3.22)
# The version lives here. Release builds can override it: -DBERRY_VERSION=x.y.z
if(NOT DEFINED BERRY_VERSION)
    set(BERRY_VERSION 1.1.2)
endif()
project(BerryTone VERSION ${BERRY_VERSION})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.6
    GIT_SHALLOW    ON)
FetchContent_MakeAvailable(JUCE)

set(BERRY_FORMATS VST3)   # no Standalone build
if(APPLE)
    list(APPEND BERRY_FORMATS AU)
endif()

juce_add_plugin(BerryTone
    COMPANY_NAME "desNotto"
    PRODUCT_NAME "BerryTone"
    BUNDLE_ID com.desnotto.berrytone
    FORMATS ${BERRY_FORMATS}
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    PLUGIN_MANUFACTURER_CODE Bery
    PLUGIN_CODE BrTn
    VST3_CATEGORIES Fx Distortion
    AU_MAIN_TYPE kAudioUnitType_Effect
    COPY_PLUGIN_AFTER_BUILD FALSE)

juce_generate_juce_header(BerryTone)

target_sources(BerryTone PRIVATE
    Source/Dsp.h
    Source/PluginProcessor.h
    Source/PluginProcessor.cpp
    Source/PluginEditor.h
    Source/PluginEditor.cpp
    Source/Theme.h
    Source/Theme.cpp
    Source/ScopeView.h
    Source/ScopeView.cpp
    Source/PresetManager.h
    Source/PresetManager.cpp
    Source/Updater.h
    Source/Updater.cpp)

target_compile_definitions(BerryTone PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(BerryTone
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# Dev tool: renders the plugin window into PNG files (layout checks without a DAW).
# Off by default. Not part of the release.
option(BERRY_SNAPSHOT "Build the BerryToneSnapshot dev tool" OFF)
if(BERRY_SNAPSHOT)
    add_executable(BerryToneSnapshot tools/snapshot/snapshot.cpp)
    target_include_directories(BerryToneSnapshot PRIVATE Source "${JUCE_MODULES_DIR}"
        "${CMAKE_CURRENT_BINARY_DIR}/BerryTone_artefacts/JuceLibraryCode")
    target_compile_definitions(BerryToneSnapshot PRIVATE $<TARGET_PROPERTY:BerryTone,COMPILE_DEFINITIONS>)
    target_link_libraries(BerryToneSnapshot PRIVATE BerryTone)
endif()
