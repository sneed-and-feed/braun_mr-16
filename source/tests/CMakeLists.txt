# ==============================================================================
# BRAUN MR-16 · Headless C++ DSP Verification Test Suite
# ==============================================================================

cmake_minimum_required(VERSION 3.22)
project(mr16_headless_tests LANGUAGES CXX)

add_executable(mr16_headless_dsp_tests
    mr16_headless_dsp_tests.cpp
)

target_include_directories(mr16_headless_dsp_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../dsp
    ${CMAKE_CURRENT_SOURCE_DIR}
)

if(TARGET mr16_dsp_core)
    target_link_libraries(mr16_headless_dsp_tests PRIVATE
        mr16_dsp_core
    )
else()
    target_sources(mr16_headless_dsp_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../dsp/BoundedSaturator.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../dsp/VactrolGate.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../dsp/SpatialChorus.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../dsp/LorenzAttractor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../dsp/ModalResonatorMatrix.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../dsp/KineticExciter.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../dsp/Mr16Engine.cpp
    )
endif()

set_target_properties(mr16_headless_dsp_tests PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

if(MSVC)
    target_compile_options(mr16_headless_dsp_tests PRIVATE /W4 /permissive- /utf-8 /fp:fast)
else()
    target_compile_options(mr16_headless_dsp_tests PRIVATE -Wall -Wextra -pedantic -ffast-math)
endif()

add_test(NAME Mr16HeadlessDspTests COMMAND mr16_headless_dsp_tests)
