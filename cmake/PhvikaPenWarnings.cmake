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

# Code generated from QML inlines Qt headers where MSVC reports unreachable code, and the warnings
# its code generator raises are out of reach of /external.
function(phvikapen_quiet_generated_qml target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /wd4702)
    endif()
endfunction()
