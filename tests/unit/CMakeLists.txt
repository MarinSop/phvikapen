find_package(GTest CONFIG REQUIRED)

add_executable(phvikapen_tests
    core/ErrorTest.cpp
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

set_target_properties(phvikapen_tests PROPERTIES AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)

include(GoogleTest)
gtest_discover_tests(phvikapen_tests DISCOVERY_MODE PRE_TEST)
