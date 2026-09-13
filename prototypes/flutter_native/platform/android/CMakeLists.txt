cmake_minimum_required(VERSION 3.22)
project(original_sequencer_flutter_android LANGUAGES CXX)

set(ORIGINAL_SEQUENCER_BUILD_TESTS OFF CACHE BOOL "Build prototype smoke tests" FORCE)

add_subdirectory(
    "${CMAKE_CURRENT_LIST_DIR}/../../../common/audio_core"
    "${CMAKE_CURRENT_BINARY_DIR}/audio_core"
)

add_subdirectory(
    "${CMAKE_CURRENT_LIST_DIR}/../../native"
    "${CMAKE_CURRENT_BINARY_DIR}/flutter_bridge"
)
