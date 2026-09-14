if(ANDROID)
    find_package(oboe REQUIRED CONFIG)

    add_library(original_sequencer_flutter_bridge SHARED
        src/android_audio_backend.cpp
        src/prototype_bridge.cpp
    )
else()
    add_library(original_sequencer_flutter_bridge STATIC
        src/prototype_bridge.cpp
    )
endif()

target_include_directories(original_sequencer_flutter_bridge PUBLIC include)
target_link_libraries(original_sequencer_flutter_bridge PUBLIC original_sequencer_audio_core)
target_compile_features(original_sequencer_flutter_bridge PUBLIC cxx_std_20)

if(ANDROID)
    target_link_libraries(original_sequencer_flutter_bridge PRIVATE oboe::oboe)
endif()

if(ORIGINAL_SEQUENCER_BUILD_TESTS)
    add_executable(flutter_bridge_smoke tests/flutter_bridge_smoke.cpp)
    target_link_libraries(flutter_bridge_smoke PRIVATE original_sequencer_flutter_bridge)
    add_test(NAME flutter_bridge_smoke COMMAND flutter_bridge_smoke)
endif()
