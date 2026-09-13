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
endif()
