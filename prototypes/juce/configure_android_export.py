#!/usr/bin/env python3
"""Pin the JUCE 9.0.2 generated Android build to the comparison toolchain."""
from pathlib import Path
import re

path = Path(__file__).resolve().parent / 'Builds/Android/app/build.gradle'
text = path.read_text()
text, count = re.subn(r'def ndkVersionString = "(?:28\.1\.13356709|28\.2\.13676358)"',
                     'def ndkVersionString = "28.2.13676358"', text)
if count != 1:
    raise SystemExit('Unexpected Projucer NDK declaration; inspect the export.')
text, count = re.subn(r'java\.toolchain\.languageVersion = JavaLanguageVersion\.of\((?:8|17)\)',
                     'java.toolchain.languageVersion = JavaLanguageVersion.of(17)', text)
if count != 1:
    raise SystemExit('Unexpected Projucer Java toolchain declaration.')
# Run Gradle with JDK 17, but preserve Java 8 bytecode for Android API 24.
options = '''    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_1_8
        targetCompatibility = JavaVersion.VERSION_1_8
    }
'''
if options not in text:
    if text.count('android {\n') != 1:
        raise SystemExit('Unexpected Android configuration block.')
    text = text.replace('android {\n', 'android {\n' + options, 1)
path.write_text(text)
print('Pinned NDK 28.2.13676358 / JDK 17 / Java 8 bytecode.')
