find_package(spdlog CONFIG REQUIRED)

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
        cpp/SettingsViewModel.cpp
        cpp/SettingsViewModel.hpp
        cpp/NotebooksViewModel.hpp
        cpp/ToolViewModel.cpp
        cpp/ToolViewModel.hpp
        cpp/UpdateViewModel.cpp
        cpp/UpdateViewModel.hpp
    QML_FILES
        qml/components/InkToolBar.qml
        qml/components/MessageBar.qml
        qml/components/NotebookTabs.qml
        qml/components/PenSwatch.qml
        qml/components/OutlineSidebar.qml
        qml/components/PageSetup.qml
        qml/components/SettingsDialog.qml
        qml/components/UpdateBar.qml
        qml/components/NotebookPage.qml
)

target_include_directories(phvikapen_ui PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/cpp")

target_link_libraries(phvikapen_ui
    PUBLIC
        PhvikaPen::platform
        Qt6::Quick
    PRIVATE
        phvikapen_warnings
        phvikapen_sanitizers
)

qt_add_executable(phvikapen
    cpp/Logging.cpp
    cpp/Logging.hpp
    cpp/main.cpp
)

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

set_target_properties(phvikapen PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
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
