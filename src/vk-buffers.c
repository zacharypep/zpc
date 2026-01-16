#include "zpc/vk.h"

#include "zpc/fatal.h"

#include <vulkan/vulkan.h>
#include <stdlib.h>
#include <string.h>

// =========================================================================================================================================
// =========================================================================================================================================
// vk_device_buff_init_z: Initialize a device-local buffer with host-visible and host-coherent memory. Maps the buffer for CPU access.
// =========================================================================================================================================
// =========================================================================================================================================
void vk_device_buff_init_z(VkDevice device, VkPhysicalDevice phys_dev, vk_device_buff_zt* p_buff, size_t stride, uint64_t max_count, VkBufferUsageFlags usage)
{
    fatal_check_z(device, "device is null");
    fatal_check_z(phys_dev, "phys_dev is null");
    fatal_check_z(p_buff, "p_buff is null");
    fatal_check_bool_z(stride > 0, "stride must be > 0");
    fatal_check_bool_z(max_count > 0, "max_count must be > 0");

    p_buff->device        = device;
    p_buff->stride        = stride;
    p_buff->max_count     = max_count;
    p_buff->count         = 0;
    p_buff->deviceAddress = 0;

    vk_buffer_create_z(device, phys_dev, max_count * stride, usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &p_buff->handle, &p_buff->memory);

    if (usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
    {
        p_buff->deviceAddress = vk_buffer_device_address_z(device, p_buff->handle);
    }

    vkMapMemory(device, p_buff->memory, 0, max_count * stride, 0, &p_buff->p_mapped);
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_device_buff_bump_z: Allocate a region of the specified number of elements without copying data. Returns a region handle.
// =========================================================================================================================================
// =========================================================================================================================================
vk_region_handle_zt vk_device_buff_bump_z(vk_device_buff_zt* p_buff, uint64_t num)
{
    fatal_check_z(p_buff, "p_buff is null");

    uint64_t new_count = p_buff->count + num;
    if (new_count > p_buff->max_count)
    {
        fatal_z("pushed past bounds: (%llu > %llu)", (unsigned long long)new_count, (unsigned long long)p_buff->max_count);
    }

    vk_region_handle_zt region = {
        .ptr         = (uint8_t*)p_buff->p_mapped + p_buff->count * p_buff->stride,
        .device_addr = p_buff->deviceAddress + p_buff->count * p_buff->stride,
        .start_idx   = p_buff->count,
        .count       = num,
    };
    p_buff->count = new_count;
    return region;
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_device_buff_bump_aligned_z: Allocate an aligned region of the specified size in bytes. Returns a region handle pointing to the aligned offset.
// NOTE: This function requires stride=1 (byte-based allocation).
// =========================================================================================================================================
// =========================================================================================================================================
vk_region_handle_zt vk_device_buff_bump_aligned_z(vk_device_buff_zt* p_buff, VkDeviceSize size, VkDeviceSize alignment)
{
    fatal_check_z(p_buff, "p_buff is null");
    fatal_check_bool_z(alignment > 0, "alignment must be > 0");
    fatal_check_bool_z(p_buff->stride == 1, "vk_device_buff_bump_aligned_z requires stride=1");

    VkDeviceSize current_offset = p_buff->count;
    VkDeviceSize aligned_offset = vk_aligned_size_vk_z(current_offset, alignment);
    VkDeviceSize padding = aligned_offset - current_offset;
    VkDeviceSize total_bytes = padding + size;

    uint64_t new_count = p_buff->count + total_bytes;
    if (new_count > p_buff->max_count)
    {
        fatal_z("pushed past bounds: (%llu > %llu)", (unsigned long long)new_count, (unsigned long long)p_buff->max_count);
    }

    vk_region_handle_zt region = {
        .ptr         = (uint8_t*)p_buff->p_mapped + aligned_offset,
        .device_addr = p_buff->deviceAddress + aligned_offset,
        .start_idx   = aligned_offset,
        .count       = size,
    };

    p_buff->count = new_count;

    return region;
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_device_buff_push_z: Copy data into the buffer and return a region handle.
// =========================================================================================================================================
// =========================================================================================================================================
vk_region_handle_zt vk_device_buff_push_z(vk_device_buff_zt* p_buff, const void* src, uint64_t num)
{
    fatal_check_z(p_buff, "p_buff is null");
    if (num > 0)
    {
        fatal_check_z(src, "src is null");
    }

    uint64_t new_count = p_buff->count + num;
    if (new_count > p_buff->max_count)
    {
        fatal_z("pushed past bounds: (%llu > %llu)", (unsigned long long)new_count, (unsigned long long)p_buff->max_count);
    }

    void* p_region = (uint8_t*)p_buff->p_mapped + p_buff->count * p_buff->stride;
    memcpy(p_region, src, num * p_buff->stride);

    vk_region_handle_zt region = {
        .ptr         = p_region,
        .device_addr = p_buff->deviceAddress + p_buff->count * p_buff->stride,
        .start_idx   = p_buff->count,
        .count       = num,
    };
    p_buff->count = new_count;
    return region;
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_device_buff_remove_z: Remove a region from the buffer.
// =========================================================================================================================================
// =========================================================================================================================================
void vk_device_buff_remove_z(vk_device_buff_zt* p_buff, vk_region_handle_zt region)
{
    fatal_check_z(p_buff, "p_buff is null");
    fatal_z("removing regions not implemented");
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_device_buff_size_z: Get the total size of the buffer in bytes.
// =========================================================================================================================================
// =========================================================================================================================================
size_t vk_device_buff_size_z(const vk_device_buff_zt* p_buff)
{
    fatal_check_z(p_buff, "p_buff is null");
    return p_buff->max_count * p_buff->stride;
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_device_buff_desc_info_z: Get descriptor buffer info for use in descriptor sets.
// =========================================================================================================================================
// =========================================================================================================================================
VkDescriptorBufferInfo vk_device_buff_desc_info_z(const vk_device_buff_zt* p_buff)
{
    fatal_check_z(p_buff, "p_buff is null");

    VkDescriptorBufferInfo desc_info = {};
    desc_info.buffer                 = p_buff->handle;
    desc_info.offset                 = 0;
    desc_info.range                  = vk_device_buff_size_z(p_buff);

    return desc_info;
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_device_buff_reset_z: Reset the buffer count to zero.
// =========================================================================================================================================
// =========================================================================================================================================
void vk_device_buff_reset_z(vk_device_buff_zt* p_buff)
{
    fatal_check_z(p_buff, "p_buff is null");
    p_buff->count = 0;
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_device_buff_cleanup_z: Clean up device buffer resources. Unmaps memory, destroys buffer, and frees memory.
// =========================================================================================================================================
// =========================================================================================================================================
void vk_device_buff_cleanup_z(VkDevice device, vk_device_buff_zt* p_buff)
{
    fatal_check_z(device, "device is null");
    fatal_check_z(p_buff, "p_buff is null");

    vk_device_buff_reset_z(p_buff);
    vkUnmapMemory(device, p_buff->memory);
    vkDestroyBuffer(device, p_buff->handle, nullptr);
    vkFreeMemory(device, p_buff->memory, nullptr);
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_host_buff_init_z: Initialize a host-visible and host-coherent buffer. Maps the buffer for CPU access.
// =========================================================================================================================================
// =========================================================================================================================================
void vk_host_buff_init_z(VkDevice device, VkPhysicalDevice phys_dev, vk_host_buff_zt* p_buff, size_t stride, uint32_t max_count, VkBufferUsageFlags usage)
{
    fatal_check_z(device, "device is null");
    fatal_check_z(phys_dev, "phys_dev is null");
    fatal_check_z(p_buff, "p_buff is null");
    fatal_check_bool_z(stride > 0, "stride must be > 0");
    fatal_check_bool_z(max_count > 0, "max_count must be > 0");

    p_buff->device    = device;
    p_buff->stride    = stride;
    p_buff->max_count = max_count;
    p_buff->count     = 0;

    vk_buffer_create_z(device, phys_dev, max_count * stride, usage, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &p_buff->handle, &p_buff->memory);

    vkMapMemory(device, p_buff->memory, 0, max_count * stride, 0, &p_buff->p_mapped);
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_host_buff_bump_z: Allocate a region of the specified number of elements without copying data. Returns a region handle.
// =========================================================================================================================================
// =========================================================================================================================================
vk_region_handle_zt vk_host_buff_bump_z(vk_host_buff_zt* p_buff, uint32_t num)
{
    fatal_check_z(p_buff, "p_buff is null");

    uint32_t new_count = p_buff->count + num;
    if (new_count > p_buff->max_count)
    {
        fatal_z("pushed past bounds: (%u > %u)", new_count, p_buff->max_count);
    }

    vk_region_handle_zt region = {
        .ptr         = (uint8_t*)p_buff->p_mapped + p_buff->count * p_buff->stride,
        .device_addr = 0,
        .start_idx   = (uint64_t)p_buff->count,
        .count       = (uint64_t)num,
    };
    p_buff->count = new_count;
    return region;
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_host_buff_push_z: Copy data into the buffer and return a region handle.
// =========================================================================================================================================
// =========================================================================================================================================
vk_region_handle_zt vk_host_buff_push_z(vk_host_buff_zt* p_buff, const void* src, uint32_t num)
{
    fatal_check_z(p_buff, "p_buff is null");
    if (num > 0)
    {
        fatal_check_z(src, "src is null");
    }

    uint32_t new_count = p_buff->count + num;
    if (new_count > p_buff->max_count)
    {
        fatal_z("pushed past bounds: (%u > %u)", new_count, p_buff->max_count);
    }

    void* p_region = (uint8_t*)p_buff->p_mapped + p_buff->count * p_buff->stride;
    memcpy(p_region, src, num * p_buff->stride);

    vk_region_handle_zt region = {
        .ptr         = p_region,
        .device_addr = 0,
        .start_idx   = p_buff->count,
        .count       = num,
    };
    p_buff->count = new_count;
    return region;
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_host_buff_remove_z: Remove a region from the buffer.
// =========================================================================================================================================
// =========================================================================================================================================
void vk_host_buff_remove_z(vk_host_buff_zt* p_buff, vk_region_handle_zt region)
{
    fatal_check_z(p_buff, "p_buff is null");
    fatal_z("removing regions not implemented");
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_host_buff_size_z: Get the total size of the buffer in bytes.
// =========================================================================================================================================
// =========================================================================================================================================
size_t vk_host_buff_size_z(const vk_host_buff_zt* p_buff)
{
    fatal_check_z(p_buff, "p_buff is null");
    return p_buff->max_count * p_buff->stride;
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_host_buff_reset_z: Reset the buffer count to zero.
// =========================================================================================================================================
// =========================================================================================================================================
void vk_host_buff_reset_z(vk_host_buff_zt* p_buff)
{
    fatal_check_z(p_buff, "p_buff is null");
    p_buff->count = 0;
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_host_buff_cleanup_z: Clean up host buffer resources. Destroys buffer and frees memory.
// =========================================================================================================================================
// =========================================================================================================================================
void vk_host_buff_cleanup_z(VkDevice device, vk_host_buff_zt* p_buff)
{
    fatal_check_z(device, "device is null");
    fatal_check_z(p_buff, "p_buff is null");

    vk_host_buff_reset_z(p_buff);
    vkDestroyBuffer(device, p_buff->handle, nullptr);
    vkFreeMemory(device, p_buff->memory, nullptr);
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_staged_device_buff_init_z: Initialize a staged device-local buffer with host-visible and host-coherent memory. Creates a staging buffer for CPU writes.
// =========================================================================================================================================
// =========================================================================================================================================
void vk_staged_device_buff_init_z(VkDevice device, VkPhysicalDevice phys_dev, vk_staged_device_buff_zt* p_buff, size_t stride, uint32_t max_count, VkBufferUsageFlags usage)
{
    fatal_check_z(device, "device is null");
    fatal_check_z(phys_dev, "phys_dev is null");
    fatal_check_z(p_buff, "p_buff is null");
    fatal_check_bool_z(stride > 0, "stride must be > 0");
    fatal_check_bool_z(max_count > 0, "max_count must be > 0");

    p_buff->device            = device;
    p_buff->stride            = stride;
    p_buff->max_count         = max_count;
    p_buff->count             = 0;
    p_buff->deviceAddress     = 0;
    p_buff->staging_buff_size = max_count * stride;

    vk_buffer_create_z(device, phys_dev, max_count * stride, usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &p_buff->handle, &p_buff->memory);

    if (usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
    {
        p_buff->deviceAddress = vk_buffer_device_address_z(device, p_buff->handle);
    }

    vkMapMemory(device, p_buff->memory, 0, max_count * stride, 0, &p_buff->p_mapped_device);

    p_buff->staging_buff = fatal_alloc_z(p_buff->staging_buff_size, "failed to allocate staging buffer");
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_staged_device_buff_bump_z: Allocate a region of the specified number of elements without copying data. Returns a region handle.
// =========================================================================================================================================
// =========================================================================================================================================
vk_region_handle_zt vk_staged_device_buff_bump_z(vk_staged_device_buff_zt* p_buff, uint32_t num)
{
    fatal_check_z(p_buff, "p_buff is null");

    uint32_t new_count = p_buff->count + num;
    if (new_count > p_buff->max_count)
    {
        fatal_z("pushed past bounds: (%u > %u)", new_count, p_buff->max_count);
    }

    vk_region_handle_zt region = {
        .ptr         = (uint8_t*)p_buff->staging_buff + p_buff->count * p_buff->stride,
        .device_addr = p_buff->deviceAddress + p_buff->count * p_buff->stride,
        .start_idx   = (uint64_t)p_buff->count,
        .count       = (uint64_t)num,
    };
    p_buff->count = new_count;
    return region;
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_staged_device_buff_push_z: Copy data into the staging buffer and return a region handle.
// =========================================================================================================================================
// =========================================================================================================================================
vk_region_handle_zt vk_staged_device_buff_push_z(vk_staged_device_buff_zt* p_buff, const void* src, uint32_t num)
{
    fatal_check_z(p_buff, "p_buff is null");
    if (num > 0)
    {
        fatal_check_z(src, "src is null");
    }

    uint32_t new_count = p_buff->count + num;
    if (new_count > p_buff->max_count)
    {
        fatal_z("pushed past bounds: (%u > %u)", new_count, p_buff->max_count);
    }

    void* p_region = (uint8_t*)p_buff->staging_buff + p_buff->count * p_buff->stride;
    memcpy(p_region, src, num * p_buff->stride);

    vk_region_handle_zt region = {
        .ptr         = p_region,
        .device_addr = p_buff->deviceAddress + p_buff->count * p_buff->stride,
        .start_idx   = p_buff->count,
        .count       = num,
    };
    p_buff->count = new_count;
    return region;
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_staged_device_buff_remove_z: Remove a region from the buffer.
// =========================================================================================================================================
// =========================================================================================================================================
void vk_staged_device_buff_remove_z(vk_staged_device_buff_zt* p_buff, vk_region_handle_zt region)
{
    fatal_check_z(p_buff, "p_buff is null");
    fatal_z("removing regions not implemented");
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_staged_device_buff_push_device_z: Copy data from the staging buffer to the device buffer.
// =========================================================================================================================================
// =========================================================================================================================================
void vk_staged_device_buff_push_device_z(vk_staged_device_buff_zt* p_buff)
{
    fatal_check_z(p_buff, "p_buff is null");
    memcpy(p_buff->p_mapped_device, p_buff->staging_buff, p_buff->count * p_buff->stride);
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_staged_device_buff_size_z: Get the total size of the buffer in bytes.
// =========================================================================================================================================
// =========================================================================================================================================
size_t vk_staged_device_buff_size_z(const vk_staged_device_buff_zt* p_buff)
{
    fatal_check_z(p_buff, "p_buff is null");
    return p_buff->max_count * p_buff->stride;
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_staged_device_buff_desc_info_z: Get descriptor buffer info for use in descriptor sets.
// =========================================================================================================================================
// =========================================================================================================================================
VkDescriptorBufferInfo vk_staged_device_buff_desc_info_z(const vk_staged_device_buff_zt* p_buff)
{
    fatal_check_z(p_buff, "p_buff is null");

    VkDescriptorBufferInfo desc_info = {};
    desc_info.buffer                 = p_buff->handle;
    desc_info.offset                 = 0;
    desc_info.range                  = vk_staged_device_buff_size_z(p_buff);

    return desc_info;
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_staged_device_buff_reset_z: Reset the buffer count to zero.
// =========================================================================================================================================
// =========================================================================================================================================
void vk_staged_device_buff_reset_z(vk_staged_device_buff_zt* p_buff)
{
    fatal_check_z(p_buff, "p_buff is null");
    p_buff->count = 0;
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_staged_device_buff_cleanup_z: Clean up staged device buffer resources. Destroys buffer and frees memory.
// =========================================================================================================================================
// =========================================================================================================================================
void vk_staged_device_buff_cleanup_z(VkDevice device, vk_staged_device_buff_zt* p_buff)
{
    fatal_check_z(device, "device is null");
    fatal_check_z(p_buff, "p_buff is null");

    vk_staged_device_buff_reset_z(p_buff);
    vkUnmapMemory(device, p_buff->memory);
    if (p_buff->staging_buff)
    {
        free(p_buff->staging_buff);
        p_buff->staging_buff = nullptr;
    }
    vkDestroyBuffer(device, p_buff->handle, nullptr);
    vkFreeMemory(device, p_buff->memory, nullptr);
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_hidden_device_local_buff_init_z: Initialize a hidden device-local buffer with host-visible and host-coherent memory. Does not map the buffer.
// =========================================================================================================================================
// =========================================================================================================================================
void vk_hidden_device_local_buff_init_z(VkDevice device, VkPhysicalDevice phys_dev, vk_hidden_device_local_buff_zt* p_buff, VkDeviceSize buff_size, VkBufferUsageFlags usage)
{
    fatal_check_z(device, "device is null");
    fatal_check_z(phys_dev, "phys_dev is null");
    fatal_check_z(p_buff, "p_buff is null");
    fatal_check_bool_z(buff_size > 0, "buff_size must be > 0");

    p_buff->device      = device;
    p_buff->device_addr = 0;

    vk_buffer_create_z(device, phys_dev, buff_size, usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &p_buff->handle, &p_buff->memory);

    if (usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
    {
        p_buff->device_addr = vk_buffer_device_address_z(device, p_buff->handle);
    }
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_hidden_device_local_buff_cleanup_z: Clean up hidden device-local buffer resources. Destroys buffer and frees memory.
// =========================================================================================================================================
// =========================================================================================================================================
void vk_hidden_device_local_buff_cleanup_z(VkDevice device, vk_hidden_device_local_buff_zt* p_buff)
{
    fatal_check_z(device, "device is null");
    fatal_check_z(p_buff, "p_buff is null");

    vkDestroyBuffer(device, p_buff->handle, nullptr);
    vkFreeMemory(device, p_buff->memory, nullptr);
}
