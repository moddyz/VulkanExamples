# Header only library

set(LIBRARY_NAME "vkbase")
add_library(
    ${LIBRARY_NAME}
    IMPORTED  # This is external.
    INTERFACE # This library target does not provide source files.  (Header only!)
    GLOBAL    # Make this library target available in directories above this one.
)

file(GLOB HEADERS *.h)
file(
    COPY ${HEADERS}
    DESTINATION ${CMAKE_BINARY_DIR}/include/${LIBRARY_NAME}
)

# Any target which links against this library  will inherit the
# include path of where the headers are installed.
target_include_directories(
    ${LIBRARY_NAME}
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
)
