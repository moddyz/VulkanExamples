cmake_minimum_required(VERSION 3.17)

# Project Name
project(
    VulkanExperiments VERSION 0.0
    DESCRIPTION "Experiments with the Vulkan Graphics API."
    LANGUAGES CXX
)

# Cross-platform compatibility.
if (UNIX AND NOT APPLE)
    set(LINUX TRUE)
endif()

# Find modules.
list(INSERT CMAKE_MODULE_PATH 0 "${CMAKE_SOURCE_DIR}/cmake/modules")
find_package(GLFW REQUIRED)
find_package(Vulkan REQUIRED)
find_program(GLSLC glslc HINTS ${Vulkan_SDK}/bin REQUIRED)
if (EXISTS ${GLSLC})
    message(STATUS "Found glslc: ${GLSLC}")
else()
    message(FATAL_ERROR
            "glslc not found, required for compiling shaders.")
endif()

# Add convenience macros.
list(INSERT CMAKE_MODULE_PATH 0 "${CMAKE_SOURCE_DIR}/cmake/macros")
include(Public)
include(Vulkan)

# Compiler options - leave no symbols un-defined!
string(APPEND CMAKE_SHARED_LINKER_FLAGS " -Wl,--no-undefined")

# Custom debug define.
set(CMAKE_CXX_FLAGS_DEBUG "-DVULKANEXAMPLES_DEBUG")

# Build targets.
add_subdirectory(src)
