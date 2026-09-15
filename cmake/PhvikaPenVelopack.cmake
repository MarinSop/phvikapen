# Velopack client library (installer hooks and updates), downloaded as a prebuilt release
# archive pinned by version and SHA-256 into the build tree. Defines Velopack::velopack.

include_guard(GLOBAL)
include(FetchContent)

set(PHVIKAPEN_VELOPACK_VERSION 1.2.0)

FetchContent_Declare(velopack_libc
    URL "https://github.com/velopack/velopack/releases/download/${PHVIKAPEN_VELOPACK_VERSION}/velopack_libc_${PHVIKAPEN_VELOPACK_VERSION}.zip"
    URL_HASH SHA256=547262ed7a1ab1ff62f580aa53851ede2f1a451ac61b8974eb7bc01117488835
)
FetchContent_MakeAvailable(velopack_libc)

string(TOLOWER "${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}" _phvikapen_velopack_arch)
if(_phvikapen_velopack_arch MATCHES "arm64|aarch64")
    set(_phvikapen_velopack_arch arm64)
elseif(_phvikapen_velopack_arch MATCHES "x64|x86_64|amd64")
    set(_phvikapen_velopack_arch x64)
else()
    message(FATAL_ERROR "Unsupported architecture for Velopack: ${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}")
endif()

set(_phvikapen_velopack_dir "${velopack_libc_SOURCE_DIR}")

if(WIN32)
    # Shipped next to the executable, as the Velopack C++ documentation recommends.
    set(_phvikapen_velopack_name "velopack_libc_win_${_phvikapen_velopack_arch}_msvc")
    add_library(Velopack::velopack SHARED IMPORTED GLOBAL)
    set_target_properties(Velopack::velopack PROPERTIES
        IMPORTED_LOCATION "${_phvikapen_velopack_dir}/lib/${_phvikapen_velopack_name}.dll"
        IMPORTED_IMPLIB "${_phvikapen_velopack_dir}/lib/${_phvikapen_velopack_name}.dll.lib"
    )
elseif(APPLE)
    # macOS is a development platform only. The static archive avoids the absolute install name
    # embedded in the prebuilt dylib.
    add_library(Velopack::velopack STATIC IMPORTED GLOBAL)
    set_target_properties(Velopack::velopack PROPERTIES
        IMPORTED_LOCATION
            "${_phvikapen_velopack_dir}/lib-static/velopack_libc_osx_${_phvikapen_velopack_arch}_gnu.a"
        INTERFACE_LINK_LIBRARIES iconv
    )
else()
    message(FATAL_ERROR "Velopack integration supports Windows and macOS only")
endif()

set_target_properties(Velopack::velopack PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${_phvikapen_velopack_dir}/include"
)

unset(_phvikapen_velopack_arch)
unset(_phvikapen_velopack_dir)
unset(_phvikapen_velopack_name)
