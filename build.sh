#!/bin/bash

set -euxo pipefail

mkdir -p build && cd build

CMAKE_ARGS=\
\ -DGLFW_LOCATION="/apps/glfw/3.3.2/"\
\ -DGLM_INCLUDE_DIR="/apps/glm/0.9.9.7/include"\
\ -DVulkan_LIBRARY="/apps/vulkan/1.2.131.2/x86_64/lib/libvulkan.so"\
\ -DVulkan_INCLUDE_DIR="/apps/vulkan/1.2.131.2/x86_64/include"\
\ -DCMAKE_BUILD_TYPE="Debug"

# Only build if installation path not specified.
if [ $# -eq 0 ]
then
    cmake $CMAKE_ARGS ..
    cmake --build . -- all test -j 8
else
    cmake $CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=$1 ..
    cmake --build . --target install -- -j 8
fi
