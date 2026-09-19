cmake_minimum_required(VERSION 3.22)

# ==============================================================================
# BRAUN MR-16 Modaler Resonator & Kinetischer Impulssynthesizer
# JUCE 8 CMake Multi-Platform & Multi-Format Build System
# Formats: VST3, CLAP 1.0+, AU, AUv3, Standalone
# Supported Platforms: Windows (x64/ARM64), macOS (Universal), Linux, iOS, Android
# ==============================================================================

# Configure Apple deployment targets before project()
if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum macOS deployment target")
    if(NOT CMAKE_OSX_ARCHITECTURES)
        set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Universal macOS binary architectures")
    endif()
endif()

project(BRAUN_MR16_PROJECT VERSION 1.0.4 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Position-independent code for Unix shared library plugins
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ==============================================================================
# Real-Time Compiler Optimizations & Cross-Platform Warning Flags
# ==============================================================================
if(MSVC)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
    add_compile_options(/utf-8 /W4 /O2 /fp:fast)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "AMD64|x86_64|x64")
        add_compile_options(/arch:AVX2)
    endif()
else()
    add_compile_options(-Wall -Wextra -Wpedantic -O3 -ffast-math -fno-finite-math-only)
endif()

# ==============================================================================
# GUI Mode Selection Option: WebView (Default) vs Pure Native LookAndFeel
# ==============================================================================
option(MR16_USE_WEBVIEW "Build BRAUN MR-16 with WebView GUI (disable for pure native JUCE UI)" ON)
if(DEFINED MR16_ENABLE_WEBVIEW)
    set(MR16_USE_WEBVIEW ${MR16_ENABLE_WEBVIEW})
endif()
if(DEFINED USE_WEBVIEW)
    set(MR16_USE_WEBVIEW ${USE_WEBVIEW})
endif()

option(MR16_BUILD_TESTS "Build headless C++ DSP continuous verification test runner" ON)

if(MR16_USE_WEBVIEW)
    message(STATUS "BRAUN MR-16 GUI Mode: Embedded Web Browser (WebView2/WebKit)")
    add_compile_definitions(
        JUCE_WEB_BROWSER=1
        MR16_USE_WEBVIEW=1
        JUCE_USE_WIN_WEBVIEW2=1
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
    )
else()
    message(STATUS "BRAUN MR-16 GUI Mode: Pure Native JUCE (Dieter Rams LookAndFeel)")
    add_compile_definitions(
        JUCE_WEB_BROWSER=0
        MR16_USE_WEBVIEW=0
        JUCE_USE_WIN_WEBVIEW2=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
    )
endif()

include(FetchContent)

