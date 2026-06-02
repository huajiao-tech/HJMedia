pluginManagement {
    repositories {
        google {
            content {
                includeGroupByRegex("com\\.android.*")
                includeGroupByRegex("com\\.google.*")
                includeGroupByRegex("androidx.*")
            }
        }
        mavenCentral()
        gradlePluginPortal()
    }
}
dependencyResolutionManagement {
    repositoriesMode.set(RepositoriesMode.FAIL_ON_PROJECT_REPOS)
    repositories {
        google()
        mavenCentral()
    }

//    //home computer must open this
//    versionCatalogs {
//        create("libs") {
//            version("agp", "8.3.2")
//            version("kotlin", "2.0.21")
//            version("appcompat", "1.6.1")
//            version("core-ktx", "1.12.0")
//            version("material", "1.11.0")
//            version("activity", "1.8.2")  // 添加 activity 版本
//            version("constraintlayout", "2.1.4")  // 添加 constraintlayout 版本
//            version("junit", "4.13.2")
//            version("test-junit", "1.1.5")
//            version("espresso", "3.5.1")
//            // 插件定义
//            plugin("android-application", "com.android.application").versionRef("agp")
//            plugin("kotlin-android", "org.jetbrains.kotlin.android").versionRef("kotlin")
//
//            // 库依赖定义
//            library("core-ktx", "androidx.core", "core-ktx").versionRef("core-ktx")
//            library("appcompat", "androidx.appcompat", "appcompat").versionRef("appcompat")
//            library("material", "com.google.android.material", "material").versionRef("material")
//
//            library(
//                "activity",
//                "androidx.activity",
//                "activity"
//            ).versionRef("activity")  // 添加 activity
//            library(
//                "activity-ktx",
//                "androidx.activity",
//                "activity-ktx"
//            ).versionRef("activity")  // 如果需要 ktx 版本
//            // 测试库
//            library("junit", "junit", "junit").versionRef("junit")
//            library("test-junit", "androidx.test.ext", "junit").versionRef("test-junit")
//            library(
//                "espresso-core",
//                "androidx.test.espresso",
//                "espresso-core"
//            ).versionRef("espresso")
//            library("constraintlayout", "androidx.constraintlayout", "constraintlayout").versionRef(
//                "constraintlayout"
//            )  // 添加这行
//        }
//    }




}

rootProject.name = "ui"
include(":app")
include(":hjmediasdk")
