cmake_minimum_required(VERSION 3.22)
project(original_sequencer_juce_prototype VERSION 0.0.1 LANGUAGES CXX)

if(ANDROID)
    message(FATAL_ERROR
        "JUCE's CMake API does not support Android targets. "
        "Use the Projucer/Android Studio path documented in prototypes/juce/README.md."
    )
endif()

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 9.0.2
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

add_subdirectory(../common/audio_core ${CMAKE_CURRENT_BINARY_DIR}/audio_core)

juce_add_gui_app(original_sequencer_juce_app
    PRODUCT_NAME "Sequencer Prototype"
)

target_sources(original_sequencer_juce_app PRIVATE app/Main.cpp)
target_compile_features(original_sequencer_juce_app PRIVATE cxx_std_20)
target_link_libraries(original_sequencer_juce_app PRIVATE
    original_sequencer_audio_core
    juce::juce_audio_utils
    juce::juce_gui_extra
)
