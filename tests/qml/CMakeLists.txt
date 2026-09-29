qt_add_executable(phvikapen_qml_tests tst_main.cpp)

target_link_libraries(phvikapen_qml_tests
    PRIVATE
        phvikapen_ui
        phvikapen_uiplugin
        Qt6::Quick
        Qt6::QuickControls2
        Qt6::QuickTest
        Qt6::Gui
        phvikapen_warnings
        phvikapen_sanitizers
)

target_compile_definitions(phvikapen_qml_tests
    PRIVATE QUICK_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

set_target_properties(phvikapen_qml_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)

phvikapen_copy_runtime_dlls(phvikapen_qml_tests)

add_test(NAME qml.components COMMAND phvikapen_qml_tests)

set_tests_properties(qml.components PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software"
)

# Taking a colour reads back what the canvas drew, which a window without a screen behind it
# cannot hand over. This runs on whatever window system is there and says so when there is none.
add_test(NAME qml.onscreen
    COMMAND phvikapen_qml_tests -input "${CMAKE_CURRENT_SOURCE_DIR}/tst_colourpicker.qml"
)
