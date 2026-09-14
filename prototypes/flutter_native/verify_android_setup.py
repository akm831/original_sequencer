#!/usr/bin/env python3
"""Verify the Flutter + native Android prototype wiring.

This is a lightweight preflight for the repository-controlled parts of the
Flutter candidate. It does not replace an Android build or a device launch.

Examples:
    python3 prototypes/flutter_native/verify_android_setup.py
    python3 prototypes/flutter_native/verify_android_setup.py --require-apk
    python3 prototypes/flutter_native/verify_android_setup.py --apk path/to/app-debug.apk
"""

from __future__ import annotations

import argparse
import re
import sys
import zipfile
from pathlib import Path

EXPECTED_COMPILE_SDK = "36"
EXPECTED_MIN_SDK = "24"
EXPECTED_TARGET_SDK = "36"
EXPECTED_NDK = "28.2.13676358"
EXPECTED_CMAKE = "3.22.1"
EXPECTED_OBOE = "1.10.0"
EXPECTED_LIBRARY = "liboriginal_sequencer_flutter_bridge.so"


def check(condition: bool, success: str, failure: str, errors: list[str]) -> None:
    if condition:
        print(f"OK: {success}")
    else:
        print(f"ERROR: {failure}")
        errors.append(failure)


def contains(pattern: str, text: str) -> bool:
    return re.search(pattern, text, flags=re.MULTILINE) is not None


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--apk", type=Path, help="APK to inspect for the Flutter native bridge shared library.")
    parser.add_argument(
        "--require-apk",
        action="store_true",
        help="Fail when no APK is available. Defaults to app-debug.apk in Flutter's build output.",
    )
    args = parser.parse_args()

    script_dir = Path(__file__).resolve().parent
    app_dir = script_dir / "app"
    gradle_file = app_dir / "android" / "app" / "build.gradle.kts"
    platform_cmake_file = script_dir / "platform" / "android" / "CMakeLists.txt"
    native_cmake_file = script_dir / "native" / "CMakeLists.txt"
    bridge_header_file = script_dir / "native" / "include" / "prototype_bridge.h"
    audio_backend_file = script_dir / "native" / "src" / "android_audio_backend.cpp"
    dart_bridge_file = app_dir / "lib" / "native_bridge.dart"
    dart_main_file = app_dir / "lib" / "main.dart"

    required_files = [
        gradle_file,
        platform_cmake_file,
        native_cmake_file,
        bridge_header_file,
        audio_backend_file,
        dart_bridge_file,
        dart_main_file,
    ]

    errors: list[str] = []
    for path in required_files:
        check(path.is_file(), f"Found {path.relative_to(script_dir)}", f"Missing {path}", errors)

    if errors:
        print(f"\nPreflight failed with {len(errors)} error(s).")
        return 1

    gradle = gradle_file.read_text(encoding="utf-8")
    platform_cmake = platform_cmake_file.read_text(encoding="utf-8")
    native_cmake = native_cmake_file.read_text(encoding="utf-8")
    bridge_header = bridge_header_file.read_text(encoding="utf-8")
    audio_backend = audio_backend_file.read_text(encoding="utf-8")
    dart_bridge = dart_bridge_file.read_text(encoding="utf-8")
    dart_main = dart_main_file.read_text(encoding="utf-8")

    print("== Flutter Android prototype preflight ==")
    check(contains(rf"compileSdk\s*=\s*{EXPECTED_COMPILE_SDK}\b", gradle), f"compileSdk is {EXPECTED_COMPILE_SDK}", f"compileSdk is not explicitly {EXPECTED_COMPILE_SDK}", errors)
    check(contains(rf"minSdk\s*=\s*{EXPECTED_MIN_SDK}\b", gradle), f"minSdk is {EXPECTED_MIN_SDK}", f"minSdk is not explicitly {EXPECTED_MIN_SDK}", errors)
    check(contains(rf"targetSdk\s*=\s*{EXPECTED_TARGET_SDK}\b", gradle), f"targetSdk is {EXPECTED_TARGET_SDK}", f"targetSdk is not explicitly {EXPECTED_TARGET_SDK}", errors)
    check(contains(rf'ndkVersion\s*=\s*"{re.escape(EXPECTED_NDK)}"', gradle), f"NDK is pinned to {EXPECTED_NDK}", f"NDK is not pinned to {EXPECTED_NDK}", errors)
    check(contains(rf'version\s*=\s*"{re.escape(EXPECTED_CMAKE)}"', gradle), f"externalNativeBuild CMake is {EXPECTED_CMAKE}", f"externalNativeBuild CMake is not {EXPECTED_CMAKE}", errors)
    check("../../../platform/android/CMakeLists.txt" in gradle, "Gradle points at the prototype Android CMake entry", "Gradle does not point at ../../../platform/android/CMakeLists.txt", errors)
    check("prefab = true" in gradle, "Gradle enables Prefab for native dependencies", "Gradle does not enable Prefab", errors)
    check(f'implementation("com.google.oboe:oboe:{EXPECTED_OBOE}")' in gradle, f"Oboe is pinned to {EXPECTED_OBOE}", f"Oboe is not pinned to {EXPECTED_OBOE}", errors)
    check("../../../common/audio_core" in platform_cmake and "../../native" in platform_cmake, "Android CMake includes Common Audio Core and Flutter native bridge", "Android CMake does not include both Common Audio Core and Flutter native bridge", errors)
    check(contains(r"if\(ANDROID\)[\s\S]*add_library\(original_sequencer_flutter_bridge\s+SHARED", native_cmake), "Android bridge target is a shared library", "Android bridge target is not visibly declared as SHARED", errors)
    check("target_link_libraries(original_sequencer_flutter_bridge PUBLIC original_sequencer_audio_core)" in native_cmake, "Flutter bridge links Common Audio Core", "Flutter bridge does not visibly link Common Audio Core", errors)
    check("find_package(oboe REQUIRED CONFIG)" in native_cmake and "oboe::oboe" in native_cmake, "Flutter bridge resolves and links Oboe", "Flutter bridge does not visibly resolve and link Oboe", errors)
    check("target_compile_features(original_sequencer_flutter_bridge PUBLIC cxx_std_20)" in native_cmake, "Flutter bridge requires C++20", "Flutter bridge does not visibly require C++20", errors)

    expected_symbols = [
        "prototype_create",
        "prototype_destroy",
        "prototype_start_audio",
        "prototype_stop_audio",
        "prototype_get_diagnostics",
    ]
    for symbol in expected_symbols:
        check(symbol in bridge_header and symbol in dart_bridge, f"FFI symbol {symbol} matches C header and Dart lookup", f"FFI symbol {symbol} is missing from the C header or Dart lookup", errors)

    check(f"DynamicLibrary.open('{EXPECTED_LIBRARY}')" in dart_bridge, f"Dart opens {EXPECTED_LIBRARY}", f"Dart does not open {EXPECTED_LIBRARY}", errors)
    check("builder.setPerformanceMode(oboe::PerformanceMode::LowLatency)" in audio_backend, "Oboe requests low-latency performance mode", "Oboe low-latency performance mode request is missing", errors)
    check("kTestToneFrequencyHz = 220.0" in audio_backend and "kTestToneAmplitude = 0.08F" in audio_backend, "Flutter P1 test tone matches JUCE 220 Hz / 0.08", "Flutter P1 test tone does not match JUCE comparison values", errors)
    check("core_.render(output, frameCount, channelCount, callbackStartFrame_)" in audio_backend, "Oboe callback renders through Common Audio Core", "Oboe callback does not visibly render through Common Audio Core", errors)
    check("Native bridge: ${bridgeLoaded ? 'loaded' : 'not loaded'}" in dart_main, "Flutter screen exposes native bridge load status", "Flutter screen does not expose native bridge load status", errors)
    check("Audio stream: ${_audioRunning ? 'running' : 'stopped'}" in dart_main, "Flutter screen exposes audio stream status", "Flutter screen does not expose audio stream status", errors)
    check("Duration(milliseconds: 200)" in dart_main, "Flutter refreshes diagnostics at 5 Hz", "Flutter diagnostics refresh cadence is not visibly 5 Hz", errors)

    apk_path = args.apk
    if apk_path is None and args.require_apk:
        apk_path = app_dir / "build" / "app" / "outputs" / "flutter-apk" / "app-debug.apk"

    if apk_path is not None:
        print("\n== APK packaging ==")
        if not apk_path.is_file():
            check(False, "", f"APK not found: {apk_path}", errors)
        else:
            try:
                with zipfile.ZipFile(apk_path) as apk:
                    matches = sorted(name for name in apk.namelist() if name.startswith("lib/") and name.endswith(f"/{EXPECTED_LIBRARY}"))
            except zipfile.BadZipFile:
                check(False, "", f"Not a valid APK/ZIP: {apk_path}", errors)
            else:
                check(bool(matches), f"APK contains {EXPECTED_LIBRARY}", f"APK does not contain {EXPECTED_LIBRARY}", errors)
                for name in matches:
                    print(f"  - {name}")
    else:
        print("\nINFO: APK packaging check skipped. Use --require-apk after flutter build apk --debug.")

    if errors:
        print(f"\nPreflight failed with {len(errors)} error(s).")
        return 1

    print("\nPreflight passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
