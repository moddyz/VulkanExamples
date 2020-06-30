#pragma once

/// \file vkbase/support.h
///
/// Utilities for checking supported Vulkan features such as layers and extensions.

namespace vkbase
{
/// Check if the validation layers \p i_requestedLayers are supported.
bool CheckVulkanLayersSupport( const std::vector< const char* >& i_requestedLayers )
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
        std::unordered_set< std::string >::const_iterator layerIt = availableLayersSet.find( std::string( layerName ) );

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

/// Check if the requested extensions \p i_requestedExtensions are supported.
bool CheckVulkanExtensionsSupport( const std::vector< const char* >& i_requestedExtensions )
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

} // namespace vkbase
