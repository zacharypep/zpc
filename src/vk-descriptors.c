#include "zpc/vk.h"

#include "zpc/fatal.h"

#include <vulkan/vulkan.h>
#include <stdlib.h>

// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
#define MAX_PANELS 12

// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
struct vk_descriptors_internal_zt
{
    VkPhysicalDeviceDescriptorBufferPropertiesEXT desc_buff_props;

    PFN_vkGetDescriptorSetLayoutSizeEXT vkGetDescriptorSetLayoutSizeEXT;
    PFN_vkGetDescriptorSetLayoutBindingOffsetEXT vkGetDescriptorSetLayoutBindingOffsetEXT;
    PFN_vkGetDescriptorEXT vkGetDescriptorEXT;

    VkDescriptorSetLayout vk_desc_set_layout;

    VkDeviceSize dsl_size;
    VkDeviceSize offset_sampler;
    VkDeviceSize offset_panels;
    VkDeviceSize offset_rw_panels;
    VkDeviceSize offset_textures;
    VkDeviceSize offset_fonts;
    VkDeviceSize offset_gbuff_base_colour;
    VkDeviceSize offset_gbuff_roughness;
    VkDeviceSize offset_gbuff_specular;
    VkDeviceSize offset_gbuff_metallic;
    VkDeviceSize offset_gbuff_emission;
    VkDeviceSize offset_gbuff_pos_world;
    VkDeviceSize offset_gbuff_nrm_world;

    size_t max_textures;
    size_t max_fonts;
};

