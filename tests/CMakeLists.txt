add_subdirectory(unit)
add_subdirectory(platform)
add_subdirectory(qml)

add_test(NAME architecture.core_is_qt_free
    COMMAND
        "${CMAKE_COMMAND}"
        -D "CORE_DIR=${PROJECT_SOURCE_DIR}/src/core"
        -P "${PROJECT_SOURCE_DIR}/cmake/CheckCoreIncludes.cmake"
)
