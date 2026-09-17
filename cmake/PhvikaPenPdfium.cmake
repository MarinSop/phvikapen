include_guard(GLOBAL)
include(FetchContent)

set(PHVIKAPEN_PDFIUM_VERSION "chromium/8057")

if(WIN32)
    set(_phvikapen_pdfium_platform "win-arm64")
    set(_phvikapen_pdfium_hash
        30070ce592b86da476601d07ca2742b26c4761095d4068121465088fe8979996)
elseif(APPLE)
    set(_phvikapen_pdfium_platform "mac-arm64")
    set(_phvikapen_pdfium_hash
        013ecc9e0a155dabb8dd006a73a028ea542965f80a4990e5f013de283b0e58a6)
else()
    message(FATAL_ERROR "PDFium integration supports Windows on ARM and macOS on Apple silicon")
endif()

string(REPLACE "/" "%2F" _phvikapen_pdfium_tag "${PHVIKAPEN_PDFIUM_VERSION}")

FetchContent_Declare(pdfium_binaries
    URL "https://github.com/bblanchon/pdfium-binaries/releases/download/${_phvikapen_pdfium_tag}/pdfium-${_phvikapen_pdfium_platform}.tgz"
    URL_HASH SHA256=${_phvikapen_pdfium_hash}
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(pdfium_binaries)

if(APPLE)
    set(_phvikapen_pdfium_library "${pdfium_binaries_SOURCE_DIR}/lib/libpdfium.dylib")
    set(_phvikapen_pdfium_stamp "${pdfium_binaries_SOURCE_DIR}/lib/.install_name_fixed")
    if(NOT EXISTS "${_phvikapen_pdfium_stamp}")
        execute_process(
            COMMAND install_name_tool -id "@rpath/libpdfium.dylib" "${_phvikapen_pdfium_library}"
            COMMAND_ERROR_IS_FATAL ANY
        )
        file(TOUCH "${_phvikapen_pdfium_stamp}")
    endif()
    unset(_phvikapen_pdfium_library)
    unset(_phvikapen_pdfium_stamp)
endif()

find_package(PDFium REQUIRED
    PATHS "${pdfium_binaries_SOURCE_DIR}"
    NO_DEFAULT_PATH
)
add_library(PDFium::pdfium ALIAS pdfium)

set(PHVIKAPEN_PDFIUM_DIR "${pdfium_binaries_SOURCE_DIR}" CACHE INTERNAL "PDFium package directory")

unset(_phvikapen_pdfium_platform)
unset(_phvikapen_pdfium_hash)
unset(_phvikapen_pdfium_tag)
