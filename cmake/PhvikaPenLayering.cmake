function(phvikapen_forbid_qt_dependency target)
    cmake_language(EVAL CODE "
        cmake_language(DEFER DIRECTORY [[${PROJECT_SOURCE_DIR}]]
            CALL _phvikapen_check_no_qt_dependency [[${target}]])
    ")
endfunction()

function(_phvikapen_check_no_qt_dependency target)
    foreach(property IN ITEMS LINK_LIBRARIES INTERFACE_LINK_LIBRARIES)
        get_target_property(libraries ${target} ${property})
        if(libraries AND libraries MATCHES "Qt[0-9]*::")
            message(FATAL_ERROR "${target} must not depend on Qt (${property}: ${libraries})")
        endif()
    endforeach()
endfunction()
