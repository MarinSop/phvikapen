find_package(GTest CONFIG REQUIRED)

find_package(unofficial-sqlite3 CONFIG REQUIRED)

add_executable(phvikapen_tests
    core/DistanceTest.cpp
    core/ErrorTest.cpp
    core/NotebookStoreTest.cpp
    core/OneEuroFilterTest.cpp
    core/PageTest.cpp
    core/StrokeCodecTest.cpp
    core/StorageThreadTest.cpp
    core/StrokeCommandsTest.cpp
    core/StrokeGridTest.cpp
    core/StrokeHitTestTest.cpp
    core/StrokeMeshTest.cpp
    core/StrokeSplineTest.cpp
    core/StrokeTest.cpp
    core/UndoStackTest.cpp
    core/UuidTest.cpp
    core/VersionTest.cpp
)

target_link_libraries(phvikapen_tests
    PRIVATE
        PhvikaPen::core
        GTest::gtest_main
        phvikapen_warnings
        phvikapen_sanitizers
)

target_include_directories(phvikapen_tests
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        $<TARGET_PROPERTY:unofficial::sqlite3::sqlite3,INTERFACE_INCLUDE_DIRECTORIES>
)

set_target_properties(phvikapen_tests PROPERTIES
    AUTOMOC OFF
    AUTOUIC OFF
    AUTORCC OFF
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)

include(GoogleTest)
gtest_discover_tests(phvikapen_tests DISCOVERY_MODE PRE_TEST)
