include_guard(GLOBAL)
include(FetchContent)

set(PHVIKAPEN_BOXICONS_VERSION 2.1.4)

FetchContent_Declare(boxicons
    URL "https://registry.npmjs.org/boxicons/-/boxicons-${PHVIKAPEN_BOXICONS_VERSION}.tgz"
    URL_HASH SHA256=f8e67523cad3e1e937d32fe408d83ab460b469d64cc90fb31a5b517a30b67eb2
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(boxicons)

# Copies the named outline icons into <destination>, and returns their paths in <out_files>.
function(phvikapen_collect_icons destination out_files)
    set(collected "")
    foreach(name IN LISTS ARGN)
        set(source "${boxicons_SOURCE_DIR}/svg/regular/bx-${name}.svg")
        if(NOT EXISTS "${source}")
            message(FATAL_ERROR "Boxicons has no icon called bx-${name}")
        endif()
        set(target "${destination}/${name}.svg")
        configure_file("${source}" "${target}" COPYONLY)
        list(APPEND collected "${target}")
    endforeach()
    configure_file("${boxicons_SOURCE_DIR}/LICENSE" "${destination}/LICENSE.txt" COPYONLY)
    set(${out_files} "${collected}" PARENT_SCOPE)
endfunction()
