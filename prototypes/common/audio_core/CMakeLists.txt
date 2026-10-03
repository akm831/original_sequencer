cmake_minimum_required(VERSION 3.22)
project(original_sequencer_audio_core LANGUAGES CXX)

add_library(original_sequencer_audio_core STATIC
    src/AudioCore.cpp
)

target_include_directories(original_sequencer_audio_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(original_sequencer_audio_core PUBLIC cxx_std_20)

if(ORIGINAL_SEQUENCER_BUILD_TESTS)
    enable_testing()

    add_executable(audio_core_smoke tests/audio_core_smoke.cpp)
    target_link_libraries(audio_core_smoke PRIVATE original_sequencer_audio_core)
    add_test(NAME audio_core_smoke COMMAND audio_core_smoke)

    add_executable(prototype_sequencer_smoke tests/prototype_sequencer_smoke.cpp)
    target_link_libraries(prototype_sequencer_smoke PRIVATE original_sequencer_audio_core)
    add_test(NAME prototype_sequencer_smoke COMMAND prototype_sequencer_smoke)

    add_executable(prototype_sequencer_soak tests/prototype_sequencer_soak.cpp)
    target_link_libraries(prototype_sequencer_soak PRIVATE original_sequencer_audio_core)
    add_test(NAME prototype_sequencer_soak COMMAND prototype_sequencer_soak)

    add_executable(audio_trigger_smoke tests/audio_trigger_smoke.cpp)
    target_link_libraries(audio_trigger_smoke PRIVATE original_sequencer_audio_core)
    add_test(NAME audio_trigger_smoke COMMAND audio_trigger_smoke)

    add_executable(audio_command_queue_smoke tests/audio_command_queue_smoke.cpp)
    target_link_libraries(audio_command_queue_smoke PRIVATE original_sequencer_audio_core)
    add_test(NAME audio_command_queue_smoke COMMAND audio_command_queue_smoke)
endif()
