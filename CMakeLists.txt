cmake_minimum_required(VERSION 3.22)
project(BassCode VERSION 1.0.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(APPLE)
 set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum supported macOS version")
endif()
if(MSVC)
 set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")  # static CRT
 add_compile_options(/utf-8)
endif()

find_package(JUCE CONFIG QUIET)
if(NOT JUCE_FOUND)
 include(FetchContent)
 FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.12)
 FetchContent_MakeAvailable(JUCE)
endif()

juce_add_binary_data(bass_assets SOURCES
 Assets/Fonts/SpaceGrotesk-Bold.ttf Assets/Fonts/Inter-Bold.ttf Assets/Fonts/Inter-Regular.ttf)
set_target_properties(bass_assets PROPERTIES POSITION_INDEPENDENT_CODE ON)

set(BASS_SOURCES
 Source/Core/Music.cpp Source/Core/Music.h
 Source/Dsp/Synth.cpp Source/Dsp/Synth.h
 Source/Dsp/Fx.cpp Source/Dsp/Fx.h
 Source/Processor.cpp Source/Processor.h
 Source/Editor.cpp Source/Editor.h)

set(BASS_DEFINITIONS JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_USE_DIRECT2D=0 JUCE_VST3_CAN_REPLACE_VST2=0
 JUCE_DISPLAY_SPLASH_SCREEN=0)

set(formats VST3 Standalone)
if(APPLE)
 list(APPEND formats AU)
endif()

juce_add_plugin(BassCode
 COMPANY_NAME "Audio Makers"
 BUNDLE_ID "com.audiomakers.basscode"
 PLUGIN_MANUFACTURER_CODE AuMk
 PLUGIN_CODE BsCd
 ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/Assets/icon-1024.png"
 FORMATS ${formats}
 PRODUCT_NAME "Bass Code"
 VST3_CATEGORIES Instrument Synth
 AU_MAIN_TYPE kAudioUnitType_MusicDevice
 IS_SYNTH TRUE
 NEEDS_MIDI_INPUT TRUE
 NEEDS_MIDI_OUTPUT FALSE
 VST3_AUTO_MANIFEST FALSE
 COPY_PLUGIN_AFTER_BUILD FALSE)
target_sources(BassCode PRIVATE ${BASS_SOURCES})
target_include_directories(BassCode PRIVATE Source)
target_compile_definitions(BassCode PUBLIC ${BASS_DEFINITIONS})
target_link_libraries(BassCode PRIVATE bass_assets juce::juce_audio_utils juce::juce_audio_processors juce::juce_dsp
 PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)

# Tests: generator + audio checks for every style/sound (BASS_SNAPSHOT=<png> writes an editor image,
# BASS_DEMO_DIR=<dir> writes one WAV per style)
juce_add_console_app(bass_tests PRODUCT_NAME "Bass Code Tests")
target_sources(bass_tests PRIVATE Tests/Tests.cpp ${BASS_SOURCES})
target_include_directories(bass_tests PRIVATE Source)
target_compile_definitions(bass_tests PRIVATE ${BASS_DEFINITIONS} JucePlugin_VersionString="1.0.0"
 JUCE_STANDALONE_APPLICATION=1)
target_link_libraries(bass_tests PRIVATE bass_assets juce::juce_audio_utils juce::juce_audio_processors juce::juce_dsp
 PUBLIC juce::juce_recommended_config_flags)

enable_testing()
add_test(NAME bass_tests COMMAND bass_tests)
