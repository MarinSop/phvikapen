cmake_minimum_required(VERSION 4.4)

project(PhvikaPen
    VERSION 0.1.0
    DESCRIPTION "Pen-first note-taking application"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(PHVIKAPEN_BUILD_TESTS "Build the unit and QML tests" ON)
option(PHVIKAPEN_ENABLE_SANITIZERS "Instrument targets with AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
option(PHVIKAPEN_USE_SCCACHE "Use sccache as the compiler launcher when it is available" ON)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
include(PhvikaPenSccache)
include(PhvikaPenWarnings)
include(PhvikaPenSanitizers)
include(PhvikaPenLayering)

find_package(Qt6 6.11 REQUIRED COMPONENTS Core Gui Quick QuickControls2 QuickTest ShaderTools)
qt_standard_project_setup(REQUIRES 6.11)

add_subdirectory(src)

if(PHVIKAPEN_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
