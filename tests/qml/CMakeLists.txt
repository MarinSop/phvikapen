qt_add_executable(phvikapen_qml_tests tst_main.cpp)

target_link_libraries(phvikapen_qml_tests
    PRIVATE
        phvikapen_ui
        phvikapen_uiplugin
        Qt6::Quick
        Qt6::QuickTest
        phvikapen_warnings
        phvikapen_sanitizers
)

target_compile_definitions(phvikapen_qml_tests
    PRIVATE QUICK_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

set_target_properties(phvikapen_qml_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)

add_test(NAME qml.components COMMAND phvikapen_qml_tests)

set_tests_properties(qml.components PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software"
)
