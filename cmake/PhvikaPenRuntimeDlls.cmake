include_guard(GLOBAL)

# Windows has no rpath: every DLL a program loads must sit next to it. A static library keeps its
# own dependencies to itself, and those never reach TARGET_RUNTIME_DLLS, so they are named here.
set(PHVIKAPEN_PRIVATE_RUNTIME_LIBRARIES
    unofficial::sqlite3::sqlite3
    PDFium::pdfium
    Velopack::velopack
)

function(_phvikapen_hidden_runtime_dlls output)
    set(hidden "")
    foreach(library IN LISTS PHVIKAPEN_PRIVATE_RUNTIME_LIBRARIES)
        if(TARGET ${library})
            get_target_property(kind ${library} TYPE)
            if(kind STREQUAL "SHARED_LIBRARY")
                list(APPEND hidden "$<TARGET_FILE:${library}>")
            endif()
        endif()
    endforeach()
    set(${output} "${hidden}" PARENT_SCOPE)
endfunction()

function(phvikapen_copy_runtime_dlls target)
    if(NOT WIN32)
        return()
    endif()

    _phvikapen_hidden_runtime_dlls(hidden)
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            -t "$<TARGET_FILE_DIR:${target}>" "$<TARGET_RUNTIME_DLLS:${target}>" ${hidden}
        COMMAND_EXPAND_LISTS
        VERBATIM
    )
endfunction()

function(phvikapen_install_runtime_dlls target)
    if(NOT WIN32)
        return()
    endif()

    _phvikapen_hidden_runtime_dlls(hidden)
    install(FILES $<TARGET_RUNTIME_DLLS:${target}> ${hidden} DESTINATION .)
endfunction()
