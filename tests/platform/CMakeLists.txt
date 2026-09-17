find_package(GTest CONFIG REQUIRED)

qt_add_executable(phvikapen_platform_tests
    main.cpp
    PdfRendererTest.cpp
    PdfiumDocumentTest.cpp
)

target_link_libraries(phvikapen_platform_tests
    PRIVATE
        PhvikaPen::platform
        Qt6::Gui
        GTest::gtest
        phvikapen_warnings
        phvikapen_sanitizers
)

set_target_properties(phvikapen_platform_tests PROPERTIES
    AUTOMOC OFF
    AUTOUIC OFF
    AUTORCC OFF
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)

if(WIN32)
    add_custom_command(TARGET phvikapen_platform_tests POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "$<TARGET_FILE:PDFium::pdfium>" "$<TARGET_FILE_DIR:phvikapen_platform_tests>"
        VERBATIM
    )
endif()

include(GoogleTest)
gtest_discover_tests(phvikapen_platform_tests
    DISCOVERY_MODE PRE_TEST
    PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)
