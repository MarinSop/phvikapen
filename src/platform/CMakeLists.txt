include(PhvikaPenVelopack)

add_library(phvikapen_platform STATIC)
add_library(PhvikaPen::platform ALIAS phvikapen_platform)

set(generated_dir "${CMAKE_CURRENT_BINARY_DIR}/generated")
configure_file(update/Feed.hpp.in "${generated_dir}/platform/update/Feed.hpp" @ONLY)

target_sources(phvikapen_platform
    PRIVATE
        audio/QtSound.cpp
        ink/qt/QtInkItem.cpp
        ink/qt/QtInkRenderer.cpp
        ink/qt/QtInkRenderer.hpp
        pdf/PdfRenderer.cpp
        pdf/PdfiumDocument.cpp
        render/PagePainter.cpp
        speech/NoSpeech.cpp
        render/PdfExporter.cpp
        update/StartupHook.cpp
        update/VelopackUpdater.cpp
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS
            "${PROJECT_SOURCE_DIR}/src"
            "${generated_dir}"
        FILES
            audio/ISound.hpp
            audio/QtSound.hpp
            ink/IInkBackend.hpp
            ink/qt/QtInkItem.hpp
            pdf/IPdfDocument.hpp
            pdf/PdfRenderer.hpp
            pdf/PdfiumDocument.hpp
            render/PagePainter.hpp
            render/PdfExporter.hpp
            render/PaperLook.hpp
            ocr/IReadPicture.hpp
            speech/ISpeech.hpp
            text/IHandwriting.hpp
            update/IUpdater.hpp
            update/StartupHook.hpp
            update/VelopackUpdater.hpp
            "${generated_dir}/platform/update/Feed.hpp"
)

qt_add_shaders(phvikapen_platform "phvikapen_ink_shaders"
    PREFIX "/phvikapen"
    FILES
        ink/qt/shaders/background.vert
        ink/qt/shaders/background.frag
        ink/qt/shaders/ink.vert
        ink/qt/shaders/layer.vert
        ink/qt/shaders/layer.frag
        ink/qt/shaders/media.vert
        ink/qt/shaders/media.frag
        ink/qt/shaders/ink.frag
)

if(WIN32)
    find_package(cppwinrt CONFIG REQUIRED)
    target_sources(phvikapen_platform
        PRIVATE
            ink/win/WinInkBackend.cpp
            text/win/WindowsHandwriting.cpp
        PUBLIC
            FILE_SET HEADERS
            FILES
                ink/win/WinInkBackend.hpp
                text/win/WindowsHandwriting.hpp
    )
    target_link_libraries(phvikapen_platform PRIVATE Microsoft::CppWinRT)
else()
    target_sources(phvikapen_platform PRIVATE text/NoHandwriting.cpp)
endif()

# The words in a picture are read by whatever the machine carries. macOS carries one; everywhere
# else the application says it cannot read any.
if(APPLE)
    target_sources(phvikapen_platform PRIVATE ocr/AppleReadPicture.mm)
    set_source_files_properties(ocr/AppleReadPicture.mm PROPERTIES LANGUAGE OBJCXX)
    target_link_libraries(phvikapen_platform PRIVATE
        "-framework Vision" "-framework CoreGraphics" "-framework ImageIO"
        "-framework Foundation")
else()
    target_sources(phvikapen_platform PRIVATE ocr/NoReadPicture.cpp)
endif()

target_link_libraries(phvikapen_platform
    PUBLIC
        PhvikaPen::core
        Qt6::Multimedia
        Qt6::Quick
    PRIVATE
        Qt6::GuiPrivate
        Velopack::velopack
        PDFium::pdfium
        phvikapen_warnings
        phvikapen_sanitizers
)

target_compile_features(phvikapen_platform PUBLIC cxx_std_23)

qt_extract_metatypes(phvikapen_platform)
