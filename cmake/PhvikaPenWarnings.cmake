# Interface target carrying the project-wide warning policy. Every warning is an error.

add_library(phvikapen_warnings INTERFACE)

set(_phvikapen_msvc_warnings /W4 /WX /permissive- /utf-8)
set(_phvikapen_clang_warnings -Wall -Wextra -Wpedantic -Werror -Wconversion -Wshadow)

target_compile_options(phvikapen_warnings
    INTERFACE
        "$<$<CXX_COMPILER_ID:MSVC>:${_phvikapen_msvc_warnings}>"
        "$<$<CXX_COMPILER_ID:AppleClang,Clang,GNU>:${_phvikapen_clang_warnings}>"
)

unset(_phvikapen_msvc_warnings)
unset(_phvikapen_clang_warnings)
