#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstdlib>
#include <iostream>
#include <stdexcept>

/// \class TriangleProgram
///
/// Simple program, which initialises a window and draws a triangle in the view using the Vulkan API.
class TriangleProgram
{
public:
    /// Begin executing the TriangleProgram.
    void Run()
    {
        InitWindow();
        InitVulkan();
        MainLoop();
        Teardown();
    }

private:
    // Primvate methods.

    // Initialize a window.
    void InitWindow()
    {
        glfwInit();

        glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
        glfwWindowHint( GLFW_RESIZABLE, GLFW_FALSE );

        m_window = glfwCreateWindow( m_windowWidth, m_windowHeight, m_windowTitle, nullptr, nullptr );
    }

    // Initialize the Vulkan instance.
    void InitVulkan()
    {
    }

    // The main event loop.
    void MainLoop()
    {
        while ( !glfwWindowShouldClose( m_window ) )
        {
            glfwPollEvents();
        }
    }

    // Teardown internal state.
    void Teardown()
    {
        glfwDestroyWindow( m_window );
        glfwTerminate();
    }

    // Members.
    GLFWwindow* m_window       = nullptr;
    const int   m_windowWidth  = 800;
    const int   m_windowHeight = 600;
    const char* m_windowTitle  = "Triangle";
};

int main()
{
    TriangleProgram program;
    try
    {
        program.Run();
    }
    catch ( const std::exception& e )
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
