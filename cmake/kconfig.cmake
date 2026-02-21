# cmake/kconfig.cmake
macro(load_kconfig_config)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.config")
        file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/.config" ConfigContents REGEX "^[^#]")
        foreach(NameAndValue ${ConfigContents})
            string(REGEX REPLACE "^CONFIG_([^=]+)=(.*)" "\\1" Name "${NameAndValue}")
            string(REGEX REPLACE "^CONFIG_([^=]+)=(.*)" "\\2" Value "${NameAndValue}")

            # Remove quotes from string values
            string(REGEX REPLACE "^\"(.*)\"$" "\\1" Value "${Value}")

            # Convert 'y'/'n' to ON/OFF for CMake options
            if("${Value}" STREQUAL "y")
                set(${Name} ON CACHE BOOL "Kconfig option" FORCE)
            elseif("${Value}" STREQUAL "n")
                set(${Name} OFF CACHE BOOL "Kconfig option" FORCE)
            else()
                set(${Name} "${Value}" CACHE STRING "Kconfig option" FORCE)
            endif()

            message(STATUS "Kconfig: ${Name} = ${Value}")
        endforeach()
    endif()
endmacro()

function(add_kconfig_targets)
    find_program(KCONFIG_MCONF kconfig-mconf)
    if(NOT KCONFIG_MCONF)
        find_program(KCONFIG_MCONF mconf)
    endif()

    if(KCONFIG_MCONF)
        # Create menuconfig target
        add_custom_target(menuconfig
            COMMAND ${KCONFIG_MCONF} Kconfig
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            USES_TERMINAL
            COMMENT "Running Kconfig configuration..."
        )
    else()
        message(WARNING "kconfig-mconf not found. 'make menuconfig' will not be available.")
    endif()

    find_program(KCONFIG_CONF kconfig-conf)
    if(NOT KCONFIG_CONF)
        find_program(KCONFIG_CONF conf)
    endif()

    if(KCONFIG_CONF)
        # Create alldefconfig target
        add_custom_target(alldefconfig
            COMMAND ${KCONFIG_CONF} --alldefconfig Kconfig
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            COMMENT "Generating default Kconfig configuration..."
        )
    endif()
endfunction()
