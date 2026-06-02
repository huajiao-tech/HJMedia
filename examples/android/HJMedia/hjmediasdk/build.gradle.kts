plugins {
    id("com.android.library")
    id("maven-publish")
}

val publishGroupId = "com.huajiao.hjmedia_ex"
val publishArtifactId = "hjmedialibs"
val publishVersion = "1.0.13.26042701"
val publishAarName = "${publishArtifactId}-${publishVersion}.aar"
val releaseAarFile = layout.buildDirectory.file("outputs/aar/${project.name}-release.aar")

android {
    namespace = "com.HJMediasdk"
    compileSdk = 34

    defaultConfig {
        minSdk = 21
        targetSdk = 34

        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"

        // Merged Native Build Configurations
        externalNativeBuild {
            cmake {
                cppFlags += "-std=c++17 -frtti -fexceptions"
                targets += listOf("HJMediaPlayer", "HJRender")
                arguments += listOf(
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
            //abiFilters.addAll(listOf("arm64-v8a", "armeabi-v7a"))
            abiFilters.addAll(listOf("arm64-v8a"))
        }
    }
    // This block points to the main CMakeLists.txt file
    externalNativeBuild {
        cmake {
            version = "3.22.1"
            path = file("../../../../CMakeLists.txt")
        }
    }

    packaging {
        jniLibs {
            excludes += setOf(
                "**/libHJInference.so"
            )
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
    testImplementation(libs.junit)
//    androidTestImplementation(libs.ext.junit)
//    androidTestImplementation(libs.espresso.core)
}

tasks.withType<JavaCompile>().configureEach {
    options.compilerArgs.add("-Xlint:deprecation")
}

val prepareLocalExportAarContents = tasks.register<Sync>("prepareLocalExportAarContents") {
    group = "build"
    description = "Prepares a stripped AAR content tree for local export."

    dependsOn("bundleReleaseAar")

    from(zipTree(releaseAarFile)) {
        exclude("jni/**/libHJRender.so")
    }

    into(layout.buildDirectory.dir("tmp/exportToLocalDir/aar"))
}

val packageLocalExportAar = tasks.register<Zip>("packageLocalExportAar") {
    group = "build"
    description = "Packages the stripped local-export AAR."

    dependsOn(prepareLocalExportAarContents)

    from(layout.buildDirectory.dir("tmp/exportToLocalDir/aar"))
    archiveFileName.set(publishAarName)
    destinationDirectory.set(layout.buildDirectory.dir("tmp/exportToLocalDir/package"))
}

val enhanceOnlyAarName = "${publishArtifactId}-${publishVersion}-enhance-only.aar"
val enhanceOnlyClassIncludes = listOf(
    "com/HJMediasdk/entry/HJBaseNativeInstance.class",
    "com/HJMediasdk/entry/HJCommonInterface.class",
    "com/HJMediasdk/utils/HJLog.class",
    "com/HJMediasdk/entry/player/HJPlayerContextJni.class",
    "com/HJMediasdk/entry/player/HJVideoEnhanceJni.class",
    "com/HJMediasdk/entry/player/HJVideoEnhanceJni$*.class",
    "com/HJMediasdk/entry/player/HJVideoEnhanceOption.class",
    "com/HJMediasdk/entry/player/HJVideoEnhanceOption$*.class"
)

val prepareEnhanceOnlyAarContents = tasks.register<Sync>("prepareEnhanceOnlyAarContents") {
    group = "build"
    description = "Prepares a minimal enhance-only AAR content tree."

    dependsOn("bundleReleaseAar")

    from(zipTree(releaseAarFile)) {
        exclude("classes.jar")
        exclude("jni/**/libHJRender.so")
    }

    into(layout.buildDirectory.dir("tmp/exportEnhanceOnly/aar"))
}

val prepareEnhanceOnlyClassesDir = tasks.register<Sync>("prepareEnhanceOnlyClassesDir") {
    group = "build"
    description = "Extracts only the enhance-related Java classes for the minimal AAR."

    dependsOn("bundleReleaseAar")

    from({
        val aarFile = releaseAarFile.get().asFile
        val classesJarFile = zipTree(aarFile).matching {
            include("classes.jar")
        }.singleFile
        zipTree(classesJarFile)
    }) {
        enhanceOnlyClassIncludes.forEach { include(it) }
    }

    into(layout.buildDirectory.dir("tmp/exportEnhanceOnly/classes"))
}

val packageEnhanceOnlyClassesJar = tasks.register<Zip>("packageEnhanceOnlyClassesJar") {
    group = "build"
    description = "Packages the minimal enhance-only classes.jar."

    dependsOn(prepareEnhanceOnlyClassesDir)

    from(layout.buildDirectory.dir("tmp/exportEnhanceOnly/classes"))
    archiveFileName.set("classes.jar")
    destinationDirectory.set(layout.buildDirectory.dir("tmp/exportEnhanceOnly/package"))
    duplicatesStrategy = DuplicatesStrategy.EXCLUDE
}

val injectEnhanceOnlyClassesJar = tasks.register<Copy>("injectEnhanceOnlyClassesJar") {
    group = "build"
    description = "Injects the minimal classes.jar into the enhance-only AAR content tree."

    dependsOn(prepareEnhanceOnlyAarContents)
    dependsOn(packageEnhanceOnlyClassesJar)

    from(packageEnhanceOnlyClassesJar.flatMap { it.archiveFile })
    into(layout.buildDirectory.dir("tmp/exportEnhanceOnly/aar"))
}

val packageEnhanceOnlyAar = tasks.register<Zip>("packageEnhanceOnlyAar") {
    group = "build"
    description = "Packages the minimal enhance-only AAR."

    dependsOn(injectEnhanceOnlyClassesJar)

    from(layout.buildDirectory.dir("tmp/exportEnhanceOnly/aar"))
    archiveFileName.set(enhanceOnlyAarName)
    destinationDirectory.set(layout.buildDirectory.dir("tmp/exportEnhanceOnly/package"))
    duplicatesStrategy = DuplicatesStrategy.EXCLUDE
}

tasks.register("exportEnhanceOnlyAarToLocalDir", Copy::class) {
    group = "build"
    description = "Exports the minimal enhance-only AAR to the local export directory."

    dependsOn(packageEnhanceOnlyAar)

    from(packageEnhanceOnlyAar.flatMap { it.archiveFile })
    into(rootProject.projectDir.resolve("export"))

    doFirst {
        destinationDir.mkdirs()
    }

    doLast {
        println(">>> Success: Exported minimal enhance-only AAR to ${destinationDir.path}")
    }
}
publishing {
    publications {
        create<MavenPublication>("release") {
            groupId = publishGroupId
            artifactId = publishArtifactId
            version = publishVersion
            artifact(packageLocalExportAar.flatMap { it.archiveFile }) {
                extension = "aar"
                builtBy(packageLocalExportAar)
            }
        }
    }

    repositories {
        maven {
            url = uri("https://jar.huajiao.com/repository/android")
            credentials {
                username = "android_admin"
                password = "c1bn07d0b1bi860"
            }
        }
    }
}

tasks.register("assembleReleaseAndPublish") {
    group = "publishing"
    description = "Assembles the release AAR and publishes it to the Maven repository."

    dependsOn("assembleRelease")
    finalizedBy("publishReleasePublicationToMavenRepository")
}


// --- Task to export the JAR file to Demo project ---
tasks.register("exportJarToDemo", Copy::class) {
    group = "build"
    description = "Exports the library's classes.jar, renames it to HJMediasdk.jar, and copies it to the demo project."

    dependsOn("bundleReleaseAar")

    from(zipTree(releaseAarFile)) {
        include("classes.jar")
    }

    into(rootProject.projectDir.parentFile.parentFile.parentFile.resolve("demo/faceu/android/Export/app/libs"))

    rename { "HJMediasdk.jar" }

    doFirst {
        destinationDir.mkdirs()
    }

    doLast {
        println(">>> Success: Exported HJMediasdk.jar to ${destinationDir.path}")
    }
}

// --- Task to copy the resource folder to Demo project ---
tasks.register("exportResourcesToDemo", Copy::class) {
    group = "build"
    description = "Copies the shared resource folder to the demo project."

    from(rootProject.projectDir.parentFile.parentFile.resolve("resource"))

    into(rootProject.projectDir.parentFile.parentFile.parentFile.resolve("demo/faceu/android/Export/resource"))

    doFirst {
        destinationDir.mkdirs()
    }

    doLast {
        println(">>> Success: Copied resources to ${destinationDir.path}")
    }
}

// --- Task to export the .so libraries to Demo project ---
tasks.register("exportSoLibsToDemo", Copy::class) {
    group = "build"
    description = "Exports .so libraries to the demo project's jniLibs."

    dependsOn("bundleReleaseAar")

    val demoJniLibsDir = rootProject.projectDir.parentFile.parentFile.parentFile
        .resolve("demo/faceu/android/Export/app/src/main/jniLibs")

    from(zipTree(releaseAarFile)) {
        include("jni/**/libHJRender.so")
    }

    into(demoJniLibsDir)

    eachFile {
        path = relativePath.pathString.substringAfter("jni/")
    }

    doFirst {
        demoJniLibsDir.mkdirs()
        demoJniLibsDir.resolve("jni").deleteRecursively()
        fileTree(demoJniLibsDir) {
            include("**/*.so")
        }.files.forEach { soFile ->
            soFile.delete()
        }
    }

    doLast {
        println(">>> Success: Exported .so libs to ${destinationDir.path}")
    }
}

// --- NEW TASK: Export everything to local 'export' directory ---
tasks.register("exportToLocalDir", Copy::class) {
    group = "build"
    description = "Exports versioned AAR and selected SO libs to local 'export' directory within the project."

    dependsOn("packageLocalExportAar")

    // AAR Part
    from(layout.buildDirectory.file("tmp/exportToLocalDir/package/${publishAarName}"))

    // SO Part: export only libHJMediaPlayer.so
    from(zipTree(releaseAarFile)) {
        include("jni/**/libHJMediaPlayer.so")
        eachFile {
            path = "jniLibs/" + relativePath.pathString.substringAfter("jni/")
        }
        includeEmptyDirs = false
    }

    // Target directory: examples/android/HJMedia/export
    into(rootProject.projectDir.resolve("export"))

    doFirst {
        destinationDir.deleteRecursively()
        destinationDir.mkdirs()
    }

    doLast {
        println(">>> Success: All artifacts exported to ${destinationDir.path}")
    }
}

tasks.register("exportToLocalDirAndPublish") {
    group = "publishing"
    description = "Exports artifacts to the local 'export' directory and publishes the release AAR to the Maven repository."

    dependsOn("exportToLocalDir")
    dependsOn("publishReleasePublicationToMavenRepository")
}


// --- Hook custom tasks into the standard 'build' task ---
tasks.named("build").configure {
    dependsOn("exportJarToDemo")
    dependsOn("exportResourcesToDemo")
    dependsOn("exportSoLibsToDemo")
    dependsOn("exportToLocalDir")
    dependsOn("exportToLocalDirAndPublish")
}

//E:\code\git\hjmedia\examples\android\HJMedia> .\gradlew.bat :hjmediasdk:packageEnhanceOnlyAar
//dir:examples\android\HJMedia\hjmediasdk\build\tmp\exportEnhanceOnly\package