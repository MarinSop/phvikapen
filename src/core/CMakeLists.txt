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
        geometry/Distance.cpp
        id/Uuid.cpp
        id/Uuid7Generator.cpp
        ink/Stroke.cpp
        ink/StrokeHitTest.cpp
        ink/StrokeMesh.cpp
        ink/StrokeSpline.cpp
        model/Outline.cpp
        model/Page.cpp
        model/PageStyle.cpp
        model/StrokeGrid.cpp
        storage/NotebookStore.cpp
        storage/Sqlite.cpp
        storage/Sqlite.hpp
        storage/StorageThread.cpp
        storage/StrokeCodec.cpp
        undo/OutlineCommands.cpp
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
            geometry/Distance.hpp
            geometry/Rect.hpp
            id/Uuid.hpp
            id/Uuid7Generator.hpp
            ink/InkSample.hpp
            ink/Stroke.hpp
            ink/StrokeHitTest.hpp
            ink/StrokeMesh.hpp
            ink/StrokeSpline.hpp
            model/Outline.hpp
            model/Page.hpp
            model/PageStyle.hpp
            model/StrokeGrid.hpp
            storage/NotebookStore.hpp
            storage/StorageThread.hpp
            storage/StrokeCodec.hpp
            undo/OutlineCommands.hpp
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

set_target_properties(phvikapen_core PROPERTIES AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)

phvikapen_forbid_qt_dependency(phvikapen_core)
