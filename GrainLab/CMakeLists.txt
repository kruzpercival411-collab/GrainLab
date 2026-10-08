cmake_minimum_required(VERSION 3.22)

# ---------------------------------------------------------------------------
#  GRAIN LAB - granular sampler VST3 (JUCE 8)
#  Rename the plugin by changing PLUGIN_NAME below (or -DPLUGIN_NAME="...").
# ---------------------------------------------------------------------------
set(PLUGIN_NAME "Grain Lab" CACHE STRING "Plugin display name shown in the DAW")

project(GrainLab VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Static MSVC runtime: the .vst3 works on any Windows PC without the VC++ redistributable.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# --- JUCE ------------------------------------------------------------------
# By default JUCE 8.0.4 is downloaded automatically. To use a local copy:
#   cmake -B build -DGRAINLAB_JUCE_PATH=C:/path/to/JUCE
set(GRAINLAB_JUCE_PATH "" CACHE PATH "Optional path to a local JUCE checkout")
if(GRAINLAB_JUCE_PATH)
    add_subdirectory(${GRAINLAB_JUCE_PATH} JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.4
        GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

option(GRAINLAB_COPY_AFTER_BUILD "Copy the VST3 into the system VST3 folder after building (needs admin on Windows)" OFF)
set(GRAINLAB_FORMATS "VST3;Standalone" CACHE STRING "Plugin formats to build")
option(GRAINLAB_NO_ALSA "Build without ALSA/JACK (Linux compile checks only)" OFF)

juce_add_plugin(GrainLab
    COMPANY_NAME             "GrainLab Audio"
    BUNDLE_ID                com.grainlab.grainlab
    PLUGIN_MANUFACTURER_CODE Grlb
    PLUGIN_CODE              Grl1
    PRODUCT_NAME             "${PLUGIN_NAME}"
    IS_SYNTH                 TRUE
    NEEDS_MIDI_INPUT         TRUE
    NEEDS_MIDI_OUTPUT        FALSE
    IS_MIDI_EFFECT           FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD  ${GRAINLAB_COPY_AFTER_BUILD}
    VST3_CATEGORIES          Instrument Sampler
    FORMATS                  ${GRAINLAB_FORMATS})

target_sources(GrainLab PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/GrainEngine.cpp
    Source/UI/Theme.cpp
    Source/UI/Widgets.cpp
    Source/UI/Displays.cpp)

target_include_directories(GrainLab PRIVATE Source)

target_compile_definitions(GrainLab PUBLIC
    GRAINLAB_PLUGIN_NAME="${PLUGIN_NAME}"
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_USE_MP3AUDIOFORMAT=1
    JUCE_USE_FLAC=1
    JUCE_USE_OGGVORBIS=1)

if(GRAINLAB_NO_ALSA)
    target_compile_definitions(GrainLab PUBLIC JUCE_ALSA=0 JUCE_JACK=0 JUCE_MODAL_LOOPS_PERMITTED=1)
endif()

target_link_libraries(GrainLab
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_formats
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# --- optional offline test harness (renders audio + UI snapshot without a DAW) ---
option(GRAINLAB_BUILD_TESTS "Build the offline test harness" OFF)
if(GRAINLAB_BUILD_TESTS)
    juce_add_console_app(GrainLabTests PRODUCT_NAME "GrainLabTests")
    target_sources(GrainLabTests PRIVATE
        Tests/TestHarness.cpp
        Source/PluginProcessor.cpp Source/PluginEditor.cpp Source/GrainEngine.cpp
        Source/UI/Theme.cpp Source/UI/Widgets.cpp Source/UI/Displays.cpp)
    target_include_directories(GrainLabTests PRIVATE Source)
    target_compile_definitions(GrainLabTests PRIVATE
        GRAINLAB_PLUGIN_NAME="${PLUGIN_NAME}" JucePlugin_VersionString="${PROJECT_VERSION}"
        JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_USE_MP3AUDIOFORMAT=1 JUCE_ALSA=0 JUCE_JACK=0 JUCE_MODAL_LOOPS_PERMITTED=1)
    target_link_libraries(GrainLabTests PRIVATE
        juce::juce_audio_processors juce::juce_audio_formats juce::juce_gui_extra juce::juce_dsp
        juce::juce_recommended_config_flags)
endif()
