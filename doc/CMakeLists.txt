find_package(Doxygen)

if(DOXYGEN_FOUND)
    # Configure the Doxyfile
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/doxyfile
        ${CMAKE_CURRENT_BINARY_DIR}/doxyfile
        @ONLY
    )

    add_custom_target(doc
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/doxyfile
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM
    )
else()
    message(STATUS "Doxygen not found. 'doc' target will not be available.")
endif()
