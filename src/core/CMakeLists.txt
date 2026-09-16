# Pure C++23 domain layer. It must never depend on Qt.

add_library(phvikapen_core STATIC)
add_library(PhvikaPen::core ALIAS phvikapen_core)

set(generated_dir "${CMAKE_CURRENT_BINARY_DIR}/generated")
configure_file(version.hpp.in "${generated_dir}/core/version.hpp" @ONLY)

target_sources(phvikapen_core
    PRIVATE
        Error.cpp
        id/Uuid.cpp
        id/Uuid7Generator.cpp
        ink/Stroke.cpp
        ink/StrokeMesh.cpp
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS
            "${PROJECT_SOURCE_DIR}/src"
            "${generated_dir}"
        FILES
            Error.hpp
            geometry/Rect.hpp
            id/Uuid.hpp
            id/Uuid7Generator.hpp
            ink/InkSample.hpp
            ink/Stroke.hpp
            ink/StrokeMesh.hpp
            "${generated_dir}/core/version.hpp"
)

target_compile_features(phvikapen_core PUBLIC cxx_std_23)
target_link_libraries(phvikapen_core PRIVATE phvikapen_warnings phvikapen_sanitizers)

# qt_standard_project_setup() enables the Qt code generators for every target; core has no Qt code.
set_target_properties(phvikapen_core PROPERTIES AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)

phvikapen_forbid_qt_dependency(phvikapen_core)
