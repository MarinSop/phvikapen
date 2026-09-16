# Operating-system specific services behind interfaces: ink input, PDF documents and updates.

include(PhvikaPenVelopack)

# QRhi is a semi-public Qt API with its own compatibility promise; its headers ship with the
# GuiPrivate component, so the general warning about private modules does not apply here.
set(QT_NO_PRIVATE_MODULE_WARNING ON)
find_package(Qt6 6.11 REQUIRED COMPONENTS GuiPrivate)

add_library(phvikapen_platform STATIC)
add_library(PhvikaPen::platform ALIAS phvikapen_platform)

target_sources(phvikapen_platform
    PRIVATE
        ink/qt/QtInkItem.cpp
        update/StartupHook.cpp
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
        FILES
            ink/IInkBackend.hpp
            ink/qt/QtInkItem.hpp
            pdf/IPdfDocument.hpp
            update/IUpdater.hpp
            update/StartupHook.hpp
)

qt_add_shaders(phvikapen_platform "phvikapen_ink_shaders"
    PREFIX "/phvikapen"
    FILES
        ink/qt/shaders/ink.vert
        ink/qt/shaders/ink.frag
)

if(WIN32)
    # C++/WinRT projection for the native low-latency Windows ink backend.
    find_package(cppwinrt CONFIG REQUIRED)
    target_sources(phvikapen_platform
        PRIVATE
            ink/win/WinInkBackend.cpp
        PUBLIC
            FILE_SET HEADERS
            FILES
                ink/win/WinInkBackend.hpp
    )
    target_link_libraries(phvikapen_platform PRIVATE Microsoft::CppWinRT)
endif()

target_link_libraries(phvikapen_platform
    PUBLIC
        PhvikaPen::core
        Qt6::Quick
    PRIVATE
        Qt6::GuiPrivate
        Velopack::velopack
        phvikapen_warnings
        phvikapen_sanitizers
)

target_compile_features(phvikapen_platform PUBLIC cxx_std_23)
