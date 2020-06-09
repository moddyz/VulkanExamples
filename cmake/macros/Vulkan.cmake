function(vulkan_shader TARGET SHADER)

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
endfunction(vulkan_shader)
