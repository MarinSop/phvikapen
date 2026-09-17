include_guard(GLOBAL)

# Windows has no rpath: every DLL a program loads must sit next to it.
function(phvikapen_copy_runtime_dlls target)
    if(NOT WIN32)
        return()
    endif()

    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            -t "$<TARGET_FILE_DIR:${target}>" "$<TARGET_RUNTIME_DLLS:${target}>"
        COMMAND_EXPAND_LISTS
        VERBATIM
    )
endfunction()
