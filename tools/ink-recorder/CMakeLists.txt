# Records raw pen and mouse input to CSV, for tuning filters on real data from the target device.

qt_add_executable(ink_recorder
    cpp/RecorderItem.cpp
    cpp/RecorderItem.hpp
    cpp/main.cpp
)

qt_add_qml_module(ink_recorder
    URI InkRecorder
    VERSION 1.0
    OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/qml/InkRecorder"
    # Lets qmllint and the QML compiler resolve the module's own URI.
    IMPORT_PATH "${PROJECT_BINARY_DIR}/qml"
    QML_FILES
        qml/Main.qml
)

target_include_directories(ink_recorder PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/cpp")

target_link_libraries(ink_recorder
    PRIVATE
        Qt6::Quick
        Qt6::QuickControls2
        phvikapen_warnings
        phvikapen_sanitizers
)

set_target_properties(ink_recorder PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
    # Its own directory, so that dependency copying does not collide with the other executables.
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)
