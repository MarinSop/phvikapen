qt_add_executable(ink_recorder
    cpp/RecorderItem.cpp
    cpp/RecorderItem.hpp
    cpp/main.cpp
)

qt_add_qml_module(ink_recorder
    URI InkRecorder
    VERSION 1.0
    OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/qml/InkRecorder"
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

phvikapen_quiet_generated_qml(ink_recorder)

set_target_properties(ink_recorder PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)

phvikapen_copy_runtime_dlls(ink_recorder)
