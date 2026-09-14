plugins {
    id("com.android.application")
    id("kotlin-android")
    id("dev.flutter.flutter-gradle-plugin")
}

android {
    namespace = "com.originalsequencer.prototype"

    // Prototype comparison must be reproducible across Candidate A/B.
    // Keep these values explicit instead of inheriting moving Flutter defaults.
    compileSdk = 36
    ndkVersion = "28.2.13676358"

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }

    kotlinOptions {
        jvmTarget = JavaVersion.VERSION_17.toString()
    }

    buildFeatures {
        prefab = true
    }

    defaultConfig {
        applicationId = "com.originalsequencer.prototype"
        minSdk = 24
        targetSdk = 36
        versionCode = flutter.versionCode
        versionName = flutter.versionName

        externalNativeBuild {
            cmake {
                arguments += listOf(
                    "-DORIGINAL_SEQUENCER_BUILD_TESTS=OFF",
                    "-DANDROID_STL=c++_shared",
                )
                cppFlags += "-std=c++20"
            }
        }
    }

    externalNativeBuild {
        cmake {
            path = file("../../../platform/android/CMakeLists.txt")
            version = "3.22.1"
        }
    }

    buildTypes {
        release {
            signingConfig = signingConfigs.getByName("debug")
        }
    }
}

dependencies {
    implementation("com.google.oboe:oboe:1.10.0")
}

flutter {
    source = "../.."
}
