set(PROGRAM_NAME "triangle")

cpp_program(${PROGRAM_NAME}
    CPPFILES
        main.cpp
    INCLUDE_PATHS
        ${GLFW_INCLUDE_DIR}
        ${GLM_INCLUDE_DIR}
        ${Vulkan_INCLUDE_DIR}
    LIBRARIES
        ${GLFW_LIBRARIES}
        ${Vulkan_LIBRARY}
        vkbase
)

vulkan_shader(${PROGRAM_NAME} shader.vert)
vulkan_shader(${PROGRAM_NAME} shader.frag)
