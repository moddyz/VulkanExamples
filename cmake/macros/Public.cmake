function(vulkanexamples_program PROGRAM_NAME)

    set(options
    )

    set(oneValueArgs
    )

    set(multiValueArgs
        CPPFILES
        INCLUDE_PATHS
        LIBRARIES
        RESOURCE_FILES
    )

    cmake_parse_arguments(args
        "${options}"
        "${oneValueArgs}"
        "${multiValueArgs}"
        ${ARGN}
    )

    add_executable(${PROGRAM_NAME} ${args_CPPFILES})

    target_compile_options(${PROGRAM_NAME}
        PRIVATE
            -g -O3 -Wno-deprecated -Werror
    )

    target_compile_features(${PROGRAM_NAME}
        PRIVATE
            cxx_std_17
    )

    target_include_directories(${PROGRAM_NAME}
        PRIVATE
            $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
            ${args_INCLUDE_PATHS}
    )

    target_link_libraries(
        ${PROGRAM_NAME} PRIVATE ${args_LIBRARIES}
    )

    # Install
    install(
        TARGETS
            ${PROGRAM_NAME}
        DESTINATION
            ${CMAKE_INSTALL_PREFIX}/bin
    )

    # Install resources.
    if (args_RESOURCE_FILES)
        install(
            FILES
                ${args_RESOURCE_FILES}
            DESTINATION
                ${CMAKE_INSTALL_PREFIX}/bin
        )
    endif()

endfunction() # vulkanexamples_program
