#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <unordered_set>
#include <vector>

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback( VkDebugUtilsMessageSeverityFlagBitsEXT      i_messageSeverity,
                                                     VkDebugUtilsMessageTypeFlagsEXT             i_messageType,
                                                     const VkDebugUtilsMessengerCallbackDataEXT* i_pCallbackData,
                                                     void*                                       i_pUserData )
{
    std::cerr << "validation layer: " << i_pCallbackData->pMessage << std::endl;

    // Should the vulkan call, which triggered this debug callback, be aborted?
    return VK_FALSE;
}

/// Retrieve the create debug messenger function.  It is an extension, so we must fetch the function
/// pointer with vkGetInstanceProcAddr.
VkResult CreateDebugUtilsMessengerEXT( VkInstance                                i_instance,
                                       const VkDebugUtilsMessengerCreateInfoEXT* i_pCreateInfo,
                                       const VkAllocationCallbacks*              i_pAllocator,
                                       VkDebugUtilsMessengerEXT*                 o_pDebugMessenger )
{
    PFN_vkCreateDebugUtilsMessengerEXT func =
        ( PFN_vkCreateDebugUtilsMessengerEXT ) vkGetInstanceProcAddr( i_instance, "vkCreateDebugUtilsMessengerEXT" );
    if ( func != nullptr )
    {
        return func( i_instance, i_pCreateInfo, i_pAllocator, o_pDebugMessenger );
    }
    else
    {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT( VkInstance                   i_instance,
                                    VkDebugUtilsMessengerEXT     o_debugMessenger,
                                    const VkAllocationCallbacks* i_pAllocator )
{
    PFN_vkDestroyDebugUtilsMessengerEXT func =
        ( PFN_vkDestroyDebugUtilsMessengerEXT ) vkGetInstanceProcAddr( i_instance, "vkDestroyDebugUtilsMessengerEXT" );
    if ( func != nullptr )
    {
        func( i_instance, o_debugMessenger, i_pAllocator );
    }
}

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

    bool CheckVulkanLayersSupport( const std::vector< const char* >& i_requestedLayers ) const
    {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties( &layerCount, nullptr );
        std::vector< VkLayerProperties > availableLayers( layerCount );
        vkEnumerateInstanceLayerProperties( &layerCount, availableLayers.data() );

        // Validation layer set.
        std::unordered_set< std::string > availableLayersSet;
        for ( const VkLayerProperties& layer : availableLayers )
        {
            availableLayersSet.insert( std::string( layer.layerName ) );
        }

        // Check that each requested extension is available.
        size_t missingLayersCount = 0;
        for ( const char* layerName : i_requestedLayers )
        {
            std::unordered_set< std::string >::const_iterator layerIt =
                availableLayersSet.find( std::string( layerName ) );

            if ( layerIt != availableLayersSet.end() )
            {
                printf( "Found requested Vulkan layer: %s.\n", layerName );
            }
            else
            {
                printf( "Missing requested Vulkan layer: %s.\n", layerName );
                missingLayersCount++;
            }
        }

        return missingLayersCount == 0;
    }

    // Query available vulkan extensions, and validate against the \p i_requestedExtensions array.
    bool CheckVulkanExtensionsSupport( const std::vector< const char* >& i_requestedExtensions ) const
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
        for ( const char* extension : i_requestedExtensions )
        {
            std::unordered_set< std::string >::const_iterator extensionIt =
                availableExtensionsSet.find( std::string( extension ) );

            if ( extensionIt != availableExtensionsSet.end() )
            {
                printf( "Found requested Vulkan extension: %s.\n", extension );
            }
            else
            {
                printf( "Missing requested Vulkan extension: %s.\n", extension );
                missingExtensionsCount++;
            }
        }

        return missingExtensionsCount == 0;
    }

    std::vector< const char* > GetRequiredExtensions() const
    {
        // Enable gflw interface extensions.
        uint32_t     glfwExtensionCount = 0;
        const char** glfwExtensions     = glfwGetRequiredInstanceExtensions( &glfwExtensionCount );

        std::vector< const char* > extensions( glfwExtensions, glfwExtensions + glfwExtensionCount );
        if ( m_enableValidationLayers )
        {
            extensions.push_back( VK_EXT_DEBUG_UTILS_EXTENSION_NAME );
        }

        return extensions;
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

        // Check extensions support.
        if ( m_enableValidationLayers && !CheckVulkanLayersSupport( m_validationLayers ) )
        {
            throw std::runtime_error( "Missing vulkan layers, abort!" );
        }

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
        if ( m_enableValidationLayers )
        {
            createInfo.enabledLayerCount   = static_cast< uint32_t >( m_validationLayers.size() );
            createInfo.ppEnabledLayerNames = m_validationLayers.data();

            PopulateDebugMessengerCreateInfo( debugCreateInfo );
            createInfo.pNext = ( VkDebugUtilsMessengerCreateInfoEXT* ) &debugCreateInfo;
        }
        else
        {
            createInfo.enabledLayerCount = 0;
            createInfo.pNext = nullptr;
        }

        // Check extensions support.
        std::vector< const char* > extensions = GetRequiredExtensions();
        if ( !CheckVulkanExtensionsSupport( extensions ) )
        {
            throw std::runtime_error( "Missing vulkan extensions, abort!" );
        }

        createInfo.enabledExtensionCount   = extensions.size();
        createInfo.ppEnabledExtensionNames = extensions.data();

        // Create the vulkan instance.
        if ( vkCreateInstance( &createInfo, nullptr, &m_instance ) != VK_SUCCESS )
        {
            throw std::runtime_error( "Failed to create vulkan instance." );
        }
    }

    /// Utility method for populating the the debug messenger create info.
    void PopulateDebugMessengerCreateInfo( VkDebugUtilsMessengerCreateInfoEXT& o_createInfo )
    {
        o_createInfo                 = {};
        o_createInfo.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        o_createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                       VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                       VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        o_createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                   VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                   VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        o_createInfo.pfnUserCallback = DebugCallback;
    }

    // Setup the debug messenger.
    void SetupDebugMessenger()
    {
        if ( !m_enableValidationLayers )
        {
            return;
        }

        VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
        PopulateDebugMessengerCreateInfo( createInfo );
        if ( CreateDebugUtilsMessengerEXT( m_instance, &createInfo, nullptr, &m_debugMessenger ) != VK_SUCCESS )
        {
            throw std::runtime_error( "failed to set up debug messenger!" );
        }
    }

    // Initialize the Vulkan instance.
    void InitVulkan()
    {
        CreateVulkanInstance();
        SetupDebugMessenger();
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
        if ( m_enableValidationLayers )
        {
            DestroyDebugUtilsMessengerEXT( m_instance, m_debugMessenger, nullptr );
        }

        vkDestroyInstance( m_instance, nullptr );
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

    // Should validation layers be enabled for this program?
    const bool m_enableValidationLayers = true;

    // Available validation layers.
    const std::vector< const char* > m_validationLayers = {"VK_LAYER_KHRONOS_validation"};

    // Vulkan instance.
    VkInstance               m_instance;
    VkDebugUtilsMessengerEXT m_debugMessenger;
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
