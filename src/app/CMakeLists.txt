find_package(spdlog CONFIG REQUIRED)

set(icons_dir "${CMAKE_CURRENT_BINARY_DIR}/icons")
phvikapen_collect_icons("${icons_dir}" icon_files
    pointer
    move
    pencil
    highlight
    shape-square
    eraser
    undo
    redo
    trash
    copy
    paste
    plus
    import
    export
    cog
    chevron-left
    chevron-right
    zoom-in
    zoom-out
    expand
    x
    palette
    save
    help-circle
    chevron-up
    chevron-down
    minus
    square
    circle
)

set_source_files_properties(qml/Icons.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
set_source_files_properties(qml/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_library(phvikapen_ui STATIC)
add_library(PhvikaPen::ui ALIAS phvikapen_ui)

qt_add_qml_module(phvikapen_ui
    URI PhvikaPen.Ui
    VERSION 1.0
    OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/qml/PhvikaPen/Ui"
    SOURCES
        cpp/AppInfo.cpp
        cpp/AppInfo.hpp
        cpp/InkCanvasForeign.hpp
        cpp/NotebookViewModel.cpp
        cpp/NotebookViewModel.hpp
        cpp/NotebooksViewModel.cpp
        cpp/OutlineModels.cpp
        cpp/OutlineModels.hpp
        cpp/PageDefaults.cpp
        cpp/PageDefaults.hpp
        cpp/SettingsViewModel.cpp
        cpp/Shortcuts.cpp
        cpp/Shortcuts.hpp
        cpp/SettingsViewModel.hpp
        cpp/NotebooksViewModel.hpp
        cpp/Thumbnails.cpp
        cpp/Thumbnails.hpp
        cpp/ToolViewModel.cpp
        cpp/ToolViewModel.hpp
        cpp/UpdateViewModel.cpp
        cpp/UpdateViewModel.hpp
    QML_FILES
        qml/Icons.qml
        qml/Theme.qml
        qml/components/AboutDialog.qml
        qml/components/AppDialog.qml
        qml/components/AppActions.qml
        qml/components/AppMenuBar.qml
        qml/components/AppStatusBar.qml
        qml/components/ArrowButton.qml
        qml/components/EmptyState.qml
        qml/components/MenuCommand.qml
        qml/components/MessageBar.qml
        qml/components/NotebookTabs.qml
        qml/components/NewNotebookDialog.qml
        qml/components/NumberField.qml
        qml/components/OptionsBar.qml
        qml/components/PagePanel.qml
        qml/components/PagesPanel.qml
        qml/components/PageSetup.qml
        qml/components/PageSetupDialog.qml
        qml/components/HintsDialog.qml
        qml/components/PenSwatch.qml
        qml/components/QuickButton.qml
        qml/components/SettingsDialog.qml
        qml/components/SettingsPage.qml
        qml/components/ShapeButton.qml
        qml/components/ShortcutField.qml
        qml/components/ThemeCard.qml
        qml/components/ToolPalette.qml
        qml/components/ToolPaletteButton.qml
        qml/components/TrashDialog.qml
        qml/components/UpdateBar.qml
        qml/components/WidthField.qml
)


qt_add_resources(phvikapen_ui "phvikapen_brand"
    PREFIX "/brand"
    BASE "${CMAKE_CURRENT_SOURCE_DIR}/brand"
    FILES brand/logo-256.png
)

qt_add_resources(phvikapen_ui "phvikapen_icons"
    PREFIX "/icons"
    BASE "${icons_dir}"
    FILES ${icon_files}
)

target_include_directories(phvikapen_ui PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/cpp")

target_link_libraries(phvikapen_ui
    PUBLIC
        PhvikaPen::platform
        Qt6::Quick
        Qt6::Svg
    PRIVATE
        phvikapen_warnings
        phvikapen_sanitizers
)

qt_add_executable(phvikapen
    cpp/Logging.cpp
    cpp/Logging.hpp
    cpp/SystemMenus.hpp
    cpp/main.cpp
)

if(APPLE)
    target_sources(phvikapen PRIVATE cpp/SystemMenus.mm)
    target_link_libraries(phvikapen PRIVATE "-framework Foundation")
else()
    target_sources(phvikapen PRIVATE cpp/SystemMenus.cpp)
endif()

qt_add_qml_module(phvikapen
    URI PhvikaPen
    VERSION 1.0
    OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/qml/PhvikaPen"
    IMPORT_PATH "${PROJECT_BINARY_DIR}/qml"
    QML_FILES
        qml/Main.qml
)

target_link_libraries(phvikapen
    PRIVATE
        phvikapen_ui
        phvikapen_uiplugin
        Qt6::Quick
        Qt6::QuickControls2
        spdlog::spdlog
        phvikapen_warnings
        phvikapen_sanitizers
)

if(WIN32)
    target_sources(phvikapen PRIVATE brand/phvikapen.rc)
endif()

if(APPLE)
    # The bundle icon is built from the same picture the window and the about box show.
    set(iconset "${CMAKE_CURRENT_BINARY_DIR}/PhvikaPen.iconset")
    set(icns "${CMAKE_CURRENT_BINARY_DIR}/PhvikaPen.icns")
    add_custom_command(OUTPUT "${icns}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${iconset}"
        COMMAND sips -Z 16 "${CMAKE_CURRENT_SOURCE_DIR}/brand/logo.png" --out "${iconset}/icon_16x16.png"
        COMMAND sips -Z 32 "${CMAKE_CURRENT_SOURCE_DIR}/brand/logo.png" --out "${iconset}/icon_16x16@2x.png"
        COMMAND sips -Z 32 "${CMAKE_CURRENT_SOURCE_DIR}/brand/logo.png" --out "${iconset}/icon_32x32.png"
        COMMAND sips -Z 64 "${CMAKE_CURRENT_SOURCE_DIR}/brand/logo.png" --out "${iconset}/icon_32x32@2x.png"
        COMMAND sips -Z 128 "${CMAKE_CURRENT_SOURCE_DIR}/brand/logo.png" --out "${iconset}/icon_128x128.png"
        COMMAND sips -Z 256 "${CMAKE_CURRENT_SOURCE_DIR}/brand/logo.png" --out "${iconset}/icon_128x128@2x.png"
        COMMAND sips -Z 256 "${CMAKE_CURRENT_SOURCE_DIR}/brand/logo.png" --out "${iconset}/icon_256x256.png"
        COMMAND sips -Z 512 "${CMAKE_CURRENT_SOURCE_DIR}/brand/logo.png" --out "${iconset}/icon_256x256@2x.png"
        COMMAND sips -Z 512 "${CMAKE_CURRENT_SOURCE_DIR}/brand/logo.png" --out "${iconset}/icon_512x512.png"
        COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_CURRENT_SOURCE_DIR}/brand/logo.png" "${iconset}/icon_512x512@2x.png"
        COMMAND iconutil -c icns "${iconset}" -o "${icns}"
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/brand/logo.png"
        COMMENT "Building the application icon"
        VERBATIM
    )
    target_sources(phvikapen PRIVATE "${icns}")
    set_source_files_properties("${icns}" PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
endif()

set_target_properties(phvikapen PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_GUI_IDENTIFIER app.phvikapen.PhvikaPen
    MACOSX_BUNDLE_BUNDLE_NAME PhvikaPen
    MACOSX_BUNDLE_ICON_FILE PhvikaPen.icns
    # Own directory: on Windows, shared output makes parallel DLL copying collide.
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)

phvikapen_copy_runtime_dlls(phvikapen)

install(TARGETS phvikapen
    BUNDLE DESTINATION .
    RUNTIME DESTINATION .
)

if(WIN32)
    install(FILES "$<TARGET_FILE:Velopack::velopack>" DESTINATION .)
    install(FILES "$<TARGET_FILE:PDFium::pdfium>" DESTINATION .)
endif()
