#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <optional>
#include <set>
#include <stdexcept>
#include <stdio.h>
#include <string.h>
#include <unordered_set>
#include <vector>

/// File system utilities.

static std::string GetParentDirectory( const std::string& i_path )
{
    return i_path.substr( 0, i_path.find_last_of( '/' ) );
}

struct BothSlashes
{
    bool operator()( char a, char b ) const
    {
        return a == '/' && b == '/';
    }
};

static std::string SanitizePath( const std::string& i_path )
{
    std::string path = i_path;
    path.erase( std::unique( path.begin(), path.end(), BothSlashes() ), path.end() );
    return path;
}

static std::string JoinPaths( const std::string& i_pathA, const std::string& i_pathB )
{
    std::string path = i_pathA + "/" + i_pathB;
    return SanitizePath( path );
}

/// Read a file, and return the array of binary data.
static std::vector< char > ReadFile( const std::string& i_filePath )
{
    std::ifstream file( i_filePath, std::ios::ate | std::ios::binary );
    if ( !file.is_open() )
    {
        throw std::runtime_error( "failed to open file!" );
    }

    // Read the entire file.
    size_t              fileSize = ( size_t ) file.tellg();
    std::vector< char > buffer( fileSize );
    file.seekg( 0 );
    file.read( buffer.data(), fileSize );
    file.close();

    return buffer;
}

static constexpr int s_maxFramesInFlight = 2;

/// \class TriangleApplication
///
/// A simple app which draws a triangle using the Vulkan API, in a window.
class TriangleApplication
{
public:
    explicit TriangleApplication( const std::string& i_executablePath )
        : m_executablePath( i_executablePath )
    {
    }

    /// Begin executing the TriangleApplication.
    void Run()
    {
        InitWindow();
        InitVulkan();
        MainLoop();
        Teardown();
    }

private:
    /// \struct SwapChainSupportDetails
    ///
    /// Swap chain details.
    struct SwapChainSupportDetails
    {
        VkSurfaceCapabilitiesKHR          m_capabilities;
        std::vector< VkSurfaceFormatKHR > m_formats;
        std::vector< VkPresentModeKHR >   m_presentModes;
    };

    /// \struct QueueFamilyIndices
    ///
    /// Internal structure for encoding queue family support, queried from the physical device.
    struct QueueFamilyIndices
    {
        std::optional< uint32_t > m_graphicsFamily;
        std::optional< uint32_t > m_presentFamily;

        /// Convenience method for checking if all the queue families that are required for this application
        /// are found.
        bool IsComplete()
        {
            return m_graphicsFamily.has_value() && m_presentFamily.has_value();
        }
    };

    static void FramebufferResizeCallback( GLFWwindow* i_window, int i_width, int i_height )
    {
        TriangleApplication* app  = reinterpret_cast< TriangleApplication* >( glfwGetWindowUserPointer( i_window ) );
        app->m_framebufferResized = true;
    }

    // Initialize a window.
    void InitWindow()
    {
        glfwInit();

        glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );

        m_window = glfwCreateWindow( m_windowWidth, m_windowHeight, m_windowTitle, nullptr, nullptr );
        glfwSetWindowUserPointer( m_window, this );
        glfwSetFramebufferSizeCallback( m_window, FramebufferResizeCallback );
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
            createInfo.pNext             = nullptr;
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
    void PopulateDebugMessengerCreateInfo( VkDebugUtilsMessengerCreateInfoEXT& o_createInfo ) const
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

    SwapChainSupportDetails QuerySwapChainSupport( VkPhysicalDevice i_device ) const
    {
        SwapChainSupportDetails details;

        // Query capabilities.
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR( i_device, m_surface, &details.m_capabilities );

        // Query available formats
        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR( i_device, m_surface, &formatCount, nullptr );
        if ( formatCount != 0 )
        {
            details.m_formats.resize( formatCount );
            vkGetPhysicalDeviceSurfaceFormatsKHR( i_device, m_surface, &formatCount, details.m_formats.data() );
        }

        // Presentation modes.
        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR( i_device, m_surface, &presentModeCount, nullptr );
        if ( presentModeCount != 0 )
        {
            details.m_presentModes.resize( presentModeCount );
            vkGetPhysicalDeviceSurfacePresentModesKHR( i_device,
                                                       m_surface,
                                                       &presentModeCount,
                                                       details.m_presentModes.data() );
        }

        return details;
    }

    /// Query if the current device supports
    QueueFamilyIndices FindQueueFamilies( VkPhysicalDevice i_device ) const
    {
        QueueFamilyIndices indices;

        // Get available queue families.
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties( i_device, &queueFamilyCount, nullptr );
        std::vector< VkQueueFamilyProperties > queueFamilies( queueFamilyCount );
        vkGetPhysicalDeviceQueueFamilyProperties( i_device, &queueFamilyCount, queueFamilies.data() );

        for ( size_t familyIndex = 0; familyIndex < queueFamilies.size(); ++familyIndex )
        {
            const VkQueueFamilyProperties& queueFamily = queueFamilies[ familyIndex ];

            // Present support.
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR( i_device, familyIndex, m_surface, &presentSupport );
            if ( presentSupport )
            {
                indices.m_presentFamily = familyIndex;
            }

            // Graphics support.
            if ( queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT )
            {
                indices.m_graphicsFamily = familyIndex;
            }

            if ( indices.IsComplete() )
            {
                break;
            }
        }

        return indices;
    }

