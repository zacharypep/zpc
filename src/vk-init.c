#include "zpc/vk.h"

#include "zpc/fatal.h"

#include <stdio.h>
#include <string.h>
#include <vulkan/vulkan.h>

// =========================================================================================================================================
// =========================================================================================================================================
// vk_is_validation_supported_z: Check if the Vulkan validation layers are available on this system. Returns true if all required
// validation layers are present.
// =========================================================================================================================================
// =========================================================================================================================================
bool vk_is_validation_supported_z(void)
{
    // =============================================================================================
    // =============================================================================================
    // Get available instance layers
    // =============================================================================================
    // =============================================================================================
    uint32_t layer_count;
    VkLayerProperties avail_layers[256];
    {
        vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
        if (layer_count > 256)
        {
            layer_count = 256;
        }
        vkEnumerateInstanceLayerProperties(&layer_count, avail_layers);
    }

    // =============================================================================================
    // =============================================================================================
    // Check if all required validation layers are present
    // =============================================================================================
    // =============================================================================================
    {
        for (uint32_t i = 0; i < sizeof(VK_VALIDATION_LAYERS) / sizeof(VK_VALIDATION_LAYERS[0]); i++)
        {
            const char* layer_name = VK_VALIDATION_LAYERS[i];
            bool found             = false;
            for (uint32_t j = 0; j < layer_count; j++)
            {
                if (strcmp(layer_name, avail_layers[j].layerName) == 0)
                {
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                return false;
            }
        }
    }

    return true;
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_init_instance_func_ptrs_z: Initialize instance-level function pointers from a VkInstance. If enable_validation is true, loads the
// debug utils messenger creation function.
// =========================================================================================================================================
// =========================================================================================================================================
void vk_init_instance_func_ptrs_z(vk_instance_zt* p_inst, VkInstance vk_instance, bool enable_validation)
{
    fatal_check_z(p_inst, "p_inst is null");
    fatal_check_z(vk_instance, "vk_instance is null");

    if (enable_validation)
    {
        p_inst->func_ptrs.vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vk_instance, "vkCreateDebugUtilsMessengerEXT");
    }
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_init_device_func_ptrs_z: Initialize device-level function pointers from a VkDevice. Loads Vulkan 1.2 extension function pointers if
// using_vk_1_2 is true, and ray tracing extension function pointers if enable_rt is true.
// =========================================================================================================================================
// =========================================================================================================================================
void vk_init_device_func_ptrs_z(vk_instance_zt* p_inst, VkDevice device, bool using_vk_1_2, bool enable_rt)
{
    fatal_check_z(p_inst, "p_inst is null");
    fatal_check_z(device, "device is null");

    p_inst->using_vk_1_2 = using_vk_1_2;

    if (using_vk_1_2)
    {
        p_inst->func_ptrs.vkQueueSubmit2        = (PFN_vkQueueSubmit2KHR)vkGetDeviceProcAddr(device, "vkQueueSubmit2KHR");
        p_inst->func_ptrs.vkCmdPipelineBarrier2 = (PFN_vkCmdPipelineBarrier2KHR)vkGetDeviceProcAddr(device, "vkCmdPipelineBarrier2KHR");
        p_inst->func_ptrs.vkCmdBeginRendering   = (PFN_vkCmdBeginRenderingKHR)vkGetDeviceProcAddr(device, "vkCmdBeginRenderingKHR");
        p_inst->func_ptrs.vkCmdEndRendering     = (PFN_vkCmdEndRenderingKHR)vkGetDeviceProcAddr(device, "vkCmdEndRenderingKHR");
        p_inst->func_ptrs.vkCmdBlitImage2       = (PFN_vkCmdBlitImage2KHR)vkGetDeviceProcAddr(device, "vkCmdBlitImage2KHR");
    }

    if (enable_rt)
    {
        p_inst->func_ptrs.vkCmdBuildAccelerationStructuresKHR        = (PFN_vkCmdBuildAccelerationStructuresKHR)vkGetDeviceProcAddr(device, "vkCmdBuildAccelerationStructuresKHR");
        p_inst->func_ptrs.vkCreateAccelerationStructureKHR           = (PFN_vkCreateAccelerationStructureKHR)vkGetDeviceProcAddr(device, "vkCreateAccelerationStructureKHR");
        p_inst->func_ptrs.vkDestroyAccelerationStructureKHR          = (PFN_vkDestroyAccelerationStructureKHR)vkGetDeviceProcAddr(device, "vkDestroyAccelerationStructureKHR");
        p_inst->func_ptrs.vkGetAccelerationStructureBuildSizesKHR    = (PFN_vkGetAccelerationStructureBuildSizesKHR)vkGetDeviceProcAddr(device, "vkGetAccelerationStructureBuildSizesKHR");
        p_inst->func_ptrs.vkGetAccelerationStructureDeviceAddressKHR = (PFN_vkGetAccelerationStructureDeviceAddressKHR)vkGetDeviceProcAddr(device, "vkGetAccelerationStructureDeviceAddressKHR");
        p_inst->func_ptrs.vkCmdTraceRaysKHR                          = (PFN_vkCmdTraceRaysKHR)vkGetDeviceProcAddr(device, "vkCmdTraceRaysKHR");
        p_inst->func_ptrs.vkGetRayTracingShaderGroupHandlesKHR       = (PFN_vkGetRayTracingShaderGroupHandlesKHR)vkGetDeviceProcAddr(device, "vkGetRayTracingShaderGroupHandlesKHR");
        p_inst->func_ptrs.vkCreateRayTracingPipelinesKHR             = (PFN_vkCreateRayTracingPipelinesKHR)vkGetDeviceProcAddr(device, "vkCreateRayTracingPipelinesKHR");
    }
}

// =========================================================================================================================================
// =========================================================================================================================================
// debug_callback: Internal callback function for Vulkan validation layer messages.
// =========================================================================================================================================
// =========================================================================================================================================
static VkBool32 debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, VkDebugUtilsMessageTypeFlagsEXT message_type, const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data, void* p_user_data)
{
    (void)message_severity;
    (void)message_type;
    (void)p_user_data;

    fprintf(stderr, "[VK] %s\n", p_callback_data->pMessage);
    return VK_FALSE;
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_setup_debug_messenger_z: Set up the Vulkan debug utils messenger for validation layer messages. Fatals if the extension function
// cannot be loaded.
// =========================================================================================================================================
// =========================================================================================================================================
void vk_setup_debug_messenger_z(VkInstance vk_instance, VkDebugUtilsMessengerEXT* p_debug_msgr)
{
    fatal_check_z(vk_instance, "vk_instance is null");
    fatal_check_z(p_debug_msgr, "p_debug_msgr is null");

    PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vk_instance, "vkCreateDebugUtilsMessengerEXT");

    if (!func)
    {
        fatal_z("failed to get vkCreateDebugUtilsMessengerEXT, ext not present?");
    }

    VkDebugUtilsMessengerCreateInfoEXT debug_info  = {};
    debug_info.sType                               = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debug_info.messageSeverity                     = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
    debug_info.messageSeverity                    |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
    debug_info.messageSeverity                    |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debug_info.messageType                         = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT;
    debug_info.messageType                        |= VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
    debug_info.messageType                        |= VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debug_info.pfnUserCallback                     = debug_callback;

    VK_CHECK(func(vk_instance, &debug_info, nullptr, p_debug_msgr), "setting up debug messenger");
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_query_swapchain_support_z: Query swapchain support details for a physical device and surface. Returns capabilities, supported surface
// formats, and supported present modes.
// =========================================================================================================================================
// =========================================================================================================================================
vk_swapchain_support_details_zt vk_query_swapchain_support_z(VkSurfaceKHR surface, VkPhysicalDevice phys_dev)
{
    fatal_check_z(surface, "surface is null");
    fatal_check_z(phys_dev, "phys_dev is null");

    vk_swapchain_support_details_zt details = {};

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(phys_dev, surface, &details.capabilities);

    // =============================================================================================
    // =============================================================================================
    // Get surface formats
    // =============================================================================================
    // =============================================================================================
    {
        uint32_t format_count;
        vkGetPhysicalDeviceSurfaceFormatsKHR(phys_dev, surface, &format_count, nullptr);
        if (format_count != 0)
        {
            if (format_count > VK_MAX_SURFACE_FORMATS)
            {
                format_count = VK_MAX_SURFACE_FORMATS;
            }
            details.format_count = format_count;
            vkGetPhysicalDeviceSurfaceFormatsKHR(phys_dev, surface, &format_count, details.formats);
        }
    }

    // =============================================================================================
    // =============================================================================================
    // Get present modes
    // =============================================================================================
    // =============================================================================================
    {
        uint32_t present_mode_count;
        vkGetPhysicalDeviceSurfacePresentModesKHR(phys_dev, surface, &present_mode_count, nullptr);
        if (present_mode_count != 0)
        {
            if (present_mode_count > VK_MAX_PRESENT_MODES)
            {
                present_mode_count = VK_MAX_PRESENT_MODES;
            }
            details.present_mode_count = present_mode_count;
            vkGetPhysicalDeviceSurfacePresentModesKHR(phys_dev, surface, &present_mode_count, details.present_modes);
        }
    }

    return details;
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_query_queue_families_z: Query queue family indices for a physical device and surface. Returns the graphics and present queue family
// indices if found.
// =========================================================================================================================================
// =========================================================================================================================================
vk_queue_families_details_zt vk_query_queue_families_z(VkPhysicalDevice phys_dev, VkSurfaceKHR surface)
{
    fatal_check_z(phys_dev, "phys_dev is null");
    fatal_check_z(surface, "surface is null");

    vk_queue_families_details_zt indices = {};

    // =============================================================================================
    // =============================================================================================
    // Get queue family properties
    // =============================================================================================
    // =============================================================================================
    uint32_t queue_family_count          = 0;
    VkQueueFamilyProperties queue_families[64];
    {
        vkGetPhysicalDeviceQueueFamilyProperties(phys_dev, &queue_family_count, nullptr);
        if (queue_family_count > 64)
        {
            queue_family_count = 64;
        }
        vkGetPhysicalDeviceQueueFamilyProperties(phys_dev, &queue_family_count, queue_families);
    }

    // =============================================================================================
    // =============================================================================================
    // Find graphics and present queue families
    // =============================================================================================
    // =============================================================================================
    {
        for (uint32_t i = 0; i < queue_family_count; i++)
        {
            if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                indices.graphics_family     = i;
                indices.has_graphics_family = true;
            }

            VkBool32 present_support = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(phys_dev, i, surface, &present_support);
            if (present_support)
            {
                indices.present_family     = i;
                indices.has_present_family = true;
            }

            if (indices.has_graphics_family && indices.has_present_family)
            {
                break;
            }
        }
    }

    return indices;
}

// =========================================================================================================================================
// =========================================================================================================================================
// clamp_u32: Clamp a uint32_t value between min and max.
// =========================================================================================================================================
// =========================================================================================================================================
static uint32_t clamp_u32(uint32_t val, uint32_t min_val, uint32_t max_val)
{
    if (val < min_val)
    {
        return min_val;
    }
    if (val > max_val)
    {
        return max_val;
    }
    return val;
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_create_swapchain_z: Create a Vulkan swapchain with the specified parameters. Queries swapchain support and selects the best available
// format, present mode, and extent. Outputs the swapchain handle, images, format, and extent.
// =========================================================================================================================================
// =========================================================================================================================================
void vk_create_swapchain_z(
    VkDevice vk_dev,
    VkSurfaceKHR vk_surface,
    VkPhysicalDevice vk_phys_dev,
    VkFormat swapchain_ideal_format,
    VkColorSpaceKHR swapchain_ideal_colour_space,
    VkPresentModeKHR swapchain_ideal_present_mode,
    VkPresentModeKHR swapchain_fallback_present_mode,
    uint32_t framebuffer_width,
    uint32_t framebuffer_height,
    VkSwapchainKHR* p_vk_swapchain,
    VkImage* p_out_swapchain_imgs,
    uint32_t* p_out_swapchain_img_count,
    uint32_t swapchain_imgs_capacity,
    VkFormat* p_out_swapchain_format,
    VkExtent2D* p_out_swapchain_extent
)
{
    fatal_check_z(vk_dev, "vk_dev is null");
    fatal_check_z(vk_surface, "vk_surface is null");
    fatal_check_z(vk_phys_dev, "vk_phys_dev is null");
    fatal_check_z(p_vk_swapchain, "p_vk_swapchain is null");
    fatal_check_z(p_out_swapchain_imgs, "p_out_swapchain_imgs is null");
    fatal_check_z(p_out_swapchain_img_count, "p_out_swapchain_img_count is null");
    fatal_check_z(p_out_swapchain_format, "p_out_swapchain_format is null");
    fatal_check_z(p_out_swapchain_extent, "p_out_swapchain_extent is null");

    vk_swapchain_support_details_zt swapchain_details = vk_query_swapchain_support_z(vk_surface, vk_phys_dev);

    // =============================================================================================
    // =============================================================================================
    // Swapchain format
    // =============================================================================================
    // =============================================================================================
    VkSurfaceFormatKHR surface_format;
    {
        surface_format = swapchain_details.formats[0];

        for (uint32_t i = 0; i < swapchain_details.format_count; i++)
        {
            if (swapchain_details.formats[i].format == swapchain_ideal_format && swapchain_details.formats[i].colorSpace == swapchain_ideal_colour_space)
            {
                surface_format = swapchain_details.formats[i];
                break;
            }
        }
    }

    // =============================================================================================
    // =============================================================================================
    // Swapchain present mode
    // =============================================================================================
    // =============================================================================================
    VkPresentModeKHR present_mode;
    {
        present_mode = swapchain_fallback_present_mode;
        for (uint32_t i = 0; i < swapchain_details.present_mode_count; i++)
        {
            if (swapchain_details.present_modes[i] == swapchain_ideal_present_mode)
            {
                present_mode = swapchain_ideal_present_mode;
                break;
            }
        }
    }

    // =============================================================================================
    // =============================================================================================
    // Swapchain extent
    // =============================================================================================
    // =============================================================================================
    VkExtent2D extent;
    {
        if (swapchain_details.capabilities.currentExtent.width != UINT32_MAX)
        {
            extent = swapchain_details.capabilities.currentExtent;
        }
        else
        {
            extent.width  = clamp_u32(framebuffer_width, swapchain_details.capabilities.minImageExtent.width, swapchain_details.capabilities.maxImageExtent.width);
            extent.height = clamp_u32(framebuffer_height, swapchain_details.capabilities.minImageExtent.height, swapchain_details.capabilities.maxImageExtent.height);
        }
    }

    // =============================================================================================
    // =============================================================================================
    // Swapchain image count
    // =============================================================================================
    // =============================================================================================
    uint32_t image_count;
    {
        image_count = swapchain_details.capabilities.minImageCount + 1;
        if (swapchain_details.capabilities.maxImageCount > 0 && image_count > swapchain_details.capabilities.maxImageCount)
        {
            image_count = swapchain_details.capabilities.maxImageCount;
        }
    }

    // =============================================================================================
    // =============================================================================================
    // Queue families and sharing mode
    // =============================================================================================
    // =============================================================================================
    VkSharingMode sharing_mode;
    uint32_t queue_family_idx_count;
    uint32_t indices[2];
    {
        vk_queue_families_details_zt details      = vk_query_queue_families_z(vk_phys_dev, vk_surface);

        indices[0]                                = details.graphics_family;
        indices[1]                                = details.present_family;

        bool separate_graphics_and_present_family = details.graphics_family != details.present_family;
        if (separate_graphics_and_present_family)
        {
            sharing_mode           = VK_SHARING_MODE_CONCURRENT;
            queue_family_idx_count = 2;
        }
        else
        {
            sharing_mode           = VK_SHARING_MODE_EXCLUSIVE;
            queue_family_idx_count = 0;
        }
    }

    // =============================================================================================
    // =============================================================================================
    // Create swapchain
    // =============================================================================================
    // =============================================================================================
    {
        VkSwapchainCreateInfoKHR info = {};
        info.sType                    = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        info.surface                  = vk_surface;
        info.minImageCount            = image_count;
        info.imageFormat              = surface_format.format;
        info.imageColorSpace          = surface_format.colorSpace;
        info.imageExtent              = extent;
        info.imageArrayLayers         = 1;
        info.imageUsage               = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        info.preTransform             = swapchain_details.capabilities.currentTransform;
        info.compositeAlpha           = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        info.presentMode              = present_mode;
        info.clipped                  = VK_TRUE;
        info.imageSharingMode         = sharing_mode;
        info.queueFamilyIndexCount    = queue_family_idx_count;
        info.pQueueFamilyIndices      = indices;

        VK_CHECK(vkCreateSwapchainKHR(vk_dev, &info, nullptr, p_vk_swapchain), "creating swapchain");
    }

    // =============================================================================================
    // =============================================================================================
    // Get swapchain images
    // =============================================================================================
    // =============================================================================================
    {
        uint32_t actual_image_count;
        vkGetSwapchainImagesKHR(vk_dev, *p_vk_swapchain, &actual_image_count, nullptr);
        if (actual_image_count > swapchain_imgs_capacity)
        {
            fatal_z("swapchain image count %u exceeds capacity %u", actual_image_count, swapchain_imgs_capacity);
        }
        vkGetSwapchainImagesKHR(vk_dev, *p_vk_swapchain, &actual_image_count, p_out_swapchain_imgs);
        *p_out_swapchain_img_count = actual_image_count;
        *p_out_swapchain_format    = surface_format.format;
        *p_out_swapchain_extent    = extent;
    }
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_init_2_z: Initialize the instance's descriptor system, compute pass, graphics pass, and RT pass.
// =========================================================================================================================================
// =========================================================================================================================================
void vk_init_2_z(vk_instance_zt* p_inst, size_t max_textures, size_t max_fonts)
{
    fatal_check_z(p_inst, "p_inst is null");

    vk_descriptors_init_z(p_inst->vk_dev, p_inst->vk_phys_dev, &p_inst->desc_sys, max_textures, max_fonts);
    vk_compute_pass_init_z(p_inst);
    vk_graphics_pass_init_z(p_inst);
    vk_rt_pass_init_z(p_inst);
    vk_tex_sys_init_z(p_inst);
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_get_as_dev_addr_z: Get the device address of an acceleration structure for use in shader device address operations.
// =========================================================================================================================================
// =========================================================================================================================================
VkDeviceAddress vk_get_as_dev_addr_z(vk_instance_zt* p_inst, VkDevice device, VkAccelerationStructureKHR as)
{
    fatal_check_z(p_inst, "p_inst is null");
    fatal_check_z(device, "device is null");
    fatal_check_z(as, "as is null");

    VkAccelerationStructureDeviceAddressInfoKHR info = {};
    info.sType                                       = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
    info.accelerationStructure                       = as;
    return p_inst->func_ptrs.vkGetAccelerationStructureDeviceAddressKHR(device, &info);
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_get_candidate_phys_devs_z: Get all physical devices that support Vulkan. Writes to the output array and returns the count. Fatals if
// no devices are found.
// =========================================================================================================================================
// =========================================================================================================================================
uint32_t vk_get_candidate_phys_devs_z(VkInstance vk_instance, VkPhysicalDevice* p_out_devices, uint32_t capacity)
{
    fatal_check_z(vk_instance, "vk_instance is null");
    fatal_check_z(p_out_devices, "p_out_devices is null");
    fatal_check_bool_z(capacity > 0, "capacity must be > 0");

    uint32_t count = 0;
    vkEnumeratePhysicalDevices(vk_instance, &count, nullptr);

    if (count == 0)
    {
        fatal_z("failed to find GPUs with Vulkan support!");
    }

    if (count > capacity)
    {
        fatal_z("physical device count %u exceeds capacity %u", count, capacity);
    }

    vkEnumeratePhysicalDevices(vk_instance, &count, p_out_devices);

    return count;
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_record_trans_image_layout_z: Record an image layout transition command into the command buffer.
// =========================================================================================================================================
// =========================================================================================================================================
void vk_record_trans_image_layout_z(vk_instance_zt* p_inst, VkCommandBuffer cmd_buff, VkImage image, VkFormat format, VkImageLayout prev_layout, VkImageLayout next_layout)
{
    fatal_check_z(p_inst, "p_inst is null");
    fatal_check_z(cmd_buff, "cmd_buff is null");
    fatal_check_z(image, "image is null");

    VkImageAspectFlags aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT;
    switch (format)
    {
        case VK_FORMAT_D16_UNORM:
        case VK_FORMAT_D32_SFLOAT:
        case VK_FORMAT_X8_D24_UNORM_PACK32: aspect_mask = VK_IMAGE_ASPECT_DEPTH_BIT; break;
        case VK_FORMAT_S8_UINT:             aspect_mask = VK_IMAGE_ASPECT_STENCIL_BIT; break;
        case VK_FORMAT_D16_UNORM_S8_UINT:
        case VK_FORMAT_D24_UNORM_S8_UINT:
        case VK_FORMAT_D32_SFLOAT_S8_UINT:  aspect_mask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT; break;
        default:                            break;
    }

    VkImageSubresourceRange subresource_range = {};
    subresource_range.aspectMask              = aspect_mask;
    subresource_range.baseMipLevel            = 0;
    subresource_range.levelCount              = VK_REMAINING_MIP_LEVELS;
    subresource_range.baseArrayLayer          = 0;
    subresource_range.layerCount              = VK_REMAINING_ARRAY_LAYERS;

    VkImageMemoryBarrier2 image_barrier       = {};
    image_barrier.sType                       = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    image_barrier.srcStageMask                = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    image_barrier.srcAccessMask               = VK_ACCESS_2_MEMORY_WRITE_BIT;
    image_barrier.dstStageMask                = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    image_barrier.dstAccessMask               = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;
    image_barrier.oldLayout                   = prev_layout;
    image_barrier.newLayout                   = next_layout;
    image_barrier.srcQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
    image_barrier.dstQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
    image_barrier.subresourceRange            = subresource_range;
    image_barrier.image                       = image;
    image_barrier.pNext                       = nullptr;

    VkDependencyInfo dep_info                 = {};
    dep_info.sType                            = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dep_info.imageMemoryBarrierCount          = 1;
    dep_info.pImageMemoryBarriers             = &image_barrier;
    dep_info.pNext                            = nullptr;

    if (p_inst->using_vk_1_2)
    {
        p_inst->func_ptrs.vkCmdPipelineBarrier2(cmd_buff, &dep_info);
    }
    else
    {
        vkCmdPipelineBarrier2(cmd_buff, &dep_info);
    }
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_record_blit_z: Record a blit operation from source to destination image with the specified sizes.
// =========================================================================================================================================
// =========================================================================================================================================
void vk_record_blit_z(vk_instance_zt* p_inst, VkCommandBuffer cmd_buff, VkImage src, VkImage dst, VkExtent2D src_size, VkExtent2D dst_size)
{
    fatal_check_z(p_inst, "p_inst is null");
    fatal_check_z(cmd_buff, "cmd_buff is null");
    fatal_check_z(src, "src is null");
    fatal_check_z(dst, "dst is null");

    VkImageBlit2 blit_2                  = {};
    blit_2.sType                         = VK_STRUCTURE_TYPE_IMAGE_BLIT_2;
    blit_2.srcOffsets[1].x               = (int32_t)src_size.width;
    blit_2.srcOffsets[1].y               = (int32_t)src_size.height;
    blit_2.srcOffsets[1].z               = 1;
    blit_2.dstOffsets[1].x               = (int32_t)dst_size.width;
    blit_2.dstOffsets[1].y               = (int32_t)dst_size.height;
    blit_2.dstOffsets[1].z               = 1;
    blit_2.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    blit_2.srcSubresource.baseArrayLayer = 0;
    blit_2.srcSubresource.layerCount     = 1;
    blit_2.srcSubresource.mipLevel       = 0;
    blit_2.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    blit_2.dstSubresource.baseArrayLayer = 0;
    blit_2.dstSubresource.layerCount     = 1;
    blit_2.dstSubresource.mipLevel       = 0;
    blit_2.pNext                         = nullptr;

    VkBlitImageInfo2 info                = {};
    info.sType                           = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2;
    info.dstImage                        = dst;
    info.dstImageLayout                  = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    info.srcImage                        = src;
    info.srcImageLayout                  = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    info.filter                          = VK_FILTER_LINEAR;
    info.regionCount                     = 1;
    info.pRegions                        = &blit_2;
    info.pNext                           = nullptr;

    if (p_inst->using_vk_1_2)
    {
        p_inst->func_ptrs.vkCmdBlitImage2(cmd_buff, &info);
    }
    else
    {
        vkCmdBlitImage2(cmd_buff, &info);
    }
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_record_buff_barrier_z: Record a buffer memory barrier into the command buffer.
// =========================================================================================================================================
// =========================================================================================================================================
void vk_record_buff_barrier_z(vk_instance_zt* p_inst, VkDevice device, VkCommandBuffer cmd_buff, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size)
{
    fatal_check_z(p_inst, "p_inst is null");
    fatal_check_z(device, "device is null");
    fatal_check_z(cmd_buff, "cmd_buff is null");
    fatal_check_z(buffer, "buffer is null");

    VkBufferMemoryBarrier2 barrier    = {};
    barrier.sType                     = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
    barrier.srcStageMask              = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    barrier.srcAccessMask             = VK_ACCESS_2_MEMORY_WRITE_BIT;
    barrier.dstStageMask              = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    barrier.dstAccessMask             = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;
    barrier.buffer                    = buffer;
    barrier.offset                    = offset;
    barrier.size                      = size;
    barrier.pNext                     = nullptr;

    VkDependencyInfo dep_info         = {};
    dep_info.sType                    = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dep_info.bufferMemoryBarrierCount = 1;
    dep_info.pBufferMemoryBarriers    = &barrier;
    dep_info.pNext                    = nullptr;

    if (p_inst->using_vk_1_2)
    {
        p_inst->func_ptrs.vkCmdPipelineBarrier2(cmd_buff, &dep_info);
    }
    else
    {
        vkCmdPipelineBarrier2(cmd_buff, &dep_info);
    }
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_sampler_init_z: Initialize a sampler with linear filtering, mipmapping, and anisotropic filtering enabled.
// =========================================================================================================================================
// =========================================================================================================================================
void vk_sampler_init_z(VkDevice device, vk_sampler_zt* p_sampler)
{
    fatal_check_z(device, "device is null");
    fatal_check_z(p_sampler, "p_sampler is null");

    VkSamplerCreateInfo info = {};
    info.sType               = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    info.minFilter           = VK_FILTER_LINEAR;
    info.magFilter           = VK_FILTER_LINEAR;
    info.mipmapMode          = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    info.addressModeU        = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    info.addressModeV        = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    info.anisotropyEnable    = VK_TRUE;
    info.maxAnisotropy       = 16.0f;
    info.minLod              = 0.f;
    info.maxLod              = 15.f;
    info.mipLodBias          = 0.f;

    VK_CHECK(vkCreateSampler(device, &info, nullptr, &p_sampler->handle), "creating sampler");
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_sampler_desc_info_z: Get descriptor image info containing just the sampler handle.
// =========================================================================================================================================
// =========================================================================================================================================
VkDescriptorImageInfo vk_sampler_desc_info_z(const vk_sampler_zt* p_sampler)
{
    fatal_check_z(p_sampler, "p_sampler is null");

    VkDescriptorImageInfo info = {};
    info.sampler               = p_sampler->handle;

    return info;
}
