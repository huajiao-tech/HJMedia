plugins {
    alias(libs.plugins.android.application)
}

android {
    namespace = "com.example.ui"
    compileSdk = 34 // Use a stable version, 36 is not released yet

    sourceSets {
        getByName("main") {
            assets {
                srcDir(file("../../../resource"))
            }
        }
    }

    defaultConfig {
        applicationId = "com.example.ui"
        minSdk = 21
        targetSdk = 34 // Match compileSdk for consistency
        versionCode = 1
        versionName = "1.0"

        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"

        externalNativeBuild {
            cmake {
                cppFlags += "-std=c++17 -frtti -fexceptions"
                arguments += listOf(
                    "-DANDROID_STUDIO_DEBUG=1",
                    "-DCMAKE_SHARED_LINKER_FLAGS=-Wl,-Bsymbolic",
                    "-DCMAKE_BUILD_TYPE=Release",
                    "-DANDROID_STL=c++_static",
                    "-DANDROID_ARM_NEON=ON",
                    "-DANDROID_APP_PIE=TRUE",
                    "-DANDROID_TOOLCHAIN=clang",
                    "-DCMAKE_CXX_FLAGS=-Wno-c++11-narrowing",
                    "-DANDROID_NATIVE_API_LEVEL=21",
                    "-DANDROID_STL_FORCE_FEATURES=TRUE",
                    "-DANDROID_FORCE_ARM_BUILD=TRUE",
                    "-DANDROID_UNIFIED_HEADERS=TRUE"
                )
            }
        }

        ndk {
            abiFilters.addAll(listOf("arm64-v8a"))
        }
    }

    externalNativeBuild {
        cmake {
            version = "3.22.1"
            path = file("../../../../CMakeLists.txt")
        }
    }

    buildTypes {
        release {
            isMinifyEnabled = false
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro"
            )
        }
    }
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_11
        targetCompatibility = JavaVersion.VERSION_11
    }
}

dependencies {
    implementation(libs.appcompat)
    implementation(libs.material)
    implementation(libs.activity)
    implementation(libs.constraintlayout)
    testImplementation(libs.junit)
//    androidTestImplementation(libs.ext.junit)
//    androidTestImplementation(libs.espresso.core)

    implementation(project(":hjmediasdk"))
}
