# Interface target that instruments its consumers with AddressSanitizer and
# UndefinedBehaviorSanitizer when PHVIKAPEN_ENABLE_SANITIZERS is ON.

add_library(phvikapen_sanitizers INTERFACE)

if(NOT PHVIKAPEN_ENABLE_SANITIZERS)
    return()
endif()

if(NOT CMAKE_CXX_COMPILER_ID MATCHES "^(AppleClang|Clang|GNU)$")
    message(FATAL_ERROR "PHVIKAPEN_ENABLE_SANITIZERS requires Clang or GCC, "
        "but the compiler is ${CMAKE_CXX_COMPILER_ID}.")
endif()

target_compile_options(phvikapen_sanitizers
    INTERFACE
        -fsanitize=address,undefined
        -fno-sanitize-recover=all
        -fno-omit-frame-pointer
)
target_link_options(phvikapen_sanitizers INTERFACE -fsanitize=address,undefined)
