# Uses sccache as the compiler launcher when it is installed and no launcher was chosen explicitly.

if(NOT PHVIKAPEN_USE_SCCACHE OR DEFINED CMAKE_CXX_COMPILER_LAUNCHER)
    return()
endif()

find_program(PHVIKAPEN_SCCACHE_PROGRAM NAMES sccache)
if(NOT PHVIKAPEN_SCCACHE_PROGRAM)
    return()
endif()

message(STATUS "Using sccache: ${PHVIKAPEN_SCCACHE_PROGRAM}")
set(CMAKE_CXX_COMPILER_LAUNCHER "${PHVIKAPEN_SCCACHE_PROGRAM}")

# sccache cannot cache MSVC objects that write debug info into a shared PDB (/Zi),
# so embed the debug info in the object files instead (/Z7).
set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug,RelWithDebInfo>:Embedded>")
