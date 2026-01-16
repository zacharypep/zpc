#include "zpc/vk.h"

#include "zpc/fatal.h"

#include <vulkan/vulkan.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
static inline float half_to_float(uint16_t h)
{
    uint32_t sign     = (h >> 15) & 0x1;
    uint32_t exponent = (h >> 10) & 0x1F;
    uint32_t mantissa = h & 0x3FF;

    uint32_t f;
    if (exponent == 0)
    {
        if (mantissa == 0)
        {
            f = sign << 31;
        }
        else
        {
            exponent = 1;
            while ((mantissa & 0x400) == 0)
            {
                mantissa <<= 1;
                exponent--;
            }
            mantissa &= 0x3FF;
            f         = (sign << 31) | ((exponent + 127 - 15) << 23) | (mantissa << 13);
        }
    }
    else if (exponent == 31)
    {
        f = (sign << 31) | 0x7F800000 | (mantissa << 13);
    }
    else
    {
        f = (sign << 31) | ((exponent + 127 - 15) << 23) | (mantissa << 13);
    }

    float result;
    memcpy(&result, &f, sizeof(float));
    return result;
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_device_image_init_z: Initialize a device-local image with the specified dimensions, format, and usage flags. Creates image, allocates
// memory, and creates image view. The image must be cleaned up with vk_device_image_cleanup_z.
// =========================================================================================================================================
// =========================================================================================================================================
void vk_device_image_init_z(
    VkDevice device, VkPhysicalDevice phys_dev, vk_device_image_zt* p_img, uint32_t width, uint32_t height, uint32_t num_channels, size_t pixel_size, uint32_t mip_levels, VkFormat format, VkImageUsageFlags usage, VkImageAspectFlags flags, const uint32_t* p_queue_families, uint32_t queue_family_count
)
{
    fatal_check_z(device, "device is null");
    fatal_check_z(phys_dev, "phys_dev is null");
    fatal_check_z(p_img, "p_img is null");
    fatal_check_bool_z(width > 0, "width must be > 0");
    fatal_check_bool_z(height > 0, "height must be > 0");
    fatal_check_bool_z(num_channels > 0, "num_channels must be > 0");
    fatal_check_bool_z(pixel_size > 0, "pixel_size must be > 0");
    fatal_check_bool_z(mip_levels > 0, "mip_levels must be > 0");

    p_img->device       = device;
    p_img->width        = width;
    p_img->height       = height;
    p_img->num_channels = num_channels;
    p_img->pixel_size   = pixel_size;
    p_img->mip_levels   = mip_levels;
    p_img->format       = format;

    // =============================================================================================
    // =============================================================================================
    // create image
    // =============================================================================================
    // =============================================================================================
    {
        VkSharingMode sharing_mode;
        {
            sharing_mode = queue_family_count > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
        }

        VkImageCreateInfo info     = {};
        info.sType                 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        info.imageType             = VK_IMAGE_TYPE_2D;
        info.extent.width          = width;
        info.extent.height         = height;
        info.extent.depth          = 1;
        info.mipLevels             = mip_levels;
        info.arrayLayers           = 1;
        info.format                = format;
        info.tiling                = VK_IMAGE_TILING_OPTIMAL;
        info.initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED;
        info.usage                 = usage;
        info.samples               = VK_SAMPLE_COUNT_1_BIT;
        info.sharingMode           = sharing_mode;
        info.queueFamilyIndexCount = queue_family_count;
        info.pQueueFamilyIndices   = p_queue_families;

        VK_CHECK(vkCreateImage(device, &info, nullptr, &p_img->handle), "creating image");
    }

    // =============================================================================================
    // =============================================================================================
    // allocate and bind memory
    // =============================================================================================
    // =============================================================================================
    {
        VkMemoryRequirements mem_require;
        vkGetImageMemoryRequirements(device, p_img->handle, &mem_require);

        VkMemoryPropertyFlags mem_props           = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

        uint32_t memory_type_idx                  = vk_find_memory_type_z(phys_dev, mem_require.memoryTypeBits, mem_props);

        VkMemoryPriorityAllocateInfoEXT prio_info = {};
        prio_info.sType                           = VK_STRUCTURE_TYPE_MEMORY_PRIORITY_ALLOCATE_INFO_EXT;
        prio_info.priority                        = 1.0f;

        VkMemoryAllocateInfo alloc_info           = {};
        alloc_info.sType                          = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.allocationSize                 = mem_require.size;
        alloc_info.memoryTypeIndex                = memory_type_idx;
        alloc_info.pNext                          = &prio_info;

        VK_CHECK(vkAllocateMemory(device, &alloc_info, nullptr, &p_img->memory), "allocating image memory");

        VK_CHECK(vkBindImageMemory(device, p_img->handle, p_img->memory, 0), "binding image memory");
    }

    // =============================================================================================
    // =============================================================================================
    // create view
    // =============================================================================================
    // =============================================================================================
    {
        VkImageViewCreateInfo info           = {};
        info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        info.image                           = p_img->handle;
        info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
        info.format                          = format;
        info.subresourceRange.aspectMask     = flags;
        info.subresourceRange.baseMipLevel   = 0;
        info.subresourceRange.levelCount     = mip_levels;
        info.subresourceRange.baseArrayLayer = 0;
        info.subresourceRange.layerCount     = 1;

        VK_CHECK(vkCreateImageView(device, &info, nullptr, &p_img->view), "creating image view");
    }

    p_img->is_init = true;
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_device_image_desc_info_z: Get descriptor image info for use in descriptor sets.
// =========================================================================================================================================
// =========================================================================================================================================
VkDescriptorImageInfo vk_device_image_desc_info_z(const vk_device_image_zt* p_img, VkImageLayout img_layout)
{
    fatal_check_z(p_img, "p_img is null");

    VkDescriptorImageInfo info = {};
    info.imageView             = p_img->view;
    info.sampler               = VK_NULL_HANDLE;
    info.imageLayout           = img_layout;

    return info;
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_device_image_cleanup_z: Clean up device-local image resources. Destroys image view, image, and frees memory.
// =========================================================================================================================================
// =========================================================================================================================================
void vk_device_image_cleanup_z(vk_device_image_zt* p_img)
{
    fatal_check_z(p_img, "p_img is null");

    p_img->is_init = false;

    vkDestroyImageView(p_img->device, p_img->view, nullptr);
    vkDestroyImage(p_img->device, p_img->handle, nullptr);
    vkFreeMemory(p_img->device, p_img->memory, nullptr);
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_device_image_copy_to_rgba8_z: Copy image data to an RGBA8 buffer. Returns true on success, false on failure. Supports various source
// formats including R8G8B8A8, B8G8R8A8, R8, R16G16B16A16_SFLOAT, R32G32B32A32_SFLOAT, and D32_SFLOAT.
// =========================================================================================================================================
// =========================================================================================================================================
bool vk_device_image_copy_to_rgba8_z(const vk_device_image_zt* p_img, VkPhysicalDevice phys_dev, VkQueue queue, VkCommandPool cmd_pool, VkImageLayout current_layout, uint8_t* out_buffer, uint32_t buffer_capacity)
{
    fatal_check_z(p_img, "p_img is null");
    fatal_check_z(phys_dev, "phys_dev is null");
    fatal_check_z(queue, "queue is null");
    fatal_check_z(cmd_pool, "cmd_pool is null");
    fatal_check_z(out_buffer, "out_buffer is null");

    // =============================================================================================
    // =============================================================================================
    // guard: validate image state and buffer capacity
    // =============================================================================================
    // =============================================================================================
    {
        if (p_img->handle == VK_NULL_HANDLE || p_img->width == 0 || p_img->height == 0)
        {
            return false;
        }

        const uint32_t required_capacity = p_img->width * p_img->height * 4;
        if (buffer_capacity < required_capacity)
        {
            return false;
        }
    }

    // =============================================================================================
    // =============================================================================================
    // determine source format properties
    // =============================================================================================
    // =============================================================================================
    uint32_t src_channels;
    uint32_t src_bytes_per_channel;
    {
        switch (p_img->format)
        {
            case VK_FORMAT_R8G8B8A8_SRGB:
            case VK_FORMAT_R8G8B8A8_UNORM:
            case VK_FORMAT_B8G8R8A8_SRGB:
            case VK_FORMAT_B8G8R8A8_UNORM:
                src_channels          = 4;
                src_bytes_per_channel = 1;
                break;
            case VK_FORMAT_R8_UNORM:
                src_channels          = 1;
                src_bytes_per_channel = 1;
                break;
            case VK_FORMAT_R16G16B16A16_SFLOAT:
                src_channels          = 4;
                src_bytes_per_channel = 2;
                break;
            case VK_FORMAT_R32G32B32A32_SFLOAT:
                src_channels          = 4;
                src_bytes_per_channel = 4;
                break;
            case VK_FORMAT_D32_SFLOAT:
                src_channels          = 1;
                src_bytes_per_channel = 4;
                break;
            default: return false;
        }
    }

    const uint32_t src_bpp  = src_channels * src_bytes_per_channel;
    const uint32_t src_size = p_img->width * p_img->height * src_bpp;

    // =============================================================================================
    // =============================================================================================
    // create staging buffer
    // =============================================================================================
    // =============================================================================================
    VkBuffer staging_buffer;
    VkDeviceMemory staging_memory;
    {
        VkBufferCreateInfo buffer_info = {};
        buffer_info.sType              = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_info.size               = src_size;
        buffer_info.usage              = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        buffer_info.sharingMode        = VK_SHARING_MODE_EXCLUSIVE;

        VK_CHECK(vkCreateBuffer(p_img->device, &buffer_info, nullptr, &staging_buffer), "creating staging buffer");

        VkMemoryRequirements mem_requirements;
        vkGetBufferMemoryRequirements(p_img->device, staging_buffer, &mem_requirements);

        VkMemoryAllocateInfo alloc_info = {};
        alloc_info.sType                = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.allocationSize       = mem_requirements.size;
        alloc_info.memoryTypeIndex      = vk_find_memory_type_z(phys_dev, mem_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        VK_CHECK(vkAllocateMemory(p_img->device, &alloc_info, nullptr, &staging_memory), "allocating staging memory");
        VK_CHECK(vkBindBufferMemory(p_img->device, staging_buffer, staging_memory, 0), "binding staging memory");
    }

    // =============================================================================================
    // =============================================================================================
    // record and submit copy commands
    // =============================================================================================
    // =============================================================================================
    {
        VkCommandBufferAllocateInfo alloc_info = {};
        alloc_info.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        alloc_info.commandPool                 = cmd_pool;
        alloc_info.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        alloc_info.commandBufferCount          = 1;

        VkCommandBuffer cmd_buff;
        VK_CHECK(vkAllocateCommandBuffers(p_img->device, &alloc_info, &cmd_buff), "allocating command buffer");

        VkCommandBufferBeginInfo begin_info = {};
        begin_info.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags                    = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(cmd_buff, &begin_info);

        // =========================================================================================
        // =========================================================================================
        // transition to transfer src layout
        // =========================================================================================
        // =========================================================================================
        {
            VkImageMemoryBarrier barrier            = {};
            barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout                       = current_layout;
            barrier.newLayout                       = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
            barrier.image                           = p_img->handle;
            barrier.subresourceRange.aspectMask     = (p_img->format == VK_FORMAT_D32_SFLOAT) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseMipLevel   = 0;
            barrier.subresourceRange.levelCount     = 1;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount     = 1;
            barrier.srcAccessMask                   = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
            barrier.dstAccessMask                   = VK_ACCESS_TRANSFER_READ_BIT;

            vkCmdPipelineBarrier(cmd_buff, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
        }

        // =========================================================================================
        // =========================================================================================
        // copy image to buffer
        // =========================================================================================
        // =========================================================================================
        {
            VkBufferImageCopy region               = {};
            region.bufferOffset                    = 0;
            region.bufferRowLength                 = 0;
            region.bufferImageHeight               = 0;
            region.imageSubresource.aspectMask     = (p_img->format == VK_FORMAT_D32_SFLOAT) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.mipLevel       = 0;
            region.imageSubresource.baseArrayLayer = 0;
            region.imageSubresource.layerCount     = 1;
            region.imageOffset                     = (VkOffset3D){0, 0, 0};
            region.imageExtent                     = (VkExtent3D){p_img->width, p_img->height, 1};

            vkCmdCopyImageToBuffer(cmd_buff, p_img->handle, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, staging_buffer, 1, &region);
        }

        // =========================================================================================
        // =========================================================================================
        // transition back to original layout
        // =========================================================================================
        // =========================================================================================
        {
            VkImageMemoryBarrier barrier            = {};
            barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout                       = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.newLayout                       = current_layout;
            barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
            barrier.image                           = p_img->handle;
            barrier.subresourceRange.aspectMask     = (p_img->format == VK_FORMAT_D32_SFLOAT) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseMipLevel   = 0;
            barrier.subresourceRange.levelCount     = 1;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount     = 1;
            barrier.srcAccessMask                   = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask                   = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;

            vkCmdPipelineBarrier(cmd_buff, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
        }

        vkEndCommandBuffer(cmd_buff);

        VkSubmitInfo submit_info       = {};
        submit_info.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers    = &cmd_buff;

        vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE);
        vkQueueWaitIdle(queue);

        vkFreeCommandBuffers(p_img->device, cmd_pool, 1, &cmd_buff);
    }

    // =============================================================================================
    // =============================================================================================
    // map memory and convert format to RGBA8
    // =============================================================================================
    // =============================================================================================
    {
        void* gpu_data;
        vkMapMemory(p_img->device, staging_memory, 0, src_size, 0, &gpu_data);

        const uint8_t* src = (const uint8_t*)gpu_data;

        for (uint32_t y = 0; y < p_img->height; ++y)
        {
            for (uint32_t x = 0; x < p_img->width; ++x)
            {
                uint32_t src_idx = (y * p_img->width + x) * src_bpp;
                uint32_t dst_idx = (y * p_img->width + x) * 4;

                switch (p_img->format)
                {
                    case VK_FORMAT_R8G8B8A8_SRGB:
                    case VK_FORMAT_R8G8B8A8_UNORM:
                    {
                        out_buffer[dst_idx + 0] = src[src_idx + 0];
                        out_buffer[dst_idx + 1] = src[src_idx + 1];
                        out_buffer[dst_idx + 2] = src[src_idx + 2];
                        out_buffer[dst_idx + 3] = src[src_idx + 3];
                    }
                    break;

                    case VK_FORMAT_B8G8R8A8_SRGB:
                    case VK_FORMAT_B8G8R8A8_UNORM:
                    {
                        out_buffer[dst_idx + 0] = src[src_idx + 2];
                        out_buffer[dst_idx + 1] = src[src_idx + 1];
                        out_buffer[dst_idx + 2] = src[src_idx + 0];
                        out_buffer[dst_idx + 3] = src[src_idx + 3];
                    }
                    break;

                    case VK_FORMAT_R8_UNORM:
                    {
                        uint8_t val             = src[src_idx];
                        out_buffer[dst_idx + 0] = val;
                        out_buffer[dst_idx + 1] = val;
                        out_buffer[dst_idx + 2] = val;
                        out_buffer[dst_idx + 3] = 255;
                    }
                    break;

                    case VK_FORMAT_R16G16B16A16_SFLOAT:
                    {
                        const uint16_t* src16   = (const uint16_t*)(src + src_idx);
                        float r                 = half_to_float(src16[0]);
                        float g                 = half_to_float(src16[1]);
                        float b                 = half_to_float(src16[2]);
                        float a                 = half_to_float(src16[3]);

                        out_buffer[dst_idx + 0] = (uint8_t)(fminf(fmaxf(r, 0.0f), 1.0f) * 255.0f);
                        out_buffer[dst_idx + 1] = (uint8_t)(fminf(fmaxf(g, 0.0f), 1.0f) * 255.0f);
                        out_buffer[dst_idx + 2] = (uint8_t)(fminf(fmaxf(b, 0.0f), 1.0f) * 255.0f);
                        out_buffer[dst_idx + 3] = (uint8_t)(fminf(fmaxf(a, 0.0f), 1.0f) * 255.0f);
                    }
                    break;

                    case VK_FORMAT_R32G32B32A32_SFLOAT:
                    {
                        const float* src32      = (const float*)(src + src_idx);
                        float r                 = src32[0];
                        float g                 = src32[1];
                        float b                 = src32[2];
                        float a                 = src32[3];

                        out_buffer[dst_idx + 0] = (uint8_t)(fminf(fmaxf(fmodf(fabsf(r), 1.0f), 0.0f), 1.0f) * 255.0f);
                        out_buffer[dst_idx + 1] = (uint8_t)(fminf(fmaxf(fmodf(fabsf(g), 1.0f), 0.0f), 1.0f) * 255.0f);
                        out_buffer[dst_idx + 2] = (uint8_t)(fminf(fmaxf(fmodf(fabsf(b), 1.0f), 0.0f), 1.0f) * 255.0f);
                        out_buffer[dst_idx + 3] = (uint8_t)(fminf(fmaxf(a, 0.0f), 1.0f) * 255.0f);
                    }
                    break;

                    case VK_FORMAT_D32_SFLOAT:
                    {
                        const float* src32      = (const float*)(src + src_idx);
                        float depth             = src32[0];
                        uint8_t val             = (uint8_t)(fminf(fmaxf(depth, 0.0f), 1.0f) * 255.0f);

                        out_buffer[dst_idx + 0] = val;
                        out_buffer[dst_idx + 1] = val;
                        out_buffer[dst_idx + 2] = val;
                        out_buffer[dst_idx + 3] = 255;
                    }
                    break;

                    default: break;
                }
            }
        }

        vkUnmapMemory(p_img->device, staging_memory);
    }

    // =============================================================================================
    // =============================================================================================
    // cleanup staging resources
    // =============================================================================================
    // =============================================================================================
    {
        vkDestroyBuffer(p_img->device, staging_buffer, nullptr);
        vkFreeMemory(p_img->device, staging_memory, nullptr);
    }

    return true;
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_device_local_image_array_init_z: Initialize a device-local image array with the specified maximum count. Creates max_count images, each
// initialized as a 1x1 RGBA8 SRGB image. The array must be cleaned up with vk_device_local_image_array_cleanup_z.
// =========================================================================================================================================
// =========================================================================================================================================
void vk_device_local_image_array_init_z(VkDevice device, VkPhysicalDevice phys_dev, vk_device_local_image_array_zt* p_arr, uint32_t max_count)
{
    fatal_check_z(device, "device is null");
    fatal_check_z(phys_dev, "phys_dev is null");
    fatal_check_z(p_arr, "p_arr is null");
    fatal_check_bool_z(max_count > 0, "max_count must be > 0");

    p_arr->max_count      = max_count;
    p_arr->p_images       = (vk_device_image_zt*)fatal_alloc_z(sizeof(vk_device_image_zt) * max_count, "failed to allocate image array");

    uint32_t queue_family = 0;
    for (uint32_t i = 0; i < max_count; i++)
    {
        vk_device_image_init_z(device, phys_dev, &p_arr->p_images[i], 1, 1, 4, 1, 1, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT, &queue_family, 1);
    }
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_device_local_image_array_desc_infos_z: Fill an output buffer with descriptor image infos for all images in the array. The out_infos buffer
// must have capacity for at least max_count elements.
// =========================================================================================================================================
// =========================================================================================================================================
void vk_device_local_image_array_desc_infos_z(const vk_device_local_image_array_zt* p_arr, VkImageLayout img_layout, VkDescriptorImageInfo* out_infos)
{
    fatal_check_z(p_arr, "p_arr is null");
    fatal_check_z(out_infos, "out_infos is null");

    for (uint32_t i = 0; i < p_arr->max_count; i++)
    {
        out_infos[i] = vk_device_image_desc_info_z(&p_arr->p_images[i], img_layout);
    }
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_device_local_image_array_get_z: Get a pointer to the image at the specified index. Fatals if index is out of bounds.
// =========================================================================================================================================
// =========================================================================================================================================
vk_device_image_zt* vk_device_local_image_array_get_z(vk_device_local_image_array_zt* p_arr, uint32_t idx)
{
    fatal_check_z(p_arr, "p_arr is null");
    fatal_check_bool_z(idx < p_arr->max_count, "index out of bounds");

    return &p_arr->p_images[idx];
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_device_local_image_array_cleanup_z: Clean up device-local image array resources. Cleans up all images and frees the array memory.
// =========================================================================================================================================
// =========================================================================================================================================
void vk_device_local_image_array_cleanup_z(vk_device_local_image_array_zt* p_arr)
{
    fatal_check_z(p_arr, "p_arr is null");

    if (p_arr->p_images)
    {
        for (uint32_t i = 0; i < p_arr->max_count; i++)
        {
            vk_device_image_cleanup_z(&p_arr->p_images[i]);
        }
        free(p_arr->p_images);
        p_arr->p_images  = nullptr;
        p_arr->max_count = 0;
    }
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_host_visible_image_init_z: Initialize a host-visible image with linear tiling. Creates image, allocates host-visible and host-coherent
// memory, maps memory, and creates image view. The image must be cleaned up with vk_host_visible_image_cleanup_z.
// =========================================================================================================================================
// =========================================================================================================================================
void vk_host_visible_image_init_z(
    VkDevice device,
    VkPhysicalDevice phys_dev,
    vk_host_visible_image_zt* p_img,
    uint32_t width,
    uint32_t height,
    uint32_t num_channels,
    size_t pixel_size,
    uint32_t mip_levels,
    VkFormat format,
    VkImageUsageFlags usage,
    VkImageAspectFlags flags,
    const uint32_t* p_queue_families,
    uint32_t queue_family_count
)
{
    fatal_check_z(device, "device is null");
    fatal_check_z(phys_dev, "phys_dev is null");
    fatal_check_z(p_img, "p_img is null");
    fatal_check_bool_z(width > 0, "width must be > 0");
    fatal_check_bool_z(height > 0, "height must be > 0");
    fatal_check_bool_z(num_channels > 0, "num_channels must be > 0");
    fatal_check_bool_z(pixel_size > 0, "pixel_size must be > 0");
    fatal_check_bool_z(mip_levels > 0, "mip_levels must be > 0");

    p_img->device       = device;
    p_img->width        = width;
    p_img->height       = height;
    p_img->num_channels = num_channels;
    p_img->pixel_size   = pixel_size;
    p_img->mip_levels   = mip_levels;
    p_img->format       = format;

    // =============================================================================================
    // =============================================================================================
    // create image
    // =============================================================================================
    // =============================================================================================
    {
        VkSharingMode sharing_mode = queue_family_count > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;

        VkImageCreateInfo info     = {};
        info.sType                 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        info.imageType             = VK_IMAGE_TYPE_2D;
        info.extent.width          = width;
        info.extent.height         = height;
        info.extent.depth          = 1;
        info.mipLevels             = mip_levels;
        info.arrayLayers           = 1;
        info.format                = format;
        info.tiling                = VK_IMAGE_TILING_LINEAR;
        info.initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED;
        info.usage                 = usage;
        info.samples               = VK_SAMPLE_COUNT_1_BIT;
        info.sharingMode           = sharing_mode;
        info.queueFamilyIndexCount = queue_family_count;
        info.pQueueFamilyIndices   = p_queue_families;

        VK_CHECK(vkCreateImage(device, &info, nullptr, &p_img->handle), "creating image");
    }

    // =============================================================================================
    // =============================================================================================
    // allocate and bind memory
    // =============================================================================================
    // =============================================================================================
    {
        VkMemoryRequirements mem_require;
        vkGetImageMemoryRequirements(device, p_img->handle, &mem_require);

        VkMemoryPropertyFlags mem_props           = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

        uint32_t memory_type_idx                  = vk_find_memory_type_z(phys_dev, mem_require.memoryTypeBits, mem_props);

        VkMemoryPriorityAllocateInfoEXT prio_info = {};
        prio_info.sType                           = VK_STRUCTURE_TYPE_MEMORY_PRIORITY_ALLOCATE_INFO_EXT;
        prio_info.priority                        = 1.0f;

        VkMemoryAllocateInfo alloc_info           = {};
        alloc_info.sType                          = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.allocationSize                 = mem_require.size;
        alloc_info.memoryTypeIndex                = memory_type_idx;
        alloc_info.pNext                          = &prio_info;

        VK_CHECK(vkAllocateMemory(device, &alloc_info, nullptr, &p_img->memory), "allocating image memory");

        VK_CHECK(vkBindImageMemory(device, p_img->handle, p_img->memory, 0), "binding image memory");
    }

    // =============================================================================================
    // =============================================================================================
    // map memory, offset to correct image layout
    // =============================================================================================
    // =============================================================================================
    {
        VkSubresourceLayout subResourceLayout;
        {
            VkImageSubresource subResource = {};
            subResource.aspectMask         = VK_IMAGE_ASPECT_COLOR_BIT;
            vkGetImageSubresourceLayout(device, p_img->handle, &subResource, &subResourceLayout);
        }

        void* imageData;
        vkMapMemory(device, p_img->memory, 0, VK_WHOLE_SIZE, 0, &imageData);
        p_img->p_mapped = (unsigned char*)imageData + subResourceLayout.offset;
    }

    // =============================================================================================
    // =============================================================================================
    // create view (only if usage flags support it)
    // =============================================================================================
    // =============================================================================================
    {
        const VkImageUsageFlags view_usage_mask =
            VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;

        if (usage & view_usage_mask)
        {
            VkImageViewCreateInfo info           = {};
            info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            info.image                           = p_img->handle;
            info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
            info.format                          = format;
            info.subresourceRange.aspectMask     = flags;
            info.subresourceRange.baseMipLevel   = 0;
            info.subresourceRange.levelCount     = mip_levels;
            info.subresourceRange.baseArrayLayer = 0;
            info.subresourceRange.layerCount     = 1;

            VK_CHECK(vkCreateImageView(device, &info, nullptr, &p_img->view), "creating image view");
        }
        else
        {
            p_img->view = VK_NULL_HANDLE;
        }
    }
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_host_visible_image_desc_info_z: Get descriptor image info for use in descriptor sets.
// =========================================================================================================================================
// =========================================================================================================================================
VkDescriptorImageInfo vk_host_visible_image_desc_info_z(const vk_host_visible_image_zt* p_img, VkImageLayout img_layout)
{
    fatal_check_z(p_img, "p_img is null");

    VkDescriptorImageInfo info = {};
    info.imageView             = p_img->view;
    info.sampler               = VK_NULL_HANDLE;
    info.imageLayout           = img_layout;

    return info;
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_host_visible_image_cleanup_z: Clean up host-visible image resources. Unmaps memory, destroys image view, image, and frees memory.
// =========================================================================================================================================
// =========================================================================================================================================
void vk_host_visible_image_cleanup_z(vk_host_visible_image_zt* p_img)
{
    fatal_check_z(p_img, "p_img is null");

    if (p_img->memory != VK_NULL_HANDLE)
    {
        vkUnmapMemory(p_img->device, p_img->memory);
    }

    if (p_img->view != VK_NULL_HANDLE)
    {
        vkDestroyImageView(p_img->device, p_img->view, nullptr);
    }
    vkDestroyImage(p_img->device, p_img->handle, nullptr);
    vkFreeMemory(p_img->device, p_img->memory, nullptr);

    p_img->p_mapped = nullptr;
}