# ==============================================================================
# 1. Provision Microsoft.Web.WebView2 for Windows
# ==============================================================================
if(WIN32 AND MR16_USE_WEBVIEW)
    set(WEBVIEW2_VERSION "1.0.2849.39")
    set(LOCAL_WEBVIEW2_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../build/packages/Microsoft.Web.WebView2.${WEBVIEW2_VERSION}")
    set(LOCAL_WEBVIEW2_DIR_RB26 "${CMAKE_CURRENT_SOURCE_DIR}/../braun_rb-26/build/packages/Microsoft.Web.WebView2.${WEBVIEW2_VERSION}")
    set(LOCAL_WEBVIEW2_DIR_AS42 "${CMAKE_CURRENT_SOURCE_DIR}/../braun_as-42/build/packages/Microsoft.Web.WebView2.${WEBVIEW2_VERSION}")
    set(LOCAL_WEBVIEW2_DIR_BUILD "${CMAKE_CURRENT_SOURCE_DIR}/build/packages/Microsoft.Web.WebView2.${WEBVIEW2_VERSION}")
    set(BINARY_WEBVIEW2_DIR "${CMAKE_BINARY_DIR}/packages/Microsoft.Web.WebView2.${WEBVIEW2_VERSION}")

    if(DEFINED JUCE_WEBVIEW2_PACKAGE_LOCATION AND EXISTS "${JUCE_WEBVIEW2_PACKAGE_LOCATION}/Microsoft.Web.WebView2.${WEBVIEW2_VERSION}/build/native/include/WebView2.h")
        set(JUCE_WEBVIEW2_PACKAGE_LOCATION "${JUCE_WEBVIEW2_PACKAGE_LOCATION}" CACHE PATH "WebView2 location" FORCE)
    elseif(EXISTS "${LOCAL_WEBVIEW2_DIR_RB26}/build/native/include/WebView2.h")
        set(JUCE_WEBVIEW2_PACKAGE_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/../braun_rb-26/build/packages" CACHE PATH "WebView2 location" FORCE)
    elseif(EXISTS "${LOCAL_WEBVIEW2_DIR_AS42}/build/native/include/WebView2.h")
        set(JUCE_WEBVIEW2_PACKAGE_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/../braun_as-42/build/packages" CACHE PATH "WebView2 location" FORCE)
    elseif(EXISTS "${LOCAL_WEBVIEW2_DIR_BUILD}/build/native/include/WebView2.h")
        set(JUCE_WEBVIEW2_PACKAGE_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/build/packages" CACHE PATH "WebView2 location" FORCE)
    elseif(EXISTS "${LOCAL_WEBVIEW2_DIR}/build/native/include/WebView2.h")
        set(JUCE_WEBVIEW2_PACKAGE_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/../build/packages" CACHE PATH "WebView2 location" FORCE)
    elseif(EXISTS "${BINARY_WEBVIEW2_DIR}/build/native/include/WebView2.h")
        set(JUCE_WEBVIEW2_PACKAGE_LOCATION "${CMAKE_BINARY_DIR}/packages" CACHE PATH "WebView2 location" FORCE)
    else()
        if(NOT EXISTS "${BINARY_WEBVIEW2_DIR}/build/native/include/WebView2.h")
            message(STATUS "Downloading Microsoft.Web.WebView2 NuGet package (${WEBVIEW2_VERSION})...")
            set(WEBVIEW2_ZIP "${CMAKE_BINARY_DIR}/webview2_${WEBVIEW2_VERSION}.zip")
            file(DOWNLOAD
                "https://www.nuget.org/api/v2/package/Microsoft.Web.WebView2/${WEBVIEW2_VERSION}"
                "${WEBVIEW2_ZIP}"
                SHOW_PROGRESS
            )
            file(ARCHIVE_EXTRACT
                INPUT "${WEBVIEW2_ZIP}"
                DESTINATION "${BINARY_WEBVIEW2_DIR}"
            )
            file(REMOVE "${WEBVIEW2_ZIP}")
        endif()
        set(JUCE_WEBVIEW2_PACKAGE_LOCATION "${CMAKE_BINARY_DIR}/packages" CACHE PATH "WebView2 location" FORCE)
    endif()
    message(STATUS "JUCE_WEBVIEW2_PACKAGE_LOCATION set to: ${JUCE_WEBVIEW2_PACKAGE_LOCATION}")
endif()

# ==============================================================================
# 2. Fetch and Configure JUCE 8 (8.0.6)
# ==============================================================================
if(NOT TARGET juce::juce_audio_processors)
    set(LOCAL_JUCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../build/_deps/juce-src")
    set(LOCAL_JUCE_DIR_RB26 "${CMAKE_CURRENT_SOURCE_DIR}/../braun_rb-26/build/_deps/juce-src")
    if(EXISTS "${LOCAL_JUCE_DIR}/CMakeLists.txt")
        message(STATUS "Using local cached JUCE 8 from ${LOCAL_JUCE_DIR}")
        FetchContent_Declare(
            JUCE
            SOURCE_DIR "${LOCAL_JUCE_DIR}"
        )
    elseif(EXISTS "${LOCAL_JUCE_DIR_RB26}/CMakeLists.txt")
        message(STATUS "Using local cached JUCE 8 from ${LOCAL_JUCE_DIR_RB26}")
        FetchContent_Declare(
            JUCE
            SOURCE_DIR "${LOCAL_JUCE_DIR_RB26}"
        )
    else()
        message(STATUS "Fetching JUCE 8 (8.0.6)...")
        FetchContent_Declare(
            JUCE
            GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
            GIT_TAG        8.0.6
            GIT_SHALLOW    TRUE
        )
    endif()
    FetchContent_MakeAvailable(JUCE)
endif()

# ==============================================================================
# 3. Fetch CLAP-JUCE Extensions for Native CLAP 1.0+ Support
# ==============================================================================
if(NOT TARGET clap_juce_extensions)
    message(STATUS "Provisioning clap-juce-extensions...")
    FetchContent_Declare(
        clap-juce-extensions
        GIT_REPOSITORY https://github.com/free-audio/clap-juce-extensions.git
        GIT_TAG        main
        GIT_SUBMODULES_RECURSE TRUE
    )
    FetchContent_MakeAvailable(clap-juce-extensions)
endif()

# ==============================================================================
# 4. mr16_dsp_core Static Library (GUI-free, pure real-time C++20 DSP engine)
# ==============================================================================
file(GLOB_RECURSE MR16_DSP_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/source/dsp/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/dsp/*.h"
)

if(NOT MR16_DSP_SOURCES)
    set(MR16_DSP_SOURCES
        source/dsp/DspMath.h
        source/dsp/KineticExciter.cpp
        source/dsp/KineticExciter.h
        source/dsp/LorenzAttractor.cpp
        source/dsp/LorenzAttractor.h
        source/dsp/ModalResonatorMatrix.cpp
        source/dsp/ModalResonatorMatrix.h
        source/dsp/SpatialChorus.cpp
        source/dsp/SpatialChorus.h
        source/dsp/VactrolGate.cpp
        source/dsp/VactrolGate.h
        source/dsp/BoundedSaturator.cpp
        source/dsp/BoundedSaturator.h
        source/dsp/Mr16Engine.cpp
        source/dsp/Mr16Engine.h
    )
endif()

add_library(mr16_dsp_core STATIC ${MR16_DSP_SOURCES})

target_include_directories(mr16_dsp_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/source
    ${CMAKE_CURRENT_SOURCE_DIR}/source/dsp
)

target_link_libraries(mr16_dsp_core PUBLIC
    juce::juce_audio_basics
    juce::juce_dsp
    juce::juce_core
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

# ==============================================================================
# 5. Packaging Web Assets (Zip Archive & Binary Data if web UI enabled)
# ==============================================================================
if(MR16_USE_WEBVIEW)
    set(MR16_WEB_DIR "")
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ui/index.html")
        set(MR16_WEB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ui")
    elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/web/index.html")
        set(MR16_WEB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/web")
    endif()

    if(MR16_WEB_DIR)
        set(MR16_WEB_ASSETS_ZIP "${CMAKE_CURRENT_BINARY_DIR}/web_assets_mr16.zip")
        if(NOT EXISTS "${MR16_WEB_ASSETS_ZIP}")
            message(STATUS "Packaging initial web assets archive: ${MR16_WEB_ASSETS_ZIP}")
            execute_process(
                COMMAND ${CMAKE_COMMAND} -E tar cf "${MR16_WEB_ASSETS_ZIP}" --format=zip .
                WORKING_DIRECTORY "${MR16_WEB_DIR}"
            )
        endif()
        file(GLOB_RECURSE MR16_WEB_FILES "${MR16_WEB_DIR}/*")
        add_custom_command(
            OUTPUT "${MR16_WEB_ASSETS_ZIP}"
            COMMAND ${CMAKE_COMMAND} -E tar cf "${MR16_WEB_ASSETS_ZIP}" --format=zip .
            WORKING_DIRECTORY "${MR16_WEB_DIR}"
            DEPENDS ${MR16_WEB_FILES}
            COMMENT "Packaging MR-16 web assets into web_assets_mr16.zip"
        )
        add_custom_target(PackageMr16WebAssets DEPENDS "${MR16_WEB_ASSETS_ZIP}")
        juce_add_binary_data(BraunMr16WebAssets SOURCES "${MR16_WEB_ASSETS_ZIP}")
        add_dependencies(BraunMr16WebAssets PackageMr16WebAssets)
    endif()
endif()

# ==============================================================================
# 6. Target Plugin Formats & BRAUN_MR16 Plugin Declaration
# ==============================================================================
set(MR16_FORMATS VST3 Standalone)
if(APPLE)
    if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
        set(MR16_FORMATS AUv3 Standalone)
    else()
        list(APPEND MR16_FORMATS AU AUv3)
    endif()
endif()

set(MR16_EXTRA_PLUGIN_ARGS "")
if(MR16_USE_WEBVIEW)
    if(WIN32)
        list(APPEND MR16_EXTRA_PLUGIN_ARGS NEEDS_WEBVIEW2 TRUE)
    endif()
    if(UNIX AND NOT APPLE)
        list(APPEND MR16_EXTRA_PLUGIN_ARGS NEEDS_WEB_BROWSER TRUE)
    endif()
endif()

juce_add_plugin(BRAUN_MR16
    COMPANY_NAME "Braun"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE Brun
    PLUGIN_CODE Mr16
    FORMATS ${MR16_FORMATS}
    PRODUCT_NAME "BRAUN_MR16"
    VERSION "1.0.4"
    ${MR16_EXTRA_PLUGIN_ARGS}
)

target_sources(BRAUN_MR16 PRIVATE
    source/plugin/PluginProcessor.cpp
    source/plugin/PluginProcessor.h
    source/plugin/PluginEditor.cpp
    source/plugin/PluginEditor.h
    source/plugin/Parameters.h
    source/plugin/LookAndFeel/BraunLookAndFeel.cpp
    source/plugin/LookAndFeel/BraunLookAndFeel.h
)

target_include_directories(BRAUN_MR16 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/source
    ${CMAKE_CURRENT_SOURCE_DIR}/source/plugin
    ${CMAKE_CURRENT_SOURCE_DIR}/source/plugin/LookAndFeel
    ${CMAKE_CURRENT_SOURCE_DIR}/source/dsp
)

if(MR16_USE_WEBVIEW)
    target_compile_definitions(BRAUN_MR16 PUBLIC
        JUCE_WEB_BROWSER=1
        MR16_USE_WEBVIEW=1
        JUCE_USE_WIN_WEBVIEW2=1
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
    )
else()
    target_compile_definitions(BRAUN_MR16 PUBLIC
        JUCE_WEB_BROWSER=0
        MR16_USE_WEBVIEW=0
        JUCE_USE_WIN_WEBVIEW2=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
    )
endif()

target_link_libraries(BRAUN_MR16 PRIVATE
    mr16_dsp_core
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_plugin_client
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_data_structures
    juce::juce_dsp
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

if(TARGET BraunMr16WebAssets)
    target_link_libraries(BRAUN_MR16 PRIVATE BraunMr16WebAssets)
endif()

# Linux embedded WebKitGTK sub-process requirement
if(UNIX AND NOT APPLE)
    get_directory_property(_sub_target JUCE_EMBEDDED_LINUX_SUBPROCESS_TARGET)
    if(_sub_target)
        target_link_libraries(BRAUN_MR16 PUBLIC ${_sub_target})
    endif()
endif()

# Attach CLAP format target via clap-juce-extensions
if(TARGET clap_juce_extensions)
    clap_juce_extensions_plugin(
        TARGET BRAUN_MR16
        CLAP_ID "com.braun.mr16"
        CLAP_FEATURES audio-effect synthesizer physical-modelling modal spatial chorus
    )
endif()

# ==============================================================================
# 7. Headless Continuous Verification Test Runner
# ==============================================================================
if(MR16_BUILD_TESTS)
    enable_testing()

    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/source/tests/CMakeLists.txt")
        add_subdirectory(source/tests)
    elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/source/tests/mr16_headless_dsp_tests.cpp")
        add_executable(mr16_headless_dsp_tests
            source/tests/mr16_headless_dsp_tests.cpp
        )
        target_link_libraries(mr16_headless_dsp_tests PRIVATE
            mr16_dsp_core
        )
        target_include_directories(mr16_headless_dsp_tests PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/source
            ${CMAKE_CURRENT_SOURCE_DIR}/source/dsp
            ${CMAKE_CURRENT_SOURCE_DIR}/source/tests
        )
        set_target_properties(mr16_headless_dsp_tests PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
        add_test(NAME Mr16HeadlessDspTests COMMAND mr16_headless_dsp_tests)
    endif()
endif()