    /// Check if the requested extensions are supported, for \p i_device.
    bool CheckDeviceExtensionSupport( VkPhysicalDevice i_device ) const
    {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties( i_device, nullptr, &extensionCount, nullptr );

        std::vector< VkExtensionProperties > availableExtensions( extensionCount );
        vkEnumerateDeviceExtensionProperties( i_device, nullptr, &extensionCount, availableExtensions.data() );

        std::set< std::string > requiredExtensions( m_deviceExtensions.begin(), m_deviceExtensions.end() );
        for ( const VkExtensionProperties& extension : availableExtensions )
        {
            requiredExtensions.erase( extension.extensionName );
        }

        return requiredExtensions.empty();
    }

    /// Check if the the input device \pr i_device is suitable for the current application.
    bool IsDeviceSuitable( VkPhysicalDevice i_device ) const
    {
        QueueFamilyIndices indices             = FindQueueFamilies( i_device );
        bool               extensionsSupported = CheckDeviceExtensionSupport( i_device );

        bool swapChainAdequate = false;
        if ( extensionsSupported )
        {
            SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport( i_device );
            swapChainAdequate = !swapChainSupport.m_formats.empty() && !swapChainSupport.m_presentModes.empty();
        }

        return indices.IsComplete() && extensionsSupported && swapChainAdequate;
    }

    void SelectPhysicalDevice()
    {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices( m_instance, &deviceCount, nullptr );
        if ( deviceCount == 0 )
        {
            throw std::runtime_error( "Failed to find graphics device with Vulkan support." );
        }

        std::vector< VkPhysicalDevice > devices( deviceCount, VK_NULL_HANDLE );
        vkEnumeratePhysicalDevices( m_instance, &deviceCount, devices.data() );
        for ( VkPhysicalDevice device : devices )
        {
            if ( IsDeviceSuitable( device ) )
            {
                m_physicalDevice = device;
                break;
            }
        }

        if ( m_physicalDevice == VK_NULL_HANDLE )
        {
            throw std::runtime_error( "Failed to find suitable graphics device." );
        }
    }

    void CreateLogicalDevice()
    {
        QueueFamilyIndices indices = FindQueueFamilies( m_physicalDevice );

        std::vector< VkDeviceQueueCreateInfo > queueCreateInfos;
        std::set< uint32_t > uniqueQueueFamilies = {indices.m_graphicsFamily.value(), indices.m_presentFamily.value()};
        float                queuePriority       = 1.0f;
        for ( uint32_t queueFamily : uniqueQueueFamilies )
        {
            VkDeviceQueueCreateInfo queueCreateInfo = {};
            queueCreateInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex        = queueFamily;
            queueCreateInfo.queueCount              = 1;
            queueCreateInfo.pQueuePriorities        = &queuePriority;
            queueCreateInfos.push_back( queueCreateInfo );
        }

        // No request for any specific features for this application.
        VkPhysicalDeviceFeatures deviceFeatures = {};

        /// Create logical device.
        VkDeviceCreateInfo createInfo   = {};
        createInfo.sType                = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pQueueCreateInfos    = queueCreateInfos.data();
        createInfo.queueCreateInfoCount = static_cast< uint32_t >( queueCreateInfos.size() );
        createInfo.pEnabledFeatures     = &deviceFeatures;

        createInfo.enabledExtensionCount   = static_cast< uint32_t >( m_deviceExtensions.size() );
        createInfo.ppEnabledExtensionNames = m_deviceExtensions.data();

        if ( m_enableValidationLayers )
        {
            createInfo.enabledLayerCount   = static_cast< uint32_t >( m_validationLayers.size() );
            createInfo.ppEnabledLayerNames = m_validationLayers.data();
        }
        else
        {
            createInfo.enabledLayerCount = 0;
        }

        if ( vkCreateDevice( m_physicalDevice, &createInfo, nullptr, &m_device ) != VK_SUCCESS )
        {
            throw std::runtime_error( "Failed to create logical device." );
        }

        // Get a handle to the graphics command queue.
        vkGetDeviceQueue( m_device, indices.m_graphicsFamily.value(), 0, &m_graphicsQueue );
        vkGetDeviceQueue( m_device, indices.m_presentFamily.value(), 0, &m_presentQueue );
    }

