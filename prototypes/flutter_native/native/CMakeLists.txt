add_library(original_sequencer_flutter_bridge STATIC
    src/prototype_bridge.cpp
)

target_include_directories(original_sequencer_flutter_bridge PUBLIC include)
target_link_libraries(original_sequencer_flutter_bridge PUBLIC original_sequencer_audio_core)
target_compile_features(original_sequencer_flutter_bridge PUBLIC cxx_std_20)

if(ORIGINAL_SEQUENCER_BUILD_TESTS)
    add_executable(flutter_bridge_smoke tests/flutter_bridge_smoke.cpp)
    target_link_libraries(flutter_bridge_smoke PRIVATE original_sequencer_flutter_bridge)
    add_test(NAME flutter_bridge_smoke COMMAND flutter_bridge_smoke)
endif()
