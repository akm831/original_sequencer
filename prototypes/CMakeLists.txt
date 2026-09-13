cmake_minimum_required(VERSION 3.22)
project(original_sequencer_prototypes LANGUAGES CXX)

option(ORIGINAL_SEQUENCER_BUILD_TESTS "Build prototype smoke tests" ON)

if(ORIGINAL_SEQUENCER_BUILD_TESTS)
    enable_testing()
endif()

add_subdirectory(common/audio_core)
add_subdirectory(flutter_native/native)
