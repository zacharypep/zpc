#include "zpc/vk.h"

#include "zpc/fatal.h"

#include <vulkan/vulkan.h>

// =========================================================================================================================================
// =========================================================================================================================================
// vk_aligned_size_u32_z: Calculate the aligned size for a uint32_t value, rounding up to the nearest multiple of alignment.
// =========================================================================================================================================
// =========================================================================================================================================
uint32_t vk_aligned_size_u32_z(uint32_t value, uint32_t alignment)
{
    return (value + alignment - 1) & ~(alignment - 1);
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_aligned_size_sz_z: Calculate the aligned size for a size_t value, rounding up to the nearest multiple of alignment.
// =========================================================================================================================================
// =========================================================================================================================================
size_t vk_aligned_size_sz_z(size_t value, size_t alignment)
{
    return (value + alignment - 1) & ~(alignment - 1);
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_aligned_size_vk_z: Calculate the aligned size for a VkDeviceSize value, rounding up to the nearest multiple of alignment.
// =========================================================================================================================================
// =========================================================================================================================================
VkDeviceSize vk_aligned_size_vk_z(VkDeviceSize value, VkDeviceSize alignment)
{
    return (value + alignment - 1) & ~(alignment - 1);
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_buffer_device_address_z: Get the device address of a Vulkan buffer for use in shader device address operations.
// =========================================================================================================================================
// =========================================================================================================================================
VkDeviceAddress vk_buffer_device_address_z(VkDevice device, VkBuffer buffer)
{
    VkBufferDeviceAddressInfo info = {};
    info.sType                     = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    info.buffer                    = buffer;
    return vkGetBufferDeviceAddress(device, &info);
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_find_memory_type_z: Find a memory type index that matches the given filter and property flags. Fatals if no suitable type is found.
// =========================================================================================================================================
// =========================================================================================================================================
uint32_t vk_find_memory_type_z(VkPhysicalDevice phys_dev, uint32_t filter, VkMemoryPropertyFlags prop_flags)
{
    VkPhysicalDeviceMemoryProperties phys_dev_mem_props;
    vkGetPhysicalDeviceMemoryProperties(phys_dev, &phys_dev_mem_props);
    for (uint32_t i = 0; i < phys_dev_mem_props.memoryTypeCount; i++)
    {
        if ((filter & (1 << i)) && (phys_dev_mem_props.memoryTypes[i].propertyFlags & prop_flags) == prop_flags)
        {
            return i;
        }
    }
    fatal_z("failed to find suitable memory type");
    return 0;
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_transform_matrix_from_mat4_z: Convert a mat4_zt matrix to a VkTransformMatrixKHR structure for use in ray tracing acceleration structures.
// =========================================================================================================================================
// =========================================================================================================================================
VkTransformMatrixKHR vk_transform_matrix_from_mat4_z(const mat4_zt* p_mat)
{
    fatal_check_z(p_mat, "p_mat is null");

    VkTransformMatrixKHR result = {};
    result.matrix[0][0]         = p_mat->f[0][0];
    result.matrix[0][1]         = p_mat->f[0][1];
    result.matrix[0][2]         = p_mat->f[0][2];
    result.matrix[0][3]         = p_mat->f[0][3];
    result.matrix[1][0]         = p_mat->f[1][0];
    result.matrix[1][1]         = p_mat->f[1][1];
    result.matrix[1][2]         = p_mat->f[1][2];
    result.matrix[1][3]         = p_mat->f[1][3];
    result.matrix[2][0]         = p_mat->f[2][0];
    result.matrix[2][1]         = p_mat->f[2][1];
    result.matrix[2][2]         = p_mat->f[2][2];
    result.matrix[2][3]         = p_mat->f[2][3];

    return result;
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_record_copy_buffer_to_image_z: Record a command to copy buffer data to an image, supporting multiple mip levels. The buffer data must be
// laid out sequentially with each mip level following the previous one.
// =========================================================================================================================================
// =========================================================================================================================================
void vk_record_copy_buffer_to_image_z(VkCommandBuffer cmd_buff, VkBuffer buff, VkDeviceSize offset, VkImage dst, uint32_t width, uint32_t height, uint32_t num_channels, size_t pixel_size, uint32_t mip_levels)
{
    fatal_check_z(cmd_buff, "cmd_buff is null");
    fatal_check_z(buff, "buff is null");
    fatal_check_z(dst, "dst is null");
    fatal_check_bool_z(mip_levels > 0, "mip_levels must be > 0");
    fatal_check_bool_z(mip_levels <= 16, "mip_levels exceeds maximum of 16");

    uint32_t running_width           = width;
    uint32_t running_height          = height;
    VkDeviceSize running_mips_offset = 0;

    VkBufferImageCopy copies[16];

    for (uint32_t i = 0; i < mip_levels; i++)
    {
        VkBufferImageCopy* copy                = &copies[i];
        copy->bufferOffset                     = offset + running_mips_offset;
        copy->bufferRowLength                  = 0;
        copy->bufferImageHeight                = 0;
        copy->imageSubresource.aspectMask      = VK_IMAGE_ASPECT_COLOR_BIT;
        copy->imageSubresource.mipLevel        = i;
        copy->imageSubresource.baseArrayLayer  = 0;
        copy->imageSubresource.layerCount      = 1;
        copy->imageOffset.x                    = 0;
        copy->imageOffset.y                    = 0;
        copy->imageOffset.z                    = 0;
        copy->imageExtent.width                = running_width;
        copy->imageExtent.height               = running_height;
        copy->imageExtent.depth                = 1;

        VkDeviceSize per_mip_size              = (VkDeviceSize)running_width * (VkDeviceSize)running_height * (VkDeviceSize)num_channels * (VkDeviceSize)pixel_size;

        running_width                         /= 2;
        running_height                        /= 2;
        running_mips_offset                   += per_mip_size;
    }

    vkCmdCopyBufferToImage(cmd_buff, buff, dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mip_levels, copies);
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_shader_module_create_z: Create a Vulkan shader module from SPIR-V bytecode. The code_size must be a multiple of 4 bytes.
// =========================================================================================================================================
// =========================================================================================================================================
void vk_shader_module_create_z(VkDevice device, VkShaderModule* p_shader_module, const void* p_code, size_t code_size)
{
    fatal_check_z(device, "device is null");
    fatal_check_z(p_shader_module, "p_shader_module is null");
    fatal_check_z(p_code, "p_code is null");
    fatal_check_bool_z(code_size > 0, "code_size must be > 0");
    fatal_check_bool_z(code_size % 4 == 0, "code_size must be multiple of 4");

    VkShaderModuleCreateInfo info = {};
    info.sType                    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    info.codeSize                 = code_size;
    info.pCode                    = (const uint32_t*)p_code;

    VK_CHECK(vkCreateShaderModule(device, &info, nullptr, p_shader_module), "creating shader module");
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_shader_module_create_from_shader_z: Create a Vulkan shader module from a vk_shader_zt structure containing SPIR-V bytecode.
// =========================================================================================================================================
// =========================================================================================================================================
void vk_shader_module_create_from_shader_z(VkDevice device, VkShaderModule* p_shader_module, const vk_shader_zt* p_shader)
{
    fatal_check_z(p_shader, "p_shader is null");
    vk_shader_module_create_z(device, p_shader_module, p_shader->p_data, p_shader->size);
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_buffer_create_z: Create a Vulkan buffer with the specified size, usage flags, and memory properties. Allocates and binds device memory
// automatically. If VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT is set, configures memory allocation flags appropriately.
// =========================================================================================================================================
// =========================================================================================================================================
void vk_buffer_create_z(VkDevice device, VkPhysicalDevice phys_dev, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags props, VkBuffer* p_buffer, VkDeviceMemory* p_memory)
{
    fatal_check_z(device, "device is null");
    fatal_check_z(phys_dev, "phys_dev is null");
    fatal_check_z(p_buffer, "p_buffer is null");
    fatal_check_z(p_memory, "p_memory is null");
    fatal_check_bool_z(size > 0, "size must be > 0");

    // =============================================================================================
    // =============================================================================================
    // Create the Vulkan buffer object with the specified size and usage flags.
    // =============================================================================================
    // =============================================================================================
    {
        VkBufferCreateInfo buffer_info = {};
        buffer_info.sType              = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_info.size               = size;
        buffer_info.usage              = usage;
        buffer_info.sharingMode        = VK_SHARING_MODE_EXCLUSIVE;

        VK_CHECK(vkCreateBuffer(device, &buffer_info, nullptr, p_buffer), "creating buffer");
    }

    // =============================================================================================
    // =============================================================================================
    // Allocate device memory for the buffer, finding a suitable memory type and configuring priority and device address flags if needed.
    // =============================================================================================
    // =============================================================================================
    {
        VkMemoryRequirements mem_requirements;
        vkGetBufferMemoryRequirements(device, *p_buffer, &mem_requirements);

        VkMemoryPriorityAllocateInfoEXT prio_info  = {};
        prio_info.sType                            = VK_STRUCTURE_TYPE_MEMORY_PRIORITY_ALLOCATE_INFO_EXT;
        prio_info.priority                         = 1.0f;

        VkMemoryAllocateInfo alloc_info            = {};
        alloc_info.sType                           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.allocationSize                  = mem_requirements.size;
        alloc_info.memoryTypeIndex                 = vk_find_memory_type_z(phys_dev, mem_requirements.memoryTypeBits, props);
        alloc_info.pNext                           = &prio_info;

        // if VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT usage bit, set appropriate memory alloc flag
        VkMemoryAllocateFlagsInfo alloc_flags_info = {};
        {
            if (usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
            {
                alloc_flags_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO_KHR;
                alloc_flags_info.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;
                prio_info.pNext        = &alloc_flags_info;
            }
        }

        VK_CHECK(vkAllocateMemory(device, &alloc_info, nullptr, p_memory), "allocating buffer memory");
    }

    // =============================================================================================
    // =============================================================================================
    // Bind the allocated memory to the buffer object.
    // =============================================================================================
    // =============================================================================================
    {
        VK_CHECK(vkBindBufferMemory(device, *p_buffer, *p_memory, 0), "binding buffer memory");
    }
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_find_shader_group_z: Find a shader group by ID in the instance's shader groups array. Returns nullptr if not found.
// =========================================================================================================================================
// =========================================================================================================================================
vk_shader_group_zt* vk_find_shader_group_z(vk_instance_zt* p_inst, uint64_t id)
{
    fatal_check_z(p_inst, "p_inst is null");

    for (uint32_t i = 0; i < p_inst->num_shader_groups; i++)
    {
        if (p_inst->shader_groups[i].id == id)
        {
            return &p_inst->shader_groups[i];
        }
    }
    return nullptr;
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_get_or_add_shader_group_z: Find a shader group by ID, or create a new one if not found. Returns pointer to the shader group.
// =========================================================================================================================================
// =========================================================================================================================================
vk_shader_group_zt* vk_get_or_add_shader_group_z(vk_instance_zt* p_inst, uint64_t id)
{
    fatal_check_z(p_inst, "p_inst is null");

    vk_shader_group_zt* existing = vk_find_shader_group_z(p_inst, id);
    if (existing != nullptr)
    {
        return existing;
    }
    if (p_inst->num_shader_groups >= VK_INSTANCE_MAX_SHADER_GROUPS)
    {
        fatal_z("max shader groups exceeded");
    }
    vk_shader_group_zt* new_group = &p_inst->shader_groups[p_inst->num_shader_groups++];
    new_group->id                 = id;
    return new_group;
}
