cmake_minimum_required(VERSION 3.21)

# Some CI/dev Linux images ship a Clang toolchain without a usable libstdc++
# dev link path configured for /usr/bin/c++. Prefer GCC if the user didn't
# explicitly select a compiler.
if(UNIX AND NOT APPLE)
  if(NOT DEFINED CMAKE_CXX_COMPILER AND EXISTS "/usr/bin/g++")
    set(CMAKE_CXX_COMPILER "/usr/bin/g++" CACHE STRING "C++ compiler" FORCE)
  endif()
endif()

project(PG300Controller
  VERSION 0.1.0
  LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# JUCE is fetched at configure-time.
# If your environment has JUCE vendored locally, replace this with add_subdirectory().
FetchContent_Declare(
  juce
  GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
  GIT_TAG 8.0.7
)
FetchContent_MakeAvailable(juce)

juce_add_gui_app(PG300Controller
  PRODUCT_NAME "PG-300 Controller"
  COMPANY_NAME "PG300"
  BUNDLE_ID "com.pg300.controller"
  MICROPHONE_PERMISSION_ENABLED FALSE
)

target_sources(PG300Controller PRIVATE
  Source/Main.cpp
  Source/MainComponent.h
  Source/MainComponent.cpp
  Source/PGParameters.h
  Source/ParameterControl.h
  Source/ParameterControl.cpp
  Source/SysEx.h
  Source/SysEx.cpp
  Source/AlphaJunoBank.h
  Source/AlphaJunoBank.cpp
  Source/MidiEngine.h
  Source/MidiEngine.cpp
)

target_compile_definitions(PG300Controller PRIVATE
  JUCE_WEB_BROWSER=0
  JUCE_USE_CURL=0
  JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(PG300Controller PRIVATE
  juce::juce_core
  juce::juce_events
  juce::juce_graphics
  juce::juce_gui_basics
  juce::juce_gui_extra
  juce::juce_audio_basics
  juce::juce_audio_devices
)

enable_testing()

add_executable(pg300_tests
  Tests/AlphaJunoBankTests.cpp
  Source/PGParameters.h
  Source/SysEx.h
  Source/SysEx.cpp
  Source/AlphaJunoBank.h
  Source/AlphaJunoBank.cpp
)

target_compile_definitions(pg300_tests PRIVATE
  JUCE_WEB_BROWSER=0
  JUCE_USE_CURL=0
)

target_link_libraries(pg300_tests PRIVATE
  juce::juce_core
  juce::juce_events
  juce::juce_audio_basics
)

add_test(NAME pg300_acceptance_tests COMMAND pg300_tests)

