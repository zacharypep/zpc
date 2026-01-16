#include "zpc/vk.h"

#include "zpc/fatal.h"

#include <vulkan/vulkan.h>
#include <stdlib.h>

// =========================================================================================================================================
// =========================================================================================================================================
// vk_tlas_init_z: Initialize a top-level acceleration structure (TLAS) with the specified maximum instance count. Creates instance buffer,
// acceleration structure buffer, and scratch buffer. The TLAS must be cleaned up with vk_tlas_cleanup_z.
// =========================================================================================================================================
// =========================================================================================================================================
void vk_tlas_init_z(vk_instance_zt* p_inst, VkDevice device, VkPhysicalDevice phys_dev, vk_tlas_zt* p_tlas, uint32_t MAX_INSTS)
{
    fatal_check_z(p_inst, "p_inst is null");
    fatal_check_z(device, "device is null");
    fatal_check_z(phys_dev, "phys_dev is null");
    fatal_check_z(p_tlas, "p_tlas is null");
    fatal_check_bool_z(MAX_INSTS > 0, "MAX_INSTS must be > 0");

    p_tlas->p_inst    = p_inst;
    p_tlas->MAX_INSTS = MAX_INSTS;

    // ========================================================================================
    // ========================================================================================
    // init instance buffer used to rebuild tlases
    // ========================================================================================
    // ========================================================================================
    {
        vk_device_buff_init_z(device, phys_dev, &p_tlas->insts_buff, sizeof(VkAccelerationStructureInstanceKHR), MAX_INSTS, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR);
    }

    // ========================================================================================
    // ========================================================================================
    // get build size info
    // note
    // using max size for convenience, no future re-creates needed
    // ========================================================================================
    // ========================================================================================
    VkAccelerationStructureBuildSizesInfoKHR build_sizes_info = {};
    {
        build_sizes_info.sType                                      = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

        VkAccelerationStructureGeometryInstancesDataKHR insts_data  = {};
        insts_data.sType                                            = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
        insts_data.arrayOfPointers                                  = VK_FALSE;
        insts_data.data.deviceAddress                               = p_tlas->insts_buff.deviceAddress;

        VkAccelerationStructureGeometryKHR as_geom                  = {};
        as_geom.sType                                               = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
        as_geom.geometryType                                        = VK_GEOMETRY_TYPE_INSTANCES_KHR;
        as_geom.flags                                               = VK_GEOMETRY_OPAQUE_BIT_KHR;
        as_geom.geometry.instances                                  = insts_data;

        VkAccelerationStructureBuildGeometryInfoKHR build_geom_info = {};
        build_geom_info.sType                                       = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        build_geom_info.type                                        = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
        build_geom_info.flags                                       = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        build_geom_info.geometryCount                               = 1;
        build_geom_info.pGeometries                                 = &as_geom;

        uint32_t prim_count                                         = MAX_INSTS;

        p_inst->func_ptrs.vkGetAccelerationStructureBuildSizesKHR(device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &build_geom_info, &prim_count, &build_sizes_info);
    }

    // ============================================================================
    // ============================================================================
    // create tlas buff and as
    // ============================================================================
    // ============================================================================
    {
        vk_buffer_create_z(device, phys_dev, build_sizes_info.accelerationStructureSize, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &p_tlas->buffer, &p_tlas->memory);

        VkAccelerationStructureCreateInfoKHR info = {};
        info.sType                                = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
        info.buffer                               = p_tlas->buffer;
        info.size                                 = build_sizes_info.accelerationStructureSize;
        info.type                                 = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;

        p_inst->func_ptrs.vkCreateAccelerationStructureKHR(device, &info, nullptr, &p_tlas->handle);
    }

    // ============================================================================
    // ============================================================================
    // init scratch buff
    // ============================================================================
    // ============================================================================
    vk_hidden_device_local_buff_init_z(device, phys_dev, &p_tlas->scratch_buff, build_sizes_info.buildScratchSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_tlas_record_build_z: Record a command to build the TLAS into the command buffer. The instance buffer must be populated before calling
// this function.
// =========================================================================================================================================
// =========================================================================================================================================
void vk_tlas_record_build_z(vk_instance_zt* p_inst, VkCommandBuffer cmd_buff, vk_tlas_zt* p_tlas)
{
    fatal_check_z(p_inst, "p_inst is null");
    fatal_check_z(cmd_buff, "cmd_buff is null");
    fatal_check_z(p_tlas, "p_tlas is null");

    VkAccelerationStructureGeometryInstancesDataKHR data            = {};
    data.sType                                                      = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    data.arrayOfPointers                                            = VK_FALSE;
    data.data.deviceAddress                                         = p_tlas->insts_buff.deviceAddress;

    VkAccelerationStructureGeometryKHR geom                         = {};
    geom.sType                                                      = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    geom.geometryType                                               = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    geom.flags                                                      = VK_GEOMETRY_OPAQUE_BIT_KHR;
    geom.geometry.instances                                         = data;

    VkAccelerationStructureBuildGeometryInfoKHR build_info          = {};
    build_info.sType                                                = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    build_info.type                                                 = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    build_info.flags                                                = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    build_info.mode                                                 = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    build_info.dstAccelerationStructure                             = p_tlas->handle;
    build_info.geometryCount                                        = 1;
    build_info.pGeometries                                          = &geom;
    build_info.scratchData.deviceAddress                            = p_tlas->scratch_buff.device_addr;

    VkAccelerationStructureBuildRangeInfoKHR range_info             = {};
    range_info.primitiveCount                                       = p_tlas->insts_buff.count;
    range_info.primitiveOffset                                      = 0;
    range_info.firstVertex                                          = 0;
    range_info.transformOffset                                      = 0;

    const VkAccelerationStructureBuildRangeInfoKHR* p_range_infos[] = {&range_info};

    p_inst->func_ptrs.vkCmdBuildAccelerationStructuresKHR(cmd_buff, 1, &build_info, p_range_infos);
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_tlas_desc_info_z: Get descriptor acceleration structure info for use in descriptor sets.
// =========================================================================================================================================
// =========================================================================================================================================
VkWriteDescriptorSetAccelerationStructureKHR vk_tlas_desc_info_z(const vk_tlas_zt* p_tlas)
{
    fatal_check_z(p_tlas, "p_tlas is null");

    VkWriteDescriptorSetAccelerationStructureKHR info = {};
    info.sType                                        = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
    info.accelerationStructureCount                   = 1;
    info.pAccelerationStructures                      = &p_tlas->handle;

    return info;
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_tlas_cleanup_z: Clean up TLAS resources. Destroys acceleration structure, buffers, and frees memory.
// =========================================================================================================================================
// =========================================================================================================================================
void vk_tlas_cleanup_z(vk_tlas_zt* p_tlas)
{
    fatal_check_z(p_tlas, "p_tlas is null");

    vk_hidden_device_local_buff_cleanup_z(p_tlas->p_inst->vk_dev, &p_tlas->scratch_buff);
    vk_device_buff_cleanup_z(p_tlas->p_inst->vk_dev, &p_tlas->insts_buff);

    if (p_tlas->handle != VK_NULL_HANDLE)
    {
        p_tlas->p_inst->func_ptrs.vkDestroyAccelerationStructureKHR(p_tlas->p_inst->vk_dev, p_tlas->handle, nullptr);
        p_tlas->handle = VK_NULL_HANDLE;
    }

    if (p_tlas->memory != VK_NULL_HANDLE)
    {
        vkFreeMemory(p_tlas->p_inst->vk_dev, p_tlas->memory, nullptr);
        p_tlas->memory = VK_NULL_HANDLE;
    }

    if (p_tlas->buffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(p_tlas->p_inst->vk_dev, p_tlas->buffer, nullptr);
        p_tlas->buffer = VK_NULL_HANDLE;
    }
}
