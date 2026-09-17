if(NOT DEFINED CORE_DIR)
    message(FATAL_ERROR "CORE_DIR is not set")
endif()

file(GLOB_RECURSE sources "${CORE_DIR}/*.hpp" "${CORE_DIR}/*.cpp" "${CORE_DIR}/*.in")
if(NOT sources)
    message(FATAL_ERROR "No sources found in ${CORE_DIR}")
endif()

set(violations "")
foreach(source IN LISTS sources)
    file(STRINGS "${source}" includes
        REGEX "^[ \t]*#[ \t]*include[ \t]*[<\"](Q[A-Za-z0-9_]*[>/.]|platform/|app/)")
    foreach(include IN LISTS includes)
        string(APPEND violations "\n  ${source}: ${include}")
    endforeach()
endforeach()

if(violations)
    message(FATAL_ERROR "core must not include Qt, platform or app headers:${violations}")
endif()
