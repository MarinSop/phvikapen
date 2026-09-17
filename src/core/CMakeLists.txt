# Pure C++23 domain layer. It must never depend on Qt.

find_package(unofficial-sqlite3 CONFIG REQUIRED)

add_library(phvikapen_core STATIC)
add_library(PhvikaPen::core ALIAS phvikapen_core)

set(generated_dir "${CMAKE_CURRENT_BINARY_DIR}/generated")
configure_file(version.hpp.in "${generated_dir}/core/version.hpp" @ONLY)

target_sources(phvikapen_core
    PRIVATE
        Error.cpp
        filter/InkFilter.cpp
        filter/OneEuroFilter.cpp
        id/Uuid.cpp
        id/Uuid7Generator.cpp
        ink/Stroke.cpp
        ink/StrokeMesh.cpp
        storage/NotebookStore.cpp
        storage/StrokeCodec.cpp
        undo/StrokeCommands.cpp
        undo/UndoStack.cpp
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS
            "${PROJECT_SOURCE_DIR}/src"
            "${generated_dir}"
        FILES
            Error.hpp
            filter/InkFilter.hpp
            filter/OneEuroFilter.hpp
            geometry/Rect.hpp
            id/Uuid.hpp
            id/Uuid7Generator.hpp
            ink/InkSample.hpp
            ink/Stroke.hpp
            ink/StrokeMesh.hpp
            storage/NotebookStore.hpp
            storage/StrokeCodec.hpp
            undo/StrokeCommands.hpp
            undo/UndoStack.hpp
            "${generated_dir}/core/version.hpp"
)

target_compile_features(phvikapen_core PUBLIC cxx_std_23)
target_link_libraries(phvikapen_core
    PRIVATE
        unofficial::sqlite3::sqlite3
        phvikapen_warnings
        phvikapen_sanitizers
)

# qt_standard_project_setup() enables the Qt code generators for every target; core has no Qt code.
set_target_properties(phvikapen_core PROPERTIES AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)

phvikapen_forbid_qt_dependency(phvikapen_core)
