# FXcursion audio controller - host test build
#
# Builds everything that does not touch the HAL and runs the tests. Nothing in
# Config/, Shared/ or ApplicationSW/AudioEngine/ includes a HAL header, which is
# what makes this possible; the SystemSW drivers and ApplicationSW/AudioIO are
# the HAL-aware parts and are excluded here.
#
# loop_store.c is the one engine file that WOULD reach the HAL, through the
# PSRAM driver. Under FXC_HOST_BUILD it swaps the DMA backend for memcpy against
# a plain array, so the looper and its staging are both testable here.
#
# From ApplicationSW/AudioIO only chan_map.c, hp_bus.c and rec_stream.c are built.
# audio_io.c and rec_spi.c
# are the DMA and interrupt plumbing and are deliberately the only parts with no
# host coverage - which is exactly why the sample format conversion, the recorder
# staging state machine and the monitor sum were split out of them.
#
# ApplicationSW/CtrlLink splits the same way: ctrl_link.c is the whole protocol
# and is built here; ctrl_uart.c is the DMA transport and is not.
#
#   cmake -S Test -B Test/build -G Ninja
#   cmake --build Test/build
#   ./Test/build/fxc_tests
#
cmake_minimum_required(VERSION 3.15)
project(fxcursion_audio_tests C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)

add_executable(fxc_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/test_main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/test_memory.c
    ${CMAKE_CURRENT_SOURCE_DIR}/test_protocol.c
    ${CMAKE_CURRENT_SOURCE_DIR}/test_engine.c
    ${CMAKE_CURRENT_SOURCE_DIR}/test_effects.c
    ${CMAKE_CURRENT_SOURCE_DIR}/test_dynamics.c
    ${CMAKE_CURRENT_SOURCE_DIR}/test_revprofile.c
    ${CMAKE_CURRENT_SOURCE_DIR}/test_modulation.c
    ${CMAKE_CURRENT_SOURCE_DIR}/test_modprofile.c
    ${CMAKE_CURRENT_SOURCE_DIR}/test_loop_mem.c
    ${CMAKE_CURRENT_SOURCE_DIR}/test_loop_xfer.c
    ${CMAKE_CURRENT_SOURCE_DIR}/test_audio_io.c
    ${CMAKE_CURRENT_SOURCE_DIR}/test_ctrl_link.c
    ${CMAKE_CURRENT_SOURCE_DIR}/test_rec_stream.c
    ${CMAKE_CURRENT_SOURCE_DIR}/test_interleave.c

    ${ROOT}/Shared/fx_defs.c
    ${ROOT}/Shared/fx_crc.c
    ${ROOT}/Shared/fx_link.c
    ${ROOT}/Shared/fx_interleave.c
    ${ROOT}/Shared/fx_loop.c

    ${ROOT}/ApplicationSW/AudioEngine/params.c
    ${ROOT}/ApplicationSW/AudioEngine/grid.c
    ${ROOT}/ApplicationSW/AudioEngine/fxblock.c
    ${ROOT}/ApplicationSW/AudioEngine/mixer.c
    ${ROOT}/ApplicationSW/AudioEngine/looper.c
    ${ROOT}/ApplicationSW/AudioEngine/loop_mem.c
    ${ROOT}/ApplicationSW/AudioEngine/loop_xfer.c
    ${ROOT}/ApplicationSW/AudioEngine/recorder.c
    ${ROOT}/ApplicationSW/AudioEngine/audio_sys.c

    ${ROOT}/ApplicationSW/AudioEngine/Effects/fx_common.c
    ${ROOT}/ApplicationSW/AudioEngine/Effects/fx_registry.c
    ${ROOT}/ApplicationSW/AudioEngine/Effects/fx_amp.c
    ${ROOT}/ApplicationSW/AudioEngine/Effects/fx_compressor.c
    ${ROOT}/ApplicationSW/AudioEngine/Effects/fx_delay.c
    ${ROOT}/ApplicationSW/AudioEngine/Effects/fx_reverb.c
    ${ROOT}/ApplicationSW/AudioEngine/Effects/fx_overdrive.c
    ${ROOT}/ApplicationSW/AudioEngine/Effects/fx_tremolo.c
    ${ROOT}/ApplicationSW/AudioEngine/Effects/fx_distortion.c
    ${ROOT}/ApplicationSW/AudioEngine/Effects/fx_modulation.c
    ${ROOT}/ApplicationSW/AudioEngine/Effects/fx_phaser.c
    ${ROOT}/ApplicationSW/AudioEngine/Effects/fx_template.c

    ${ROOT}/ApplicationSW/AudioIO/chan_map.c
    ${ROOT}/ApplicationSW/AudioIO/hp_bus.c
    ${ROOT}/ApplicationSW/AudioIO/rec_stream.c

    ${ROOT}/ApplicationSW/CtrlLink/ctrl_link.c
)

target_include_directories(fxc_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/host      # stand-in for SystemSW/Include
    ${ROOT}/Config
    ${ROOT}/Shared
    ${ROOT}/ApplicationSW/AudioEngine
    ${ROOT}/ApplicationSW/AudioIO
    ${ROOT}/ApplicationSW/CtrlLink
)

# FXC_HOST_BUILD strips the section attributes in mem_map.h. Everything else
# compiles exactly as it does on target.
target_compile_definitions(fxc_tests PRIVATE FXC_HOST_BUILD=1)

if(MSVC)
    target_compile_options(fxc_tests PRIVATE /W4)
else()
    target_compile_options(fxc_tests PRIVATE
        -Wall -Wextra -Wdouble-promotion -Wfloat-conversion
        -Wshadow -Wpointer-arith -Wcast-align -Wstrict-prototypes
    )
    target_link_libraries(fxc_tests PRIVATE m)
endif()

enable_testing()
add_test(NAME fxc_tests COMMAND fxc_tests)
