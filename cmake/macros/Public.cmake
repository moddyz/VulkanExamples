function(vulkanexamples_shader TARGET SHADER)

	# All shaders for a sample are found here.
    set(SHADER_OUTPUT_PATH ${CMAKE_BINARY_DIR}/shaders/${SHADER}.spv)

	# Add a custom command to compile GLSL to SPIR-V.
    get_filename_component(SHADER_OUTPUT_DIR ${SHADER_OUTPUT_PATH} DIRECTORY)
	file(MAKE_DIRECTORY ${SHADER_OUTPUT_DIR})
	add_custom_command(
		OUTPUT ${SHADER_OUTPUT_PATH}
		COMMAND ${GLSLC} -o ${SHADER_OUTPUT_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/${SHADER}
		DEPENDS ${SHADER}
		IMPLICIT_DEPENDS CXX ${SHADER}
		VERBATIM)

	# Make sure our native build depends on this output.
	set_source_files_properties(${SHADER_OUTPUT_PATH} PROPERTIES GENERATED TRUE)
	target_sources(${TARGET} PRIVATE ${SHADER_OUTPUT_PATH})

    install(
        FILES
            ${SHADER_OUTPUT_PATH}
        DESTINATION
            ${CMAKE_INSTALL_PREFIX}/shaders
    )
endfunction(vulkanexamples_shader)

function(vulkanexamples_program PROGRAM_NAME)

    set(options
    )

    set(oneValueArgs
    )

    set(multiValueArgs
        CPPFILES
        INCLUDE_PATHS
        LIBRARIES
        SHADERS
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

    foreach(SHADER ${args_SHADERS})
        vulkanexamples_shader(${PROGRAM_NAME} ${SHADER})
    endforeach(SHADER)

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
