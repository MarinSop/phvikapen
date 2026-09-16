# Presentation layer: QML user interface and the C++ view models behind it.

find_package(spdlog CONFIG REQUIRED)

# Reusable QML module. It is a library rather than part of the executable so that Qt Quick Test
# can import it.
qt_add_library(phvikapen_ui STATIC)
add_library(PhvikaPen::ui ALIAS phvikapen_ui)

qt_add_qml_module(phvikapen_ui
    URI PhvikaPen.Ui
    VERSION 1.0
    # Tooling such as qmllint expects the output directory to mirror the module URI.
    OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/qml/PhvikaPen/Ui"
    SOURCES
        cpp/AppInfo.cpp
        cpp/AppInfo.hpp
        cpp/InkCanvasForeign.hpp
        cpp/NotebooksViewModel.cpp
        cpp/NotebooksViewModel.hpp
        cpp/ToolViewModel.hpp
    QML_FILES
        qml/components/InkToolBar.qml
        qml/components/NotebookPage.qml
)

# The generated QML type registrations include these headers by file name only.
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
    # Lets qmllint and the QML compiler resolve the PhvikaPen.Ui module.
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
    # Every executable keeps its own directory. On Windows the Qt project setup would otherwise
    # put them all in the build root, where the dependency copying of two targets that link at
    # the same time fights over the same DLL files.
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)

if(WIN32)
    # The Velopack library ships as a DLL that has to sit next to the executable.
    add_custom_command(TARGET phvikapen POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "$<TARGET_RUNTIME_DLLS:phvikapen>" "$<TARGET_FILE_DIR:phvikapen>"
        COMMAND_EXPAND_LISTS
    )
endif()

install(TARGETS phvikapen
    BUNDLE DESTINATION .
    RUNTIME DESTINATION .
)

if(WIN32)
    # Velopack ships as a DLL and has to travel with the executable into the installer.
    install(FILES "$<TARGET_FILE:Velopack::velopack>" DESTINATION .)
endif()
