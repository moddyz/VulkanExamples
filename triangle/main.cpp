#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstdlib>
#include <iostream>
#include <optional>
#include <set>
#include <stdexcept>
#include <unordered_set>
#include <vector>

/// \class TriangleApplication
///
/// A simple app which draws a triangle using the Vulkan API, in a window.
class TriangleApplication
{
public:
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

        int graphicsFamilyCount = 0;
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

    VkExtent2D SelectSwapExtent( const VkSurfaceCapabilitiesKHR& i_capabilities ) const
    {
        if ( i_capabilities.currentExtent.width != UINT32_MAX )
        {
            return i_capabilities.currentExtent;
        }
        else
        {
            VkExtent2D actualExtent = {m_windowWidth, m_windowHeight};

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

            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

            createInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel   = 0;
            createInfo.subresourceRange.levelCount     = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount     = 1;

            if ( vkCreateImageView( m_device, &createInfo, nullptr, &m_swapChainImageViews[ imageIndex ] ) !=
                 VK_SUCCESS )
            {
                throw std::runtime_error( "failed to create image views!" );
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
    }

    // The main event loop.
    void MainLoop()
    {
        while ( !glfwWindowShouldClose( m_window ) )
        {
            glfwPollEvents();
        }
    }

    // Teardown internal state, in reverse order of initiailization.
    void Teardown()
    {
        for ( VkImageView imageView : m_swapChainImageViews )
        {
            vkDestroyImageView( m_device, imageView, nullptr );
        }

        vkDestroySwapchainKHR( m_device, m_swapChain, nullptr );
        vkDestroyDevice( m_device, nullptr );
        vkDestroySurfaceKHR( m_instance, m_surface, nullptr );
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
    const uint32_t m_windowWidth  = 800;
    const uint32_t m_windowHeight = 600;

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
};

int main()
{
    TriangleApplication app;

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