    /// Debug callback handling.
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
            ( PFN_vkCreateDebugUtilsMessengerEXT ) vkGetInstanceProcAddr( i_instance,
                                                                          "vkCreateDebugUtilsMessengerEXT" );
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
            ( PFN_vkDestroyDebugUtilsMessengerEXT ) vkGetInstanceProcAddr( i_instance,
                                                                           "vkDestroyDebugUtilsMessengerEXT" );
        if ( func != nullptr )
        {
            func( i_instance, o_debugMessenger, i_pAllocator );
        }
    }

    /// Choose a preferred surface format.
    VkSurfaceFormatKHR SelectSwapSurfaceFormat( const std::vector< VkSurfaceFormatKHR >& i_availableFormats ) const
    {
        for ( const VkSurfaceFormatKHR& availableFormat : i_availableFormats )
        {
            if ( availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
                 availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR )
            {
                return availableFormat;
            }
        }

        return i_availableFormats[ 0 ];
    }

    VkPresentModeKHR SelectSwapPresentMode( const std::vector< VkPresentModeKHR >& i_availablePresentModes ) const
    {
        for ( const VkPresentModeKHR& availablePresentMode : i_availablePresentModes )
        {
            if ( availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR )
            {
                return availablePresentMode;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D SelectSwapExtent( const VkSurfaceCapabilitiesKHR& i_capabilities )
    {
        if ( i_capabilities.currentExtent.width != UINT32_MAX )
        {
            return i_capabilities.currentExtent;
        }
        else
        {
            int width, height;
            glfwGetFramebufferSize( m_window, &width, &height );
            VkExtent2D actualExtent = {( uint32_t ) width, ( uint32_t ) height};

            actualExtent.width  = std::max( i_capabilities.minImageExtent.width,
                                           std::min( i_capabilities.maxImageExtent.width, actualExtent.width ) );
            actualExtent.height = std::max( i_capabilities.minImageExtent.height,
                                            std::min( i_capabilities.maxImageExtent.height, actualExtent.height ) );

            return actualExtent;
        }
    }

    void CreateSwapChain()
    {
        SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport( m_physicalDevice );
        VkSurfaceFormatKHR      surfaceFormat    = SelectSwapSurfaceFormat( swapChainSupport.m_formats );
        VkPresentModeKHR        presentMode      = SelectSwapPresentMode( swapChainSupport.m_presentModes );
        VkExtent2D              extent           = SelectSwapExtent( swapChainSupport.m_capabilities );

        uint32_t imageCount = swapChainSupport.m_capabilities.minImageCount + 1;
        if ( swapChainSupport.m_capabilities.maxImageCount > 0 &&
             imageCount > swapChainSupport.m_capabilities.maxImageCount )
        {
            imageCount = swapChainSupport.m_capabilities.maxImageCount;
        }

        // Create the swap chain.
        VkSwapchainCreateInfoKHR createInfo = {};
        createInfo.sType                    = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface                  = m_surface;
        createInfo.minImageCount            = imageCount;
        createInfo.imageFormat              = surfaceFormat.format;
        createInfo.imageColorSpace          = surfaceFormat.colorSpace;
        createInfo.imageExtent              = extent;
        createInfo.imageArrayLayers         = 1;
        createInfo.imageUsage               = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        // Specify how the images in the swap chain will be used across multiple queue families.
        QueueFamilyIndices indices              = FindQueueFamilies( m_physicalDevice );
        uint32_t           queueFamilyIndices[] = {indices.m_graphicsFamily.value(), indices.m_presentFamily.value()};
        if ( indices.m_graphicsFamily != indices.m_presentFamily )
        {
            createInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices   = queueFamilyIndices;
        }
        else
        {
            createInfo.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0;       // Optional
            createInfo.pQueueFamilyIndices   = nullptr; // Optional
        }

        // No transformation.
        createInfo.preTransform = swapChainSupport.m_capabilities.currentTransform;

        // Don't alpha-blend with other windows.
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

        // Present mode.
        createInfo.presentMode = presentMode;

        // Do we care about the color of pixels obscured by a window in the front?
        createInfo.clipped = VK_TRUE;

        // For later, when we want to create a new optimal swap chain to replace an older one.
        createInfo.oldSwapchain = VK_NULL_HANDLE;

        if ( vkCreateSwapchainKHR( m_device, &createInfo, nullptr, &m_swapChain ) != VK_SUCCESS )
        {
            throw std::runtime_error( "failed to create swap chain!" );
        }

        // Get handles to swap chain images, which we will render into.
        vkGetSwapchainImagesKHR( m_device, m_swapChain, &imageCount, nullptr );
        m_swapChainImages.resize( imageCount );
        vkGetSwapchainImagesKHR( m_device, m_swapChain, &imageCount, m_swapChainImages.data() );

        // Cache image format & extent.
        m_swapChainImageFormat = surfaceFormat.format;
        m_swapChainExtent      = extent;
    }

    void TeardownSwapChain()
    {
        for ( VkFramebuffer framebuffer : m_swapChainFramebuffers )
        {
            vkDestroyFramebuffer( m_device, framebuffer, nullptr );
        }

        vkFreeCommandBuffers( m_device,
                              m_commandPool,
                              static_cast< uint32_t >( m_commandBuffers.size() ),
                              m_commandBuffers.data() );

        vkDestroyPipeline( m_device, m_graphicsPipeline, nullptr );
        vkDestroyPipelineLayout( m_device, m_pipelineLayout, nullptr );
        vkDestroyRenderPass( m_device, m_renderPass, nullptr );

        for ( VkImageView imageView : m_swapChainImageViews )
        {
            vkDestroyImageView( m_device, imageView, nullptr );
        }

        vkDestroySwapchainKHR( m_device, m_swapChain, nullptr );
    }

    void RecreateSwapChain()
    {
        // Pause application on minimization.
        int width = 0, height = 0;
        do
        {
            glfwGetFramebufferSize( m_window, &width, &height );
            glfwWaitEvents();
        } while ( width == 0 || height == 0 );

        // Wait for graphics operations to complete.
        vkDeviceWaitIdle( m_device );

        m_framebufferResized = false;

        TeardownSwapChain();

        CreateSwapChain();
        CreateImageViews();
        CreateRenderPass();
        CreateGraphicsPipeline();
        CreateFramebuffers();
        CreateCommandBuffers();
    }

    void CreateSurface()
    {
        if ( glfwCreateWindowSurface( m_instance, m_window, nullptr, &m_surface ) != VK_SUCCESS )
        {
            throw std::runtime_error( "Failed to create window surface." );
        }
    }

    void CreateImageViews()
    {
        m_swapChainImageViews.resize( m_swapChainImages.size() );
        for ( size_t imageIndex = 0; imageIndex < m_swapChainImages.size(); imageIndex++ )
        {
            VkImageViewCreateInfo createInfo = {};
            createInfo.sType                 = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image                 = m_swapChainImages[ imageIndex ];
            createInfo.viewType              = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format                = m_swapChainImageFormat;

            // Do not remap components.
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

            // For displaying color.
            createInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel   = 0;
            createInfo.subresourceRange.levelCount     = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount     = 1;

            if ( vkCreateImageView( m_device, &createInfo, nullptr, &m_swapChainImageViews[ imageIndex ] ) !=
                 VK_SUCCESS )
            {
                throw std::runtime_error( "Failed to create image views." );
            }
        }
    }

    VkShaderModule CreateShaderModule( const std::vector< char >& i_code )
    {
        VkShaderModuleCreateInfo createInfo = {};
        createInfo.sType                    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize                 = i_code.size(); // Num bytes.
        createInfo.pCode                    = reinterpret_cast< const uint32_t* >( i_code.data() );

        VkShaderModule shaderModule;
        if ( vkCreateShaderModule( m_device, &createInfo, nullptr, &shaderModule ) != VK_SUCCESS )
        {
            throw std::runtime_error( "Failed to create shader module." );
        }

        return shaderModule;
    }

    void CreateRenderPass()
    {
        // Color buffer attachment.
        VkAttachmentDescription colorAttachment = {};
        colorAttachment.format =
            m_swapChainImageFormat; // The color buffer should have the same format as our swapchain images.
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;        // Only one sample.
        colorAttachment.loadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;  // Clear the buffer to constant value, before write.
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // Keep the written colors around after.
        colorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        // Reference to the color buffer attachment.
        VkAttachmentReference colorAttachmentRef = {};
        colorAttachmentRef.attachment            = 0;
        colorAttachmentRef.layout                = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        // Create the subpass.  In this example, there will only be a single sub-pass.
        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS; // This is a graphics subpass (and not compute).
        subpass.colorAttachmentCount = 1;                   // Only a single color attachment for this subpass.
        subpass.pColorAttachments    = &colorAttachmentRef; // Reference to the color attachment.

        // Create the render pass.
        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType                  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount        = 1;                // Only a single attachment (the color buffer).
        renderPassInfo.pAttachments           = &colorAttachment; // The array of attachments.
        renderPassInfo.subpassCount           = 1;                // Only a single subpass.
        renderPassInfo.pSubpasses             = &subpass;         // The array of subpasses.

        // Have the implicit subpass to wait until we are ready to write to the color buffer.
        VkSubpassDependency dependency = {};
        dependency.srcSubpass          = VK_SUBPASS_EXTERNAL; // Refers to the implicit subpass before the render pass.
        dependency.dstSubpass          = 0; // This is the implicit subpass after the render pass.  Do nothing.
        dependency.srcStageMask =
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // Wait until we are ready to write color.
        dependency.srcAccessMask = 0;
        dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies   = &dependency;

        if ( vkCreateRenderPass( m_device, &renderPassInfo, nullptr, &m_renderPass ) != VK_SUCCESS )
        {
            throw std::runtime_error( "Failed to create render pass." );
        }
    }

    void CreateGraphicsPipeline()
    {
        std::string         executableDir   = GetParentDirectory( m_executablePath );
        std::string         shaderDirectory = JoinPaths( executableDir, "../shaders/" );
        std::vector< char > vertShaderCode  = ReadFile( JoinPaths( shaderDirectory, "shader.vert.spv" ) );
        std::vector< char > fragShaderCode  = ReadFile( JoinPaths( shaderDirectory, "shader.frag.spv" ) );

        // Create shader modules from code.
        VkShaderModule vertShaderModule = CreateShaderModule( vertShaderCode );
        VkShaderModule fragShaderModule = CreateShaderModule( fragShaderCode );

        // Create info for vertex shader pipeline stage.
        VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
        vertShaderStageInfo.sType                           = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage                           = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module                          = vertShaderModule;
        vertShaderStageInfo.pName                           = "main";

        // Create info for fragment shader pipeline stage.
        VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
        fragShaderStageInfo.sType                           = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage                           = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module                          = fragShaderModule;
        fragShaderStageInfo.pName                           = "main";

        // Shader stages.
        VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

        // Vertex input stage.
        VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
        vertexInputInfo.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount   = 0;
        vertexInputInfo.pVertexBindingDescriptions      = nullptr; // Optional
        vertexInputInfo.vertexAttributeDescriptionCount = 0;
        vertexInputInfo.pVertexAttributeDescriptions    = nullptr; // Optional

        // Input assembly, describing what kind of geometry will be drawn.
        VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
        inputAssembly.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        // Create the viewport.  This is the region in the framebuffer that the pixels will be rendered into.
        VkViewport viewport = {};
        viewport.x          = 0.0f;
        viewport.y          = 0.0f;
        viewport.width      = ( float ) m_swapChainExtent.width;
        viewport.height     = ( float ) m_swapChainExtent.height;
        viewport.minDepth   = 0.0f;
        viewport.maxDepth   = 1.0f;

        // Draw into the entire frame buffer.
        VkRect2D scissor = {};
        scissor.offset   = {0, 0};
        scissor.extent   = m_swapChainExtent;

        // Viewport state.
        VkPipelineViewportStateCreateInfo viewportState = {};
        viewportState.sType                             = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount                     = 1;
        viewportState.pViewports                        = &viewport;
        viewportState.scissorCount                      = 1;
        viewportState.pScissors                         = &scissor;

        // Rasterizer, for converting geometry shapes into fragments for shading.
        VkPipelineRasterizationStateCreateInfo rasterizer = {};
        rasterizer.sType                                  = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable                       = VK_FALSE;
        rasterizer.rasterizerDiscardEnable                = VK_FALSE;
        rasterizer.polygonMode                            = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth                              = 1.0f;
        rasterizer.cullMode                               = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace                              = VK_FRONT_FACE_CLOCKWISE;
        rasterizer.depthBiasEnable                        = VK_FALSE;
        rasterizer.depthBiasConstantFactor                = 0.0f; // Optional
        rasterizer.depthBiasClamp                         = 0.0f; // Optional
        rasterizer.depthBiasSlopeFactor                   = 0.0f; // Optional

        // Multi-sampling for reducing edge aliasing (disabled for now.)
        VkPipelineMultisampleStateCreateInfo multisampling = {};
        multisampling.sType                                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable                  = VK_FALSE;
        multisampling.rasterizationSamples                 = VK_SAMPLE_COUNT_1_BIT;
        multisampling.minSampleShading                     = 1.0f;     // Optional
        multisampling.pSampleMask                          = nullptr;  // Optional
        multisampling.alphaToCoverageEnable                = VK_FALSE; // Optional
        multisampling.alphaToOneEnable                     = VK_FALSE; // Optional

        // Color blending.
        VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
        colorBlendAttachment.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable         = VK_FALSE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;  // Optional
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
        colorBlendAttachment.colorBlendOp        = VK_BLEND_OP_ADD;      // Optional
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;  // Optional
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
        colorBlendAttachment.alphaBlendOp        = VK_BLEND_OP_ADD;      // Optional

        // Aggregate of color attachments.
        VkPipelineColorBlendStateCreateInfo colorBlending = {};
        colorBlending.sType                               = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable                       = VK_FALSE;
        colorBlending.logicOp                             = VK_LOGIC_OP_COPY; // Optional
        colorBlending.attachmentCount                     = 1;
        colorBlending.pAttachments                        = &colorBlendAttachment;
        colorBlending.blendConstants[ 0 ]                 = 0.0f; // Optional
        colorBlending.blendConstants[ 1 ]                 = 0.0f; // Optional
        colorBlending.blendConstants[ 2 ]                 = 0.0f; // Optional
        colorBlending.blendConstants[ 3 ]                 = 0.0f; // Optional

        // Pipeline layout.
        VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
        pipelineLayoutInfo.sType                      = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount             = 0;       // Optional
        pipelineLayoutInfo.pSetLayouts                = nullptr; // Optional
        pipelineLayoutInfo.pushConstantRangeCount     = 0;       // Optional
        pipelineLayoutInfo.pPushConstantRanges        = nullptr; // Optional
        if ( vkCreatePipelineLayout( m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout ) != VK_SUCCESS )
        {
            throw std::runtime_error( "failed to create pipeline layout!" );
        }

        // Now, create the pipeline!
        VkGraphicsPipelineCreateInfo pipelineInfo = {};
        pipelineInfo.sType                        = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount                   = 2;                // Number of _programmable_ stages.
        pipelineInfo.pStages                      = shaderStages;     // Array of programmable stages.
        pipelineInfo.pVertexInputState            = &vertexInputInfo; // Vertex input.
        pipelineInfo.pInputAssemblyState          = &inputAssembly;   // Input assembly.
        pipelineInfo.pViewportState               = &viewportState;   // Viewport.
        pipelineInfo.pRasterizationState          = &rasterizer;      // Rasterization state.
        pipelineInfo.pMultisampleState            = &multisampling;   // Multi sampling.
        pipelineInfo.pDepthStencilState           = nullptr;          // No depth / stenciling.
        pipelineInfo.pColorBlendState             = &colorBlending;   // Color blending.
        pipelineInfo.pDynamicState                = nullptr;          // Optional
        pipelineInfo.layout                       = m_pipelineLayout; // Layout.
        pipelineInfo.renderPass                   = m_renderPass; // The render pass, with the color buffer attachment.
        pipelineInfo.subpass            = 0; // The index of the subpass, where this graphics pipeline will be used.
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Pipeline to derived from.  None, in this case.
        pipelineInfo.basePipelineIndex  = -1;             // ???
        if ( vkCreateGraphicsPipelines( m_device,
                                        /* pipelineCache */ VK_NULL_HANDLE,
                                        1,
                                        &pipelineInfo,
                                        nullptr,
                                        &m_graphicsPipeline ) != VK_SUCCESS )
        {
            throw std::runtime_error( "Failed to create graphics pipeline." );
        }

        vkDestroyShaderModule( m_device, fragShaderModule, nullptr );
        vkDestroyShaderModule( m_device, vertShaderModule, nullptr );
    }

    void CreateFramebuffers()
    {
        m_swapChainFramebuffers.resize( m_swapChainImageViews.size() );
        for ( size_t imageIndex = 0; imageIndex < m_swapChainImageViews.size(); imageIndex++ )
        {
            VkImageView attachments[] = {m_swapChainImageViews[ imageIndex ]};

            VkFramebufferCreateInfo framebufferInfo = {};
            framebufferInfo.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass      = m_renderPass; // The render pass that this framebuffer is compatible with.
            framebufferInfo.attachmentCount = 1;            // Same number of attachments as the render pass.
            framebufferInfo.pAttachments    = attachments;  // attached image view.
            framebufferInfo.width           = m_swapChainExtent.width; // Extent.
            framebufferInfo.height          = m_swapChainExtent.height;
            framebufferInfo.layers          = 1; // The entries in the swap chain are single images.

            if ( vkCreateFramebuffer( m_device, &framebufferInfo, nullptr, &m_swapChainFramebuffers[ imageIndex ] ) !=
                 VK_SUCCESS )
            {
                throw std::runtime_error( "Failed to create framebuffer." );
            }
        }
    }

    void CreateCommandPool()
    {
        QueueFamilyIndices queueFamilyIndices = FindQueueFamilies( m_physicalDevice );

        VkCommandPoolCreateInfo poolInfo = {};
        poolInfo.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex =
            queueFamilyIndices.m_graphicsFamily.value(); // The queue that the commands will be submitted to.
        poolInfo.flags = 0;                              // Controls command buffer behavior.

        if ( vkCreateCommandPool( m_device, &poolInfo, nullptr, &m_commandPool ) != VK_SUCCESS )
        {
            throw std::runtime_error( "Failed to create command pool." );
        }
    }

    void CreateCommandBuffers()
    {
        // There will be a buffer for each frame buffer in the swap chain.
        m_commandBuffers.resize( m_swapChainFramebuffers.size() );

        VkCommandBufferAllocateInfo allocInfo = {};
        allocInfo.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool                 = m_commandPool; // The parent command pool, managing the buffer memory.
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; // PRIMARY level can be submitted to a queue for execution.
        allocInfo.commandBufferCount = ( uint32_t ) m_commandBuffers.size();

        if ( vkAllocateCommandBuffers( m_device, &allocInfo, m_commandBuffers.data() ) != VK_SUCCESS )
        {
            throw std::runtime_error( "Failed to allocate command buffers." );
        }

        // Begin recording into the command buffer(s).
        for ( size_t bufferIndex = 0; bufferIndex < m_commandBuffers.size(); bufferIndex++ )
        {
            VkCommandBufferBeginInfo beginInfo = {};
            beginInfo.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags                    = 0;       // Optional
            beginInfo.pInheritanceInfo         = nullptr; // Optional

            if ( vkBeginCommandBuffer( m_commandBuffers[ bufferIndex ], &beginInfo ) != VK_SUCCESS )
            {
                throw std::runtime_error( "Failed to begin recording command buffer." );
            }

            // Begin recording the render pass command.
            VkRenderPassBeginInfo renderPassInfo = {};
            renderPassInfo.sType                 = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass            = m_renderPass;
            renderPassInfo.framebuffer = m_swapChainFramebuffers[ bufferIndex ]; // The associated frame buffer.

            // Describes where the shader loads and stores will take place.
            renderPassInfo.renderArea.offset = {0, 0};
            renderPassInfo.renderArea.extent = m_swapChainExtent;

            // The color value used to reset the attachment to before writing.
            VkClearValue clearColor        = {0.0f, 0.0f, 0.0f, 1.0f};
            renderPassInfo.clearValueCount = 1;
            renderPassInfo.pClearValues    = &clearColor;

            // Begin render pass.
            //
            // VK_SUBPASS_CONTENTS_INLINE means that the render pass commands are embedded in the command buffer
            // itself.  No secondary command buffers are executed.
            vkCmdBeginRenderPass( m_commandBuffers[ bufferIndex ], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE );

            // Bind the graphics pipeline.
            vkCmdBindPipeline( m_commandBuffers[ bufferIndex ], VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline );

            // Draw command.
            vkCmdDraw( m_commandBuffers[ bufferIndex ],
                       /*numVerts*/ 3,
                       /*numInstances*/ 1,
                       /*vertOffset*/ 0,
                       /*instanceOffset*/ 0 );

            // End render pass.
            vkCmdEndRenderPass( m_commandBuffers[ bufferIndex ] );

            // End recording.
            if ( vkEndCommandBuffer( m_commandBuffers[ bufferIndex ] ) != VK_SUCCESS )
            {
                throw std::runtime_error( "Failed to record command buffer." );
            }
        }
    }

    void CreateSyncObjects()
    {
        m_imageAvailableSemaphores.resize( s_maxFramesInFlight );
        m_renderFinishedSemaphores.resize( s_maxFramesInFlight );
        m_inFlightFences.resize( s_maxFramesInFlight );
        m_imagesInFlight.resize( m_swapChainImages.size(), VK_NULL_HANDLE );

        VkSemaphoreCreateInfo semaphoreInfo = {};
        semaphoreInfo.sType                 = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo = {};
        fenceInfo.sType             = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        // Create in a signaled state, as if we had rendered an initial frame that finished.
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for ( size_t frameIndex = 0; frameIndex < s_maxFramesInFlight; ++frameIndex )
        {
            if ( vkCreateSemaphore( m_device, &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[ frameIndex ] ) !=
                     VK_SUCCESS ||
                 vkCreateSemaphore( m_device, &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[ frameIndex ] ) !=
                     VK_SUCCESS ||
                 vkCreateFence( m_device, &fenceInfo, nullptr, &m_inFlightFences[ frameIndex ] ) != VK_SUCCESS )
            {
                throw std::runtime_error( "Failed to create synchronization objects." );
            }
        }
    }

    // Initialize the Vulkan instance.
    void InitVulkan()
    {
        CreateVulkanInstance();
        SetupDebugMessenger();
        CreateSurface();
        SelectPhysicalDevice();
        CreateLogicalDevice();
        CreateSwapChain();
        CreateImageViews();
        CreateRenderPass();
        CreateGraphicsPipeline();
        CreateFramebuffers();
        CreateCommandPool();
        CreateCommandBuffers();
        CreateSyncObjects();
    }

    /// Draw a single frame, by submitting the command buffer.
    void DrawFrame()
    {
        // CPU - GPU Synchronization.
        vkWaitForFences( m_device, 1, &m_inFlightFences[ m_currentFrame ], VK_TRUE, UINT64_MAX );

        // Acquire an image from the swap chain.
        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR( m_device,
                                                 m_swapChain,
                                                 /*timeOut*/ UINT64_MAX,
                                                 m_imageAvailableSemaphores[ m_currentFrame ],
                                                 VK_NULL_HANDLE,
                                                 &imageIndex );

        // Is the swap-chain sub-optimal?
        if ( result == VK_ERROR_OUT_OF_DATE_KHR )
        {
            RecreateSwapChain();
            return;
        }
        else if ( result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR )
        {
            throw std::runtime_error( "Failed to acquire swap chain image." );
        }

        // Check if a previous frame is using this image (i.e. there is its fence to wait on)
        if ( m_imagesInFlight[ imageIndex ] != VK_NULL_HANDLE )
        {
            vkWaitForFences( m_device, 1, &m_imagesInFlight[ imageIndex ], VK_TRUE, UINT64_MAX );
        }

        // Mark the image as now being in use by this frame
        m_imagesInFlight[ imageIndex ] = m_inFlightFences[ m_currentFrame ];

        // Submit a command buffer.
        VkSubmitInfo submitInfo      = {};
        submitInfo.sType             = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        VkSemaphore waitSemaphores[] = {m_imageAvailableSemaphores[ m_currentFrame ]}; // Wait until image is acquired.

        // The graphics pipeline will execute up until the the color output stage, to wait on the semaphore.
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

        submitInfo.waitSemaphoreCount = 1; // Number of semaphores to wait on.
        submitInfo.pWaitSemaphores    = waitSemaphores;
        submitInfo.pWaitDstStageMask  = waitStages; // Each entry in waitStages correspond to the semaphore it waits on.

        // Bind the command buffer respective to the swap chain image.
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers    = &m_commandBuffers[ imageIndex ];

        // The semphore to signal once command buffers have finished execution.
        VkSemaphore signalSemaphores[]  = {m_renderFinishedSemaphores[ m_currentFrame ]};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores    = signalSemaphores;

        // Submit to the graphics queue.
        vkResetFences( m_device, 1, &m_inFlightFences[ m_currentFrame ] );
        if ( vkQueueSubmit( m_graphicsQueue, 1, &submitInfo, m_inFlightFences[ m_currentFrame ] ) != VK_SUCCESS )
        {
            throw std::runtime_error( "failed to submit draw command buffer!" );
        }

        // Presentation.
        VkPresentInfoKHR presentInfo   = {};
        presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores    = signalSemaphores; // Presentation shuold wait on m_renderFinishedSemaphore.

        VkSwapchainKHR swapChains[] = {m_swapChain};
        presentInfo.swapchainCount  = 1;
        presentInfo.pSwapchains     = swapChains;
        presentInfo.pImageIndices   = &imageIndex;

        presentInfo.pResults = nullptr; // Optional

        // Present!
        result = vkQueuePresentKHR( m_presentQueue, &presentInfo );
        if ( result == VK_ERROR_OUT_OF_DATE_KHR || result != VK_SUBOPTIMAL_KHR || m_framebufferResized )
        {
            RecreateSwapChain();
            return;
        }
        else if ( result != VK_SUCCESS )
        {
            throw std::runtime_error( "Failed to acquire swap chain image." );
        }

        // Increment frame.
        m_currentFrame = ( m_currentFrame + 1 ) % s_maxFramesInFlight;
    }

    // The main event loop.
    void MainLoop()
    {
        while ( !glfwWindowShouldClose( m_window ) )
        {
            glfwPollEvents();
            DrawFrame();
        }

        // Functions submitted in DrawFrame are asynchronous, so operations may still be in flight.
        // We would like to wait until our device has finished execution.
        vkDeviceWaitIdle( m_device );
    }

    // Teardown internal state, in reverse order of initialization.
    void Teardown()
    {
        TeardownSwapChain();

        for ( size_t frameIndex = 0; frameIndex < s_maxFramesInFlight; ++frameIndex )
        {
            vkDestroySemaphore( m_device, m_renderFinishedSemaphores[ frameIndex ], nullptr );
            vkDestroySemaphore( m_device, m_imageAvailableSemaphores[ frameIndex ], nullptr );
            vkDestroyFence( m_device, m_inFlightFences[ frameIndex ], nullptr );
        }

        vkDestroyCommandPool( m_device, m_commandPool, nullptr );
        vkDestroyDevice( m_device, nullptr );

        if ( m_enableValidationLayers )
        {
            DestroyDebugUtilsMessengerEXT( m_instance, m_debugMessenger, nullptr );
        }

        vkDestroySurfaceKHR( m_instance, m_surface, nullptr );
        vkDestroyInstance( m_instance, nullptr );
        glfwDestroyWindow( m_window );
        glfwTerminate();
    }

    // Path of the executable.
    std::string m_executablePath;

    // Window instance.
    GLFWwindow* m_window = nullptr;

    // Dimensions of the window.
    int m_windowWidth  = 800;
    int m_windowHeight = 600;

    // Title of the window.
    const char* m_windowTitle = "Triangle";

    // Should validation layers be enabled for this app?
    const bool m_enableValidationLayers = true;

    // Available validation layers.
    const std::vector< const char* > m_validationLayers = {"VK_LAYER_KHRONOS_validation"};

    // Device extensions.
    const std::vector< const char* > m_deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    VkInstance               m_instance;                        // Vulkan instance.
    VkDebugUtilsMessengerEXT m_debugMessenger;                  // Debug messenger.
    VkPhysicalDevice         m_physicalDevice = VK_NULL_HANDLE; // The physical device.
    VkDevice                 m_device;                          // The logical device.
    VkQueue                  m_graphicsQueue;                   // The graphics queue.
    VkQueue                  m_presentQueue;                    // Presentation queue.
    VkSurfaceKHR             m_surface;                         // Surface to render on.

    // The swap chain, representing the queue of images to be presented to the screen.
    VkSwapchainKHR m_swapChain;

    // Handles to images in the swap chain.
    std::vector< VkImage >     m_swapChainImages;
    std::vector< VkImageView > m_swapChainImageViews;
    VkFormat                   m_swapChainImageFormat;
    VkExtent2D                 m_swapChainExtent;

    VkRenderPass     m_renderPass;       // Render pass.
    VkPipelineLayout m_pipelineLayout;   // Pipeline layout
    VkPipeline       m_graphicsPipeline; // The handle to the graphics pipeline.

    // Frame buffers representing each VkImageView.
    std::vector< VkFramebuffer > m_swapChainFramebuffers;

    // Command buffer.
    VkCommandPool                  m_commandPool; // Offers creation of CommandBuffers and manages their memory.
    std::vector< VkCommandBuffer > m_commandBuffers;

    // Semaphores for synchronizing frame drawing.
    std::vector< VkSemaphore > m_imageAvailableSemaphores;
    std::vector< VkSemaphore > m_renderFinishedSemaphores;
    std::vector< VkFence >     m_inFlightFences;
    std::vector< VkFence >     m_imagesInFlight;
    size_t                     m_currentFrame = 0;

    // Check if frame buffer requires a resize.
    bool m_framebufferResized = false;
};

int main( int i_argc, char** i_argv )
{
    std::string         executablePath( i_argv[ 0 ] );
    TriangleApplication app( executablePath );

    try
    {
        app.Run();
    }
    catch ( const std::exception& e )
    {
        fprintf( stderr, "Error during runtime: %s.\n", e.what() );
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
