#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <unordered_set>
#include <vector>

/// \class TriangleProgram
///
/// A simple program which draws a triangle using the Vulkan API, in a window.
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

    // Query available vulkan extensions, and validate against the \p i_requestedExtensions array.
    bool ValidateVulkanExtensions( uint32_t i_requestedExtensionsCount, const char** i_requestedExtensions )
    {
        uint32_t availableExtensionsCount = 0;
        vkEnumerateInstanceExtensionProperties( nullptr, &availableExtensionsCount, nullptr );
        std::vector< VkExtensionProperties > availableExtensions( availableExtensionsCount );
        vkEnumerateInstanceExtensionProperties( nullptr, &availableExtensionsCount, availableExtensions.data() );

        std::unordered_set< std::string > availableExtensionsSet;
        for ( const VkExtensionProperties& extension : availableExtensions )
        {
            availableExtensionsSet.insert( std::string( extension.extensionName ) );
        }

        // Check that each requested extension is available.
        size_t missingExtensionsCount = 0;
        for ( size_t extensionIndex = 0; extensionIndex < i_requestedExtensionsCount; ++extensionIndex )
        {
            std::string requestedExtension( i_requestedExtensions[ extensionIndex ] );
            std::unordered_set< std::string >::const_iterator extensionIt =
                availableExtensionsSet.find( requestedExtension );

            if ( extensionIt != availableExtensionsSet.end() )
            {
                printf( "Found requested Vulkan extension: %s.\n", extensionIt->c_str() );
            }
            else
            {
                printf( "Missing requested Vulkan extension: %s.\n", extensionIt->c_str() );
                missingExtensionsCount++;
            }
        }

        return missingExtensionsCount > 0;
    }

    // Create the vulkan instance.
    void CreateVulkanInstance()
    {
        // Information about the application.
        VkApplicationInfo appInfo  = {};
        appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName   = "Hello Triangle";
        appInfo.applicationVersion = VK_MAKE_VERSION( 1, 0, 0 );
        appInfo.pEngineName        = "No Engine";
        appInfo.engineVersion      = VK_MAKE_VERSION( 1, 0, 0 );
        appInfo.apiVersion         = VK_API_VERSION_1_0;

        // Instance creation info.
        VkInstanceCreateInfo createInfo = {};
        createInfo.sType                = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo     = &appInfo;

        // Enable gflw interface extensions.
        uint32_t     glfwExtensionCount = 0;
        const char** glfwExtensions     = glfwGetRequiredInstanceExtensions( &glfwExtensionCount );

        ValidateVulkanExtensions( glfwExtensionCount, glfwExtensions );

        createInfo.enabledExtensionCount   = glfwExtensionCount;
        createInfo.ppEnabledExtensionNames = glfwExtensions;

        // Create the vulkan instance.
        if ( vkCreateInstance( &createInfo, nullptr, &m_vulkanInstance ) != VK_SUCCESS )
        {
            throw std::runtime_error( "Failed to create vulkan instance." );
        }
    }

    // Initialize the Vulkan instance.
    void InitVulkan()
    {
        CreateVulkanInstance();
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

    // Window instance.
    GLFWwindow* m_window = nullptr;

    // Dimensions of the window.
    const int m_windowWidth  = 800;
    const int m_windowHeight = 600;

    // Title of the window.
    const char* m_windowTitle = "Triangle";

    // Vulkan instance.
    VkInstance m_vulkanInstance;
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
        fprintf( stderr, "Error during runtime: %s.\n", e.what() );
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
