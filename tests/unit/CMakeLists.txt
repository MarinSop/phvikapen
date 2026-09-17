find_package(GTest CONFIG REQUIRED)

# The notebook test forges a file from a future schema version, which needs the SQLite header.
# The library itself arrives through the core library, so linking it again here would put the
# same archive on the link line twice.
find_package(unofficial-sqlite3 CONFIG REQUIRED)

add_executable(phvikapen_tests
    core/ErrorTest.cpp
    core/NotebookStoreTest.cpp
    core/OneEuroFilterTest.cpp
    core/StrokeCodecTest.cpp
    core/StrokeMeshTest.cpp
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
    PRIVATE $<TARGET_PROPERTY:unofficial::sqlite3::sqlite3,INTERFACE_INCLUDE_DIRECTORIES>
)

set_target_properties(phvikapen_tests PROPERTIES
    AUTOMOC OFF
    AUTOUIC OFF
    AUTORCC OFF
    # Its own directory, so that dependency copying does not collide with the other executables.
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)

include(GoogleTest)
gtest_discover_tests(phvikapen_tests DISCOVERY_MODE PRE_TEST)
