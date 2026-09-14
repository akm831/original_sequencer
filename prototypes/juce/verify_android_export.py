#!/usr/bin/env python3
"""Validate the JUCE Android prototype before and after Projucer export."""

from __future__ import annotations

import argparse
import os
import re
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

EXPECTED_MIN_SDK = "24"
EXPECTED_TARGET_SDK = "36"
EXPECTED_NDK = "28.2.13676358"
EXPECTED_CPP_STANDARD = "20"
REQUIRED_MODULES = {"juce_audio_basics", "juce_audio_devices", "juce_audio_utils"}
REQUIRED_COMPILE_FILES = {"app/Main.cpp", "../common/audio_core/src/AudioCore.cpp"}
TEXT_SUFFIXES = {
    ".gradle",
    ".kts",
    ".properties",
    ".xml",
    ".txt",
    ".cmake",
    ".mk",
    ".cpp",
    ".h",
}


def fail(errors: list[str], message: str) -> None:
    errors.append(message)
    print(f"ERROR: {message}")


def ok(message: str) -> None:
    print(f"OK: {message}")


def warn(message: str) -> None:
    print(f"WARN: {message}")


def normalized(value: str | None) -> str:
    return (value or "").replace("\\", "/")


def validate_jucer(jucer: Path, errors: list[str]) -> Path | None:
    try:
        root = ET.parse(jucer).getroot()
    except (ET.ParseError, OSError) as exc:
        fail(errors, f"Could not parse {jucer}: {exc}")
        return None

    cpp_standard = root.get("cppLanguageStandard", "")
    if cpp_standard == EXPECTED_CPP_STANDARD:
        ok(f".jucer C++ language standard is C++{EXPECTED_CPP_STANDARD}")
    else:
        fail(
            errors,
            ".jucer cppLanguageStandard expected "
            f"{EXPECTED_CPP_STANDARD!r}, got {cpp_standard!r}",
        )

    exporter = root.find("./EXPORTFORMATS/ANDROIDSTUDIO")
    if exporter is None:
        fail(errors, "ANDROIDSTUDIO exporter is missing from SequencerPrototype.jucer")
        return None

    checks = {
        "androidMinimumSDK": EXPECTED_MIN_SDK,
        "androidTargetSDK": EXPECTED_TARGET_SDK,
        "targetFolder": "Builds/Android",
    }
    for key, expected in checks.items():
        actual = normalized(exporter.get(key))
        if actual == expected:
            ok(f".jucer {key}={expected}")
        else:
            fail(errors, f".jucer {key} expected {expected!r}, got {actual!r}")

    header_path = normalized(exporter.get("headerPath"))
    if "../common/audio_core/include" in header_path:
        ok(".jucer includes Common Audio Core headers")
    else:
        fail(errors, ".jucer headerPath is missing ../common/audio_core/include")

    modules = {node.get("id", "") for node in root.findall("./MODULES/MODULE")}
    missing_modules = sorted(REQUIRED_MODULES - modules)
    if missing_modules:
        fail(errors, f".jucer is missing required JUCE modules: {', '.join(missing_modules)}")
    else:
        ok(".jucer contains required JUCE audio modules")

    compile_files = {
        normalized(node.get("file"))
        for node in root.findall(".//FILE")
        if node.get("compile") == "1"
    }
    missing_files = sorted(REQUIRED_COMPILE_FILES - compile_files)
    if missing_files:
        fail(errors, f".jucer compile list is missing: {', '.join(missing_files)}")
    else:
        ok(".jucer compiles Main.cpp and Common AudioCore.cpp")

    target_folder = normalized(exporter.get("targetFolder")) or "Builds/Android"
    return (jucer.parent / target_folder).resolve()


def collect_generated_text(project_dir: Path) -> tuple[str, list[Path]]:
    chunks: list[str] = []
    files: list[Path] = []
    for path in project_dir.rglob("*"):
        if not path.is_file() or path.suffix.lower() not in TEXT_SUFFIXES:
            continue
        try:
            text = path.read_text(encoding="utf-8", errors="ignore")
        except OSError:
            continue
        files.append(path)
        chunks.append(f"\n# FILE: {path.relative_to(project_dir)}\n{text}")
    return "".join(chunks), files


def contains_sdk(text: str, kind: str, value: str) -> bool:
    patterns = {
        "min": [rf"minSdk(?:Version)?\s*(?:=\s*)?{re.escape(value)}\b"],
        "target": [rf"targetSdk(?:Version)?\s*(?:=\s*)?{re.escape(value)}\b"],
        "compile": [rf"compileSdk(?:Version)?\s*(?:=\s*)?{re.escape(value)}\b"],
    }
    return any(re.search(pattern, text) for pattern in patterns[kind])


