find_package(GTest CONFIG REQUIRED)

add_executable(phvikapen_tests
    core/ErrorTest.cpp
    core/StrokeMeshTest.cpp
    core/StrokeTest.cpp
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

set_target_properties(phvikapen_tests PROPERTIES
    AUTOMOC OFF
    AUTOUIC OFF
    AUTORCC OFF
    # Its own directory, so that dependency copying does not collide with the other executables.
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)

include(GoogleTest)
gtest_discover_tests(phvikapen_tests DISCOVERY_MODE PRE_TEST)
