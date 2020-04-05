cmake_minimum_required(VERSION 3.10)

# Project Name
project(
    VulkanExamples VERSION 0.0
    DESCRIPTION "Vulkan examples."
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

list(INSERT CMAKE_MODULE_PATH 0 "${CMAKE_SOURCE_DIR}/cmake/macros")
include(Public)

# Compiler options
string(APPEND CMAKE_SHARED_LINKER_FLAGS " -Wl,--no-undefined")

# Debug option.
set(CMAKE_CXX_FLAGS_DEBUG "-DVULKANEXAMPLES_DEBUG")

# Build targets.
add_subdirectory(triangle)