def contains_cpp_standard(text: str, value: str) -> bool:
    patterns = [
        rf"CMAKE_CXX_STANDARD\s+(?:CACHE\s+STRING\s+)?{re.escape(value)}\b",
        rf"CMAKE_CXX_STANDARD\s*=\s*{re.escape(value)}\b",
    ]
    return any(re.search(pattern, text) for pattern in patterns)


def contains_ndk(text: str, value: str) -> bool:
    patterns = [
        rf"ndkVersion\s*(?:=\s*)?[\"']?{re.escape(value)}[\"']?",
        rf"ndkVersionString\s*=\s*[\"']{re.escape(value)}[\"']",
        rf"ANDROID_NDK_VERSION\s*(?:=|\s)\s*[\"']?{re.escape(value)}[\"']?",
    ]
    return any(re.search(pattern, text) for pattern in patterns)


def validate_generated(project_dir: Path, errors: list[str], expected_ndk: str) -> None:
    text, files = collect_generated_text(project_dir)
    if not files:
        fail(errors, f"No readable generated Android project files found under {project_dir}")
        return

    ok(f"Generated Android project found ({len(files)} text files inspected)")

    for kind, value in (
        ("min", EXPECTED_MIN_SDK),
        ("target", EXPECTED_TARGET_SDK),
        ("compile", EXPECTED_TARGET_SDK),
    ):
        if contains_sdk(text, kind, value):
            ok(f"Generated project declares {kind} SDK {value}")
        else:
            fail(errors, f"Generated project does not visibly declare {kind} SDK {value}")

    if contains_cpp_standard(text, EXPECTED_CPP_STANDARD):
        ok(f"Generated project declares CMAKE_CXX_STANDARD {EXPECTED_CPP_STANDARD}")
    else:
        fail(
            errors,
            "Generated project does not visibly declare "
            f"CMAKE_CXX_STANDARD {EXPECTED_CPP_STANDARD}",
        )

    normalized_text = text.replace("\\", "/")
    if "Main.cpp" in normalized_text:
        ok("Generated project references Main.cpp")
    else:
        fail(errors, "Generated project does not visibly reference Main.cpp")

    if "AudioCore.cpp" in normalized_text:
        ok("Generated project references AudioCore.cpp")
    else:
        fail(errors, "Generated project does not visibly reference AudioCore.cpp")

    if "common/audio_core/include" in normalized_text:
        ok("Generated project references Common Audio Core include path")
    else:
        warn(
            "Generated text does not visibly contain common/audio_core/include; "
            "inspect compiler include paths during build"
        )

    if contains_ndk(text, expected_ndk):
        ok(f"Generated project pins NDK {expected_ndk}")
    else:
        fail(
            errors,
            f"Generated project does not visibly pin NDK {expected_ndk}. "
            "Regenerate or update the local Android project before building.",
        )


def validate_ndk_install(expected_ndk: str) -> None:
    sdk_root = os.environ.get("ANDROID_SDK_ROOT") or os.environ.get("ANDROID_HOME")
    if not sdk_root:
        warn("ANDROID_SDK_ROOT / ANDROID_HOME is not set; local NDK installation was not checked")
        return

    ndk_dir = Path(sdk_root) / "ndk" / expected_ndk
    if ndk_dir.is_dir():
        ok(f"Reference NDK is installed: {ndk_dir}")
    else:
        warn(f"Reference NDK {expected_ndk} was not found under {Path(sdk_root) / 'ndk'}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--require-generated",
        action="store_true",
        help="fail if the Projucer Android export has not been generated",
    )
    parser.add_argument("--expected-ndk", default=EXPECTED_NDK)
    args = parser.parse_args()

    script_dir = Path(__file__).resolve().parent
    jucer = script_dir / "SequencerPrototype.jucer"
    errors: list[str] = []

    print("== JUCE Android prototype preflight ==")
    generated_dir = validate_jucer(jucer, errors)
    validate_ndk_install(args.expected_ndk)

    if generated_dir is not None:
        if generated_dir.is_dir():
            print("\n== Generated Android project ==")
            validate_generated(generated_dir, errors, args.expected_ndk)
        elif args.require_generated:
            fail(errors, f"Projucer export is missing: {generated_dir}")
        else:
            warn(f"Projucer export is not generated yet: {generated_dir}")
            warn(
                "Open SequencerPrototype.jucer in JUCE 9.0.2 Projucer and Save Project, "
                "then rerun with --require-generated"
            )

    if errors:
        print(f"\nFAILED: {len(errors)} validation error(s)")
        return 1

    print("\nPASS: JUCE Android prototype preflight checks succeeded")
    return 0


if __name__ == "__main__":
    sys.exit(main())
