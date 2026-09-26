find_package(unofficial-sqlite3 CONFIG REQUIRED GLOBAL)

add_library(phvikapen_core STATIC)
add_library(PhvikaPen::core ALIAS phvikapen_core)

set(generated_dir "${CMAKE_CURRENT_BINARY_DIR}/generated")
configure_file(version.hpp.in "${generated_dir}/core/version.hpp" @ONLY)

target_sources(phvikapen_core
    PRIVATE
        Error.cpp
        geometry/Distance.cpp
        geometry/Viewport.cpp
        id/ContentId.cpp
        id/Uuid.cpp
        id/Uuid7Generator.cpp
        ink/Stroke.cpp
        ink/StrokeEraser.cpp
        ink/StrokeHitTest.cpp
        ink/StrokeMesh.cpp
        ink/StrokeOutline.cpp
        ink/StrokeSelection.cpp
        ink/StrokeShapes.cpp
        ink/StrokeSpline.cpp
        ink/StrokeSteadier.cpp
        ink/StrokeTransform.cpp
        ink/StrokeTurns.cpp
        math/Answer.cpp
        math/Drawing.cpp
        math/Equation.cpp
        math/Plotting.cpp
        math/Reading.cpp
        math/Solving.cpp
        model/Layer.cpp
        model/Outline.cpp
        model/Page.cpp
        model/PageStyle.cpp
        model/Picture.cpp
        model/Recording.cpp
        model/StrokeGrid.cpp
        model/Table.cpp
        model/TextBox.cpp
        storage/NotebookStore.cpp
        storage/Sqlite.cpp
        storage/Sqlite.hpp
        storage/StorageThread.cpp
        storage/StrokeCodec.cpp
        text/Folding.cpp
        text/WrittenText.cpp
        undo/BundleCommand.cpp
        undo/LayerCommands.cpp
        undo/OutlineCommands.cpp
        undo/PictureCommands.cpp
        undo/StrokeCommands.cpp
        undo/TableCommands.cpp
        undo/TextCommands.cpp
        undo/UndoStack.cpp
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS
            "${PROJECT_SOURCE_DIR}/src"
            "${generated_dir}"
        FILES
            Error.hpp
            geometry/Distance.hpp
            geometry/Rect.hpp
            geometry/Viewport.hpp
            id/ContentId.hpp
            id/Uuid.hpp
            id/Uuid7Generator.hpp
            ink/InkSample.hpp
            ink/Stroke.hpp
            ink/StrokeEraser.hpp
            ink/StrokeHitTest.hpp
            ink/StrokeMesh.hpp
            ink/StrokeOutline.hpp
            ink/StrokeSelection.hpp
            ink/StrokeShapes.hpp
            ink/StrokeSpline.hpp
            ink/StrokeSteadier.hpp
            ink/StrokeTransform.hpp
            ink/StrokeTurns.hpp
            math/Answer.hpp
            math/Drawing.hpp
            math/Equation.hpp
            math/Plotting.hpp
            math/Reading.hpp
            math/Solving.hpp
            model/Asset.hpp
            model/Layer.hpp
            model/Outline.hpp
            model/Page.hpp
            model/PageStyle.hpp
            model/Picture.hpp
            model/Recording.hpp
            model/StrokeGrid.hpp
            model/Table.hpp
            model/TextBox.hpp
            storage/NotebookStore.hpp
            storage/StorageThread.hpp
            storage/StrokeCodec.hpp
            text/Folding.hpp
            text/InkWord.hpp
            text/WrittenText.hpp
            undo/BundleCommand.hpp
            undo/LayerCommands.hpp
            undo/OutlineCommands.hpp
            undo/PictureCommands.hpp
            undo/StrokeCommands.hpp
            undo/TableCommands.hpp
            undo/TextCommands.hpp
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