// =========================================================================================================================================
// =========================================================================================================================================
// vk_descriptors_init_z: Initialize a descriptor system for Vulkan descriptor buffers. Creates descriptor set layout, pipeline layout, and
// descriptor buffer. The system must be cleaned up with vk_descriptors_cleanup_z.
// =========================================================================================================================================
// =========================================================================================================================================
void vk_descriptors_init_z(VkDevice device, VkPhysicalDevice phys_dev, vk_descriptors_system_zt* p_sys, size_t max_textures, size_t max_fonts)
{
    fatal_check_z(device, "device is null");
    fatal_check_z(phys_dev, "phys_dev is null");
    fatal_check_z(p_sys, "p_sys is null");

    // =============================================================================================
    // =============================================================================================
    // allocate internal structure
    // =============================================================================================
    // =============================================================================================
    {
        p_sys->p_i               = (vk_descriptors_internal_zt*)fatal_alloc_z(sizeof(vk_descriptors_internal_zt), "failed to allocate descriptor system internals");
        p_sys->p_i->max_textures = max_textures;
        p_sys->p_i->max_fonts    = max_fonts;
    }

    // =============================================================================================
    // =============================================================================================
    // device props
    // =============================================================================================
    // =============================================================================================
    {
        p_sys->p_i->desc_buff_props       = (VkPhysicalDeviceDescriptorBufferPropertiesEXT){0};
        p_sys->p_i->desc_buff_props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_PROPERTIES_EXT;

        VkPhysicalDeviceProperties2 props = {0};
        props.sType                       = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
        props.pNext                       = &p_sys->p_i->desc_buff_props;

        vkGetPhysicalDeviceProperties2(phys_dev, &props);
    }

    // =============================================================================================
    // =============================================================================================
    // get function pointers
    // =============================================================================================
    // =============================================================================================
    {
        p_sys->p_i->vkGetDescriptorSetLayoutSizeEXT          = (PFN_vkGetDescriptorSetLayoutSizeEXT)vkGetDeviceProcAddr(device, "vkGetDescriptorSetLayoutSizeEXT");
        p_sys->p_i->vkGetDescriptorSetLayoutBindingOffsetEXT = (PFN_vkGetDescriptorSetLayoutBindingOffsetEXT)vkGetDeviceProcAddr(device, "vkGetDescriptorSetLayoutBindingOffsetEXT");
        p_sys->p_i->vkGetDescriptorEXT                       = (PFN_vkGetDescriptorEXT)vkGetDeviceProcAddr(device, "vkGetDescriptorEXT");
    }

    // =============================================================================================
    // =============================================================================================
    // dsl
    // =============================================================================================
    // =============================================================================================
    {
        VkDescriptorSetLayoutBinding bindings[12];
        {
            VkDescriptorSetLayoutBinding* binding = &bindings[0];
            binding->binding                      = 0;
            binding->descriptorType               = VK_DESCRIPTOR_TYPE_SAMPLER;
            binding->descriptorCount              = 1;
            binding->stageFlags                   = VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
            binding->pImmutableSamplers           = nullptr;
        }
        {
            VkDescriptorSetLayoutBinding* binding = &bindings[1];
            binding->binding                      = 1;
            binding->descriptorType               = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            binding->descriptorCount              = MAX_PANELS;
            binding->stageFlags                   = VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
            binding->pImmutableSamplers           = nullptr;
        }
        {
            VkDescriptorSetLayoutBinding* binding = &bindings[2];
            binding->binding                      = 2;
            binding->descriptorType               = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            binding->descriptorCount              = MAX_PANELS;
            binding->stageFlags                   = VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
            binding->pImmutableSamplers           = nullptr;
        }
        {
            VkDescriptorSetLayoutBinding* binding = &bindings[3];
            binding->binding                      = 3;
            binding->descriptorType               = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            binding->descriptorCount              = (uint32_t)max_textures;
            binding->stageFlags                   = VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
            binding->pImmutableSamplers           = nullptr;
        }
        {
            VkDescriptorSetLayoutBinding* binding = &bindings[4];
            binding->binding                      = 4;
            binding->descriptorType               = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            binding->descriptorCount              = (uint32_t)max_fonts;
            binding->stageFlags                   = VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
            binding->pImmutableSamplers           = nullptr;
        }
        {
            VkDescriptorSetLayoutBinding* binding = &bindings[5];
            binding->binding                      = 5;
            binding->descriptorType               = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            binding->descriptorCount              = MAX_PANELS;
            binding->stageFlags                   = VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
            binding->pImmutableSamplers           = nullptr;
        }
        {
            VkDescriptorSetLayoutBinding* binding = &bindings[6];
            binding->binding                      = 6;
            binding->descriptorType               = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            binding->descriptorCount              = MAX_PANELS;
            binding->stageFlags                   = VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
            binding->pImmutableSamplers           = nullptr;
        }
        {
            VkDescriptorSetLayoutBinding* binding = &bindings[7];
            binding->binding                      = 7;
            binding->descriptorType               = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            binding->descriptorCount              = MAX_PANELS;
            binding->stageFlags                   = VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
            binding->pImmutableSamplers           = nullptr;
        }
        {
            VkDescriptorSetLayoutBinding* binding = &bindings[8];
            binding->binding                      = 8;
            binding->descriptorType               = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            binding->descriptorCount              = MAX_PANELS;
            binding->stageFlags                   = VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
            binding->pImmutableSamplers           = nullptr;
        }
        {
            VkDescriptorSetLayoutBinding* binding = &bindings[9];
            binding->binding                      = 9;
            binding->descriptorType               = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            binding->descriptorCount              = MAX_PANELS;
            binding->stageFlags                   = VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
            binding->pImmutableSamplers           = nullptr;
        }
        {
            VkDescriptorSetLayoutBinding* binding = &bindings[10];
            binding->binding                      = 10;
            binding->descriptorType               = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            binding->descriptorCount              = MAX_PANELS;
            binding->stageFlags                   = VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
            binding->pImmutableSamplers           = nullptr;
        }
        {
            VkDescriptorSetLayoutBinding* binding = &bindings[11];
            binding->binding                      = 11;
            binding->descriptorType               = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            binding->descriptorCount              = MAX_PANELS;
            binding->stageFlags                   = VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
            binding->pImmutableSamplers           = nullptr;
        }

        VkDescriptorSetLayoutCreateInfo info = {0};
        info.sType                           = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        info.bindingCount                    = 12;
        info.pBindings                       = bindings;
        info.flags                           = VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT;

        VK_CHECK(vkCreateDescriptorSetLayout(device, &info, nullptr, &p_sys->p_i->vk_desc_set_layout), "vkCreateDescriptorSetLayout");
    }

    // =============================================================================================
    // =============================================================================================
    // pipeline layout
    // =============================================================================================
    // =============================================================================================
    {
        const VkPushConstantRange pc_range = {
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT,
            .offset     = 0,
            .size       = sizeof(VkDeviceAddress),
        };

        VkPipelineLayoutCreateInfo info = {0};
        info.sType                      = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        info.setLayoutCount             = 1;
        info.pSetLayouts                = &p_sys->p_i->vk_desc_set_layout;
        info.pushConstantRangeCount     = 1;
        info.pPushConstantRanges        = &pc_range;

        VK_CHECK(vkCreatePipelineLayout(device, &info, nullptr, &p_sys->vk_pipeline_layout), "vkCreatePipelineLayout");
    }

    // =============================================================================================
    // =============================================================================================
    // get offsets
    // =============================================================================================
    // =============================================================================================
    {
        p_sys->p_i->vkGetDescriptorSetLayoutSizeEXT(device, p_sys->p_i->vk_desc_set_layout, &p_sys->p_i->dsl_size);
        p_sys->p_i->vkGetDescriptorSetLayoutBindingOffsetEXT(device, p_sys->p_i->vk_desc_set_layout, 0, &p_sys->p_i->offset_sampler);
        p_sys->p_i->vkGetDescriptorSetLayoutBindingOffsetEXT(device, p_sys->p_i->vk_desc_set_layout, 1, &p_sys->p_i->offset_panels);
        p_sys->p_i->vkGetDescriptorSetLayoutBindingOffsetEXT(device, p_sys->p_i->vk_desc_set_layout, 2, &p_sys->p_i->offset_rw_panels);
        p_sys->p_i->vkGetDescriptorSetLayoutBindingOffsetEXT(device, p_sys->p_i->vk_desc_set_layout, 3, &p_sys->p_i->offset_textures);
        p_sys->p_i->vkGetDescriptorSetLayoutBindingOffsetEXT(device, p_sys->p_i->vk_desc_set_layout, 4, &p_sys->p_i->offset_fonts);
        p_sys->p_i->vkGetDescriptorSetLayoutBindingOffsetEXT(device, p_sys->p_i->vk_desc_set_layout, 5, &p_sys->p_i->offset_gbuff_base_colour);
        p_sys->p_i->vkGetDescriptorSetLayoutBindingOffsetEXT(device, p_sys->p_i->vk_desc_set_layout, 6, &p_sys->p_i->offset_gbuff_roughness);
        p_sys->p_i->vkGetDescriptorSetLayoutBindingOffsetEXT(device, p_sys->p_i->vk_desc_set_layout, 7, &p_sys->p_i->offset_gbuff_specular);
        p_sys->p_i->vkGetDescriptorSetLayoutBindingOffsetEXT(device, p_sys->p_i->vk_desc_set_layout, 8, &p_sys->p_i->offset_gbuff_metallic);
        p_sys->p_i->vkGetDescriptorSetLayoutBindingOffsetEXT(device, p_sys->p_i->vk_desc_set_layout, 9, &p_sys->p_i->offset_gbuff_emission);
        p_sys->p_i->vkGetDescriptorSetLayoutBindingOffsetEXT(device, p_sys->p_i->vk_desc_set_layout, 10, &p_sys->p_i->offset_gbuff_pos_world);
        p_sys->p_i->vkGetDescriptorSetLayoutBindingOffsetEXT(device, p_sys->p_i->vk_desc_set_layout, 11, &p_sys->p_i->offset_gbuff_nrm_world);
    }

    // =============================================================================================
    // =============================================================================================
    // create descriptor buffer
    // =============================================================================================
    // =============================================================================================
    {
        VkDeviceSize buff_size      = p_sys->p_i->dsl_size;
        VkBufferUsageFlags usage    = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT;
        VkMemoryPropertyFlags props = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

        vk_buffer_create_z(device, phys_dev, buff_size, usage, props, &p_sys->desc_buff_handle, &p_sys->desc_buff_memory);

        p_sys->desc_buff_device_addr = vk_buffer_device_address_z(device, p_sys->desc_buff_handle);

        VK_CHECK(vkMapMemory(device, p_sys->desc_buff_memory, 0, buff_size, 0, &p_sys->desc_buff_p_mapped), "mapping descriptor buffer memory");
    }
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_descriptors_set_samp_z: Set the sampler descriptor in the descriptor buffer.
// =========================================================================================================================================
// =========================================================================================================================================
void vk_descriptors_set_samp_z(VkDevice device, vk_descriptors_system_zt* p_sys, VkSampler sampler)
{
    fatal_check_z(device, "device is null");
    fatal_check_z(p_sys, "p_sys is null");
    fatal_check_z(p_sys->p_i, "p_sys->p_i is null");

    VkDescriptorDataEXT data = {
        .pSampler = &sampler,
    };

    VkDescriptorGetInfoEXT info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
        .pNext = nullptr,
        .type  = VK_DESCRIPTOR_TYPE_SAMPLER,
        .data  = data,
    };

    p_sys->p_i->vkGetDescriptorEXT(device, &info, p_sys->p_i->desc_buff_props.samplerDescriptorSize, (uint8_t*)p_sys->desc_buff_p_mapped + p_sys->p_i->offset_sampler);
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_descriptors_set_panel_z: Set panel image descriptors (both sampled and storage) at the specified index in the descriptor buffer.
// =========================================================================================================================================
// =========================================================================================================================================
void vk_descriptors_set_panel_z(VkDevice device, vk_descriptors_system_zt* p_sys, uint32_t idx, VkImageView image_view)
{
    fatal_check_z(device, "device is null");
    fatal_check_z(p_sys, "p_sys is null");
    fatal_check_z(p_sys->p_i, "p_sys->p_i is null");
    fatal_check_bool_z(idx < MAX_PANELS, "panel index out of bounds");

    // =============================================================================================
    // =============================================================================================
    // set sampled image
    // =============================================================================================
    // =============================================================================================
    {
        VkDescriptorImageInfo img_info = {
            .imageView   = image_view,
            .sampler     = VK_NULL_HANDLE,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };

        VkDescriptorDataEXT data = {
            .pSampledImage = &img_info,
        };

        VkDescriptorGetInfoEXT info = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
            .pNext = nullptr,
            .type  = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
            .data  = data,
        };

        p_sys->p_i->vkGetDescriptorEXT(device, &info, p_sys->p_i->desc_buff_props.sampledImageDescriptorSize, (uint8_t*)p_sys->desc_buff_p_mapped + p_sys->p_i->offset_panels + idx * p_sys->p_i->desc_buff_props.sampledImageDescriptorSize);
    }

    // =============================================================================================
    // =============================================================================================
    // set storage image
    // =============================================================================================
    // =============================================================================================
    {
        VkDescriptorImageInfo img_info = {
            .imageView   = image_view,
            .sampler     = VK_NULL_HANDLE,
            .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
        };

        VkDescriptorDataEXT data = {
            .pStorageImage = &img_info,
        };

        VkDescriptorGetInfoEXT info = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
            .pNext = nullptr,
            .type  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .data  = data,
        };

        p_sys->p_i->vkGetDescriptorEXT(device, &info, p_sys->p_i->desc_buff_props.storageImageDescriptorSize, (uint8_t*)p_sys->desc_buff_p_mapped + p_sys->p_i->offset_rw_panels + idx * p_sys->p_i->desc_buff_props.storageImageDescriptorSize);
    }
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_descriptors_set_tex_z: Set texture image descriptor at the specified index in the descriptor buffer.
// =========================================================================================================================================
// =========================================================================================================================================
void vk_descriptors_set_tex_z(VkDevice device, vk_descriptors_system_zt* p_sys, uint32_t idx, VkDescriptorImageInfo img_info)
{
    fatal_check_z(device, "device is null");
    fatal_check_z(p_sys, "p_sys is null");
    fatal_check_z(p_sys->p_i, "p_sys->p_i is null");
    fatal_check_bool_z(idx < p_sys->p_i->max_textures, "texture index out of bounds");

    VkDescriptorDataEXT data = {
        .pSampledImage = &img_info,
    };

    VkDescriptorGetInfoEXT info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
        .pNext = nullptr,
        .type  = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
        .data  = data,
    };

    p_sys->p_i->vkGetDescriptorEXT(device, &info, p_sys->p_i->desc_buff_props.sampledImageDescriptorSize, (uint8_t*)p_sys->desc_buff_p_mapped + p_sys->p_i->offset_textures + idx * p_sys->p_i->desc_buff_props.sampledImageDescriptorSize);
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_descriptors_set_font_z: Set font image descriptor at the specified index in the descriptor buffer.
// =========================================================================================================================================
// =========================================================================================================================================
void vk_descriptors_set_font_z(VkDevice device, vk_descriptors_system_zt* p_sys, uint32_t idx, VkDescriptorImageInfo img_info)
{
    fatal_check_z(device, "device is null");
    fatal_check_z(p_sys, "p_sys is null");
    fatal_check_z(p_sys->p_i, "p_sys->p_i is null");
    fatal_check_bool_z(idx < p_sys->p_i->max_fonts, "font index out of bounds");

    VkDescriptorDataEXT data = {
        .pSampledImage = &img_info,
    };

    VkDescriptorGetInfoEXT info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
        .pNext = nullptr,
        .type  = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
        .data  = data,
    };

    p_sys->p_i->vkGetDescriptorEXT(device, &info, p_sys->p_i->desc_buff_props.sampledImageDescriptorSize, (uint8_t*)p_sys->desc_buff_p_mapped + p_sys->p_i->offset_fonts + idx * p_sys->p_i->desc_buff_props.sampledImageDescriptorSize);
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_descriptors_set_gbuff_base_colour_z: Set g-buffer base colour image descriptor at the specified index in the descriptor buffer.
// =========================================================================================================================================
// =========================================================================================================================================
void vk_descriptors_set_gbuff_base_colour_z(VkDevice device, vk_descriptors_system_zt* p_sys, uint32_t idx, VkImageView image_view)
{
    fatal_check_z(device, "device is null");
    fatal_check_z(p_sys, "p_sys is null");
    fatal_check_z(p_sys->p_i, "p_sys->p_i is null");
    fatal_check_bool_z(idx < MAX_PANELS, "gbuff index out of bounds");

    VkDescriptorImageInfo img_info = {
        .imageView   = image_view,
        .sampler     = VK_NULL_HANDLE,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };

    VkDescriptorDataEXT data = {
        .pSampledImage = &img_info,
    };

    VkDescriptorGetInfoEXT info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
        .pNext = nullptr,
        .type  = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
        .data  = data,
    };

    p_sys->p_i->vkGetDescriptorEXT(device, &info, p_sys->p_i->desc_buff_props.sampledImageDescriptorSize, (uint8_t*)p_sys->desc_buff_p_mapped + p_sys->p_i->offset_gbuff_base_colour + idx * p_sys->p_i->desc_buff_props.sampledImageDescriptorSize);
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_descriptors_set_gbuff_roughness_z: Set g-buffer roughness image descriptor at the specified index in the descriptor buffer.
// =========================================================================================================================================
// =========================================================================================================================================
void vk_descriptors_set_gbuff_roughness_z(VkDevice device, vk_descriptors_system_zt* p_sys, uint32_t idx, VkImageView image_view)
{
    fatal_check_z(device, "device is null");
    fatal_check_z(p_sys, "p_sys is null");
    fatal_check_z(p_sys->p_i, "p_sys->p_i is null");
    fatal_check_bool_z(idx < MAX_PANELS, "gbuff index out of bounds");

    VkDescriptorImageInfo img_info = {
        .imageView   = image_view,
        .sampler     = VK_NULL_HANDLE,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };

    VkDescriptorDataEXT data = {
        .pSampledImage = &img_info,
    };

    VkDescriptorGetInfoEXT info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
        .pNext = nullptr,
        .type  = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
        .data  = data,
    };

    p_sys->p_i->vkGetDescriptorEXT(device, &info, p_sys->p_i->desc_buff_props.sampledImageDescriptorSize, (uint8_t*)p_sys->desc_buff_p_mapped + p_sys->p_i->offset_gbuff_roughness + idx * p_sys->p_i->desc_buff_props.sampledImageDescriptorSize);
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_descriptors_set_gbuff_specular_z: Set g-buffer specular image descriptor at the specified index in the descriptor buffer.
// =========================================================================================================================================
// =========================================================================================================================================
void vk_descriptors_set_gbuff_specular_z(VkDevice device, vk_descriptors_system_zt* p_sys, uint32_t idx, VkImageView image_view)
{
    fatal_check_z(device, "device is null");
    fatal_check_z(p_sys, "p_sys is null");
    fatal_check_z(p_sys->p_i, "p_sys->p_i is null");
    fatal_check_bool_z(idx < MAX_PANELS, "gbuff index out of bounds");

    VkDescriptorImageInfo img_info = {
        .imageView   = image_view,
        .sampler     = VK_NULL_HANDLE,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };

    VkDescriptorDataEXT data = {
        .pSampledImage = &img_info,
    };

    VkDescriptorGetInfoEXT info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
        .pNext = nullptr,
        .type  = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
        .data  = data,
    };

    p_sys->p_i->vkGetDescriptorEXT(device, &info, p_sys->p_i->desc_buff_props.sampledImageDescriptorSize, (uint8_t*)p_sys->desc_buff_p_mapped + p_sys->p_i->offset_gbuff_specular + idx * p_sys->p_i->desc_buff_props.sampledImageDescriptorSize);
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_descriptors_set_gbuff_metallic_z: Set g-buffer metallic image descriptor at the specified index in the descriptor buffer.
// =========================================================================================================================================
// =========================================================================================================================================
void vk_descriptors_set_gbuff_metallic_z(VkDevice device, vk_descriptors_system_zt* p_sys, uint32_t idx, VkImageView image_view)
{
    fatal_check_z(device, "device is null");
    fatal_check_z(p_sys, "p_sys is null");
    fatal_check_z(p_sys->p_i, "p_sys->p_i is null");
    fatal_check_bool_z(idx < MAX_PANELS, "gbuff index out of bounds");

    VkDescriptorImageInfo img_info = {
        .imageView   = image_view,
        .sampler     = VK_NULL_HANDLE,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };

    VkDescriptorDataEXT data = {
        .pSampledImage = &img_info,
    };

    VkDescriptorGetInfoEXT info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
        .pNext = nullptr,
        .type  = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
        .data  = data,
    };

    p_sys->p_i->vkGetDescriptorEXT(device, &info, p_sys->p_i->desc_buff_props.sampledImageDescriptorSize, (uint8_t*)p_sys->desc_buff_p_mapped + p_sys->p_i->offset_gbuff_metallic + idx * p_sys->p_i->desc_buff_props.sampledImageDescriptorSize);
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_descriptors_set_gbuff_emission_z: Set g-buffer emission image descriptor at the specified index in the descriptor buffer.
// =========================================================================================================================================
// =========================================================================================================================================
void vk_descriptors_set_gbuff_emission_z(VkDevice device, vk_descriptors_system_zt* p_sys, uint32_t idx, VkImageView image_view)
{
    fatal_check_z(device, "device is null");
    fatal_check_z(p_sys, "p_sys is null");
    fatal_check_z(p_sys->p_i, "p_sys->p_i is null");
    fatal_check_bool_z(idx < MAX_PANELS, "gbuff index out of bounds");

    VkDescriptorImageInfo img_info = {
        .imageView   = image_view,
        .sampler     = VK_NULL_HANDLE,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };

    VkDescriptorDataEXT data = {
        .pSampledImage = &img_info,
    };

    VkDescriptorGetInfoEXT info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
        .pNext = nullptr,
        .type  = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
        .data  = data,
    };

    p_sys->p_i->vkGetDescriptorEXT(device, &info, p_sys->p_i->desc_buff_props.sampledImageDescriptorSize, (uint8_t*)p_sys->desc_buff_p_mapped + p_sys->p_i->offset_gbuff_emission + idx * p_sys->p_i->desc_buff_props.sampledImageDescriptorSize);
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_descriptors_set_gbuff_pos_world_z: Set g-buffer position world image descriptor at the specified index in the descriptor buffer.
// =========================================================================================================================================
// =========================================================================================================================================
void vk_descriptors_set_gbuff_pos_world_z(VkDevice device, vk_descriptors_system_zt* p_sys, uint32_t idx, VkImageView image_view)
{
    fatal_check_z(device, "device is null");
    fatal_check_z(p_sys, "p_sys is null");
    fatal_check_z(p_sys->p_i, "p_sys->p_i is null");
    fatal_check_bool_z(idx < MAX_PANELS, "gbuff index out of bounds");

    VkDescriptorImageInfo img_info = {
        .imageView   = image_view,
        .sampler     = VK_NULL_HANDLE,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };

    VkDescriptorDataEXT data = {
        .pSampledImage = &img_info,
    };

    VkDescriptorGetInfoEXT info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
        .pNext = nullptr,
        .type  = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
        .data  = data,
    };

    p_sys->p_i->vkGetDescriptorEXT(device, &info, p_sys->p_i->desc_buff_props.sampledImageDescriptorSize, (uint8_t*)p_sys->desc_buff_p_mapped + p_sys->p_i->offset_gbuff_pos_world + idx * p_sys->p_i->desc_buff_props.sampledImageDescriptorSize);
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_descriptors_set_gbuff_nrm_world_z: Set g-buffer normal world image descriptor at the specified index in the descriptor buffer.
// =========================================================================================================================================
// =========================================================================================================================================
void vk_descriptors_set_gbuff_nrm_world_z(VkDevice device, vk_descriptors_system_zt* p_sys, uint32_t idx, VkImageView image_view)
{
    fatal_check_z(device, "device is null");
    fatal_check_z(p_sys, "p_sys is null");
    fatal_check_z(p_sys->p_i, "p_sys->p_i is null");
    fatal_check_bool_z(idx < MAX_PANELS, "gbuff index out of bounds");

    VkDescriptorImageInfo img_info = {
        .imageView   = image_view,
        .sampler     = VK_NULL_HANDLE,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };

    VkDescriptorDataEXT data = {
        .pSampledImage = &img_info,
    };

    VkDescriptorGetInfoEXT info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
        .pNext = nullptr,
        .type  = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
        .data  = data,
    };

    p_sys->p_i->vkGetDescriptorEXT(device, &info, p_sys->p_i->desc_buff_props.sampledImageDescriptorSize, (uint8_t*)p_sys->desc_buff_p_mapped + p_sys->p_i->offset_gbuff_nrm_world + idx * p_sys->p_i->desc_buff_props.sampledImageDescriptorSize);
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_descriptors_cleanup_z: Clean up descriptor system resources. Unmaps and destroys the descriptor buffer and frees internal structures.
// =========================================================================================================================================
// =========================================================================================================================================
void vk_descriptors_cleanup_z(VkDevice device, vk_descriptors_system_zt* p_sys)
{
    fatal_check_z(device, "device is null");
    fatal_check_z(p_sys, "p_sys is null");

    if (p_sys->p_i)
    {
        if (p_sys->desc_buff_p_mapped)
        {
            vkUnmapMemory(device, p_sys->desc_buff_memory);
        }
        if (p_sys->desc_buff_handle)
        {
            vkDestroyBuffer(device, p_sys->desc_buff_handle, nullptr);
        }
        if (p_sys->desc_buff_memory)
        {
            vkFreeMemory(device, p_sys->desc_buff_memory, nullptr);
        }
        if (p_sys->p_i->vk_desc_set_layout)
        {
            vkDestroyDescriptorSetLayout(device, p_sys->p_i->vk_desc_set_layout, nullptr);
        }
        if (p_sys->vk_pipeline_layout)
        {
            vkDestroyPipelineLayout(device, p_sys->vk_pipeline_layout, nullptr);
        }
        free(p_sys->p_i);
        p_sys->p_i = nullptr;
    }
}
