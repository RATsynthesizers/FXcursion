# FXcursion FX Bench - a VST3 for auditioning the pedal's effects
#
# The point of this project is that it compiles THE FIRMWARE'S OWN EFFECT CODE.
# Not a port, not a reimplementation: the same .c files that go on the STM32,
# with the same fixed 48 kHz block maths. What you hear in FL Studio is what the
# hardware will do.
#
# Only the effect layer is built. No grid, no mixer, no looper, no protocol -
# the effects depend on nothing but fx_common and their own state, which is
# exactly what makes this possible.
#
#   cmake -S . -B build -G "Visual Studio 17 2022" -A x64
#   cmake --build build --config Release
#
# or just run build.bat.

cmake_minimum_required(VERSION 3.22)

project(VST_test_RAT VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)

# The firmware tree. Everything audio comes from there, nothing is copied.
set(FW "${CMAKE_CURRENT_SOURCE_DIR}/../FXcursion_Audio_SHIELD")

if(NOT EXISTS "${FW}/Shared/fx_defs.c")
    message(FATAL_ERROR "Cannot find the audio firmware at ${FW}")
endif()

add_subdirectory(_deps/JUCE)

juce_add_plugin(VST_test_RAT
    COMPANY_NAME        "RAT Synthesizers"
    PLUGIN_MANUFACTURER_CODE Rats
    PLUGIN_CODE         Fxb1
    BUNDLE_ID           "com.ratsynthesizers.fxbench"
    FORMATS             VST3
    PRODUCT_NAME        "FXcursion FX Bench"
    IS_SYNTH            FALSE
    NEEDS_MIDI_INPUT    FALSE
    NEEDS_MIDI_OUTPUT   FALSE
    IS_MIDI_EFFECT      FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(VST_test_RAT PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/FxHost.cpp
    Source/FxUnits.cpp

    # ---- the firmware, verbatim ----------------------------------------------
    ${FW}/Shared/fx_defs.c
    ${FW}/ApplicationSW/AudioEngine/Effects/fx_common.c
    ${FW}/ApplicationSW/AudioEngine/Effects/fx_registry.c
    ${FW}/ApplicationSW/AudioEngine/Effects/fx_amp.c
    ${FW}/ApplicationSW/AudioEngine/Effects/fx_compressor.c
    ${FW}/ApplicationSW/AudioEngine/Effects/fx_delay.c
    ${FW}/ApplicationSW/AudioEngine/Effects/fx_reverb.c
    ${FW}/ApplicationSW/AudioEngine/Effects/fx_overdrive.c
    ${FW}/ApplicationSW/AudioEngine/Effects/fx_tremolo.c
    ${FW}/ApplicationSW/AudioEngine/Effects/fx_distortion.c
    ${FW}/ApplicationSW/AudioEngine/Effects/fx_modulation.c
    ${FW}/ApplicationSW/AudioEngine/Effects/fx_phaser.c)

target_include_directories(VST_test_RAT PRIVATE
    ${FW}/Config
    ${FW}/Shared
    ${FW}/ApplicationSW/AudioEngine
    ${FW}/Test/host)               # stand-in for SystemSW/Include, as the unit tests use

target_compile_definitions(VST_test_RAT PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    # Strips the linker-section attributes in mem_map.h. Nothing else in the
    # effect code changes between target and host.
    FXC_HOST_BUILD=1)

# The firmware is MISRA-ish C compiled by GCC. MSVC objects to a few of its
# habits that are not actually problems here.
if(MSVC)
    target_compile_options(VST_test_RAT PRIVATE
        /wd4244     # conversion, possible loss of data - deliberate in the DSP
        /wd4305     # truncation from double to float - ditto
        /wd4310)    # FXC_STATIC_ASSERT's negative-array trick
endif()

target_link_libraries(VST_test_RAT
    PRIVATE juce::juce_audio_utils
    PUBLIC  juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags
            juce::juce_recommended_warning_flags)


# ==============================================================================
# Load test - a console host that opens the built .vst3 the way a DAW would.
#
# Compiling and loading are different claims. This target makes the second one
# checkable without opening FL Studio: it instantiates the plugin, selects each
# effect in turn and pushes noise through it, failing on silence or NaN.
# ==============================================================================

juce_add_console_app(FxBenchLoadTest PRODUCT_NAME "FxBenchLoadTest")

target_sources(FxBenchLoadTest PRIVATE
    Source/LoadTest.cpp
    Source/FxUnits.cpp
    # The firmware's own mapping functions, so the readout can be compared
    # against what the DSP computes rather than trusted.
    ${FW}/Shared/fx_defs.c
    ${FW}/ApplicationSW/AudioEngine/Effects/fx_common.c)

target_include_directories(FxBenchLoadTest PRIVATE
    ${FW}/Config
    ${FW}/Shared
    ${FW}/ApplicationSW/AudioEngine
    ${FW}/Test/host
    Source)

target_compile_definitions(FxBenchLoadTest PRIVATE
    FXC_HOST_BUILD=1
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_PLUGINHOST_VST3=1
    JUCE_MODAL_LOOPS_PERMITTED=1)

target_link_libraries(FxBenchLoadTest
    PRIVATE juce::juce_audio_processors juce::juce_audio_utils
    PUBLIC  juce::juce_recommended_config_flags)


# ==============================================================================
# UI test - builds the real editor and inspects it.
#
# The VST3 load test cannot see inside a hosted plugin's window, so a panel
# whose knobs are all disabled passes it perfectly. This target links the plugin
# sources directly and asks the controls what state they are in.
# ==============================================================================

juce_add_console_app(FxBenchUiTest PRODUCT_NAME "FxBenchUiTest")

target_sources(FxBenchUiTest PRIVATE
    Source/UiTest.cpp
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/FxHost.cpp
    Source/FxUnits.cpp

    ${FW}/Shared/fx_defs.c
    ${FW}/ApplicationSW/AudioEngine/Effects/fx_common.c
    ${FW}/ApplicationSW/AudioEngine/Effects/fx_registry.c
    ${FW}/ApplicationSW/AudioEngine/Effects/fx_amp.c
    ${FW}/ApplicationSW/AudioEngine/Effects/fx_compressor.c
    ${FW}/ApplicationSW/AudioEngine/Effects/fx_delay.c
    ${FW}/ApplicationSW/AudioEngine/Effects/fx_reverb.c
    ${FW}/ApplicationSW/AudioEngine/Effects/fx_overdrive.c
    ${FW}/ApplicationSW/AudioEngine/Effects/fx_tremolo.c
    ${FW}/ApplicationSW/AudioEngine/Effects/fx_distortion.c
    ${FW}/ApplicationSW/AudioEngine/Effects/fx_modulation.c
    ${FW}/ApplicationSW/AudioEngine/Effects/fx_phaser.c)

target_include_directories(FxBenchUiTest PRIVATE
    ${FW}/Config
    ${FW}/Shared
    ${FW}/ApplicationSW/AudioEngine
    ${FW}/Test/host
    Source)

target_compile_definitions(FxBenchUiTest PRIVATE
    FXC_HOST_BUILD=1
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_MODAL_LOOPS_PERMITTED=1)

if(MSVC)
    target_compile_options(FxBenchUiTest PRIVATE /wd4244 /wd4305 /wd4310)
endif()

target_link_libraries(FxBenchUiTest
    PRIVATE juce::juce_audio_utils
    PUBLIC  juce::juce_recommended_config_flags)
