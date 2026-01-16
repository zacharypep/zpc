#include "zpc/vk.h"

#include "zpc/fatal.h"

#include <vulkan/vulkan.h>
#include <stdlib.h>
#include <string.h>

// =========================================================================================================================================
// =========================================================================================================================================
// vk_blas_init_tri_blas_z: Initialize a triangle-based bottom-level acceleration structure (BLAS). Calculates build sizes and creates
// acceleration structure buffer and scratch buffer. The BLAS must be cleaned up with vk_blas_cleanup_z.
// =========================================================================================================================================
// =========================================================================================================================================
void vk_blas_init_tri_blas_z(vk_instance_zt* p_inst, VkDevice device, VkPhysicalDevice phys_dev, vk_blas_zt* p_blas, const uint32_t* p_submesh_tri_counts, uint32_t submesh_count)
{
    fatal_check_z(p_inst, "p_inst is null");
    fatal_check_z(device, "device is null");
    fatal_check_z(phys_dev, "phys_dev is null");
    fatal_check_z(p_blas, "p_blas is null");
    fatal_check_z(p_submesh_tri_counts, "p_submesh_tri_counts is null");
    fatal_check_bool_z(submesh_count > 0, "submesh_count must be > 0");

    p_blas->p_inst                               = p_inst;
    p_blas->device                               = device;
    p_blas->num_tris_initialised                 = -1;

    // ========================================================================================
    // ========================================================================================
    // compose as geometries
    // ========================================================================================
    // ========================================================================================
    VkAccelerationStructureGeometryKHR* as_geoms = fatal_alloc_z(sizeof(VkAccelerationStructureGeometryKHR) * submesh_count, "failed to allocate as_geoms");
    {
        VkAccelerationStructureGeometryTrianglesDataKHR tris = {};
        tris.sType                                           = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
        tris.vertexFormat                                    = VK_FORMAT_R32G32B32_SFLOAT;
        tris.vertexStride                                    = sizeof(vec3_zt);
        tris.indexType                                       = VK_INDEX_TYPE_UINT32;

        VkAccelerationStructureGeometryKHR geom              = {};
        geom.sType                                           = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
        geom.flags                                           = VK_GEOMETRY_NO_DUPLICATE_ANY_HIT_INVOCATION_BIT_KHR;
        geom.geometryType                                    = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
        geom.geometry.triangles                              = tris;

        for (uint32_t i = 0; i < submesh_count; i++)
        {
            as_geoms[i] = geom;
        }
    }

    // ========================================================================================
    // ========================================================================================
    // get build sizes info
    // ========================================================================================
    // ========================================================================================
    VkAccelerationStructureBuildSizesInfoKHR sizes_info = {};
    {
        sizes_info.sType                                       = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

        VkAccelerationStructureBuildGeometryInfoKHR geom_info  = {};
        geom_info.sType                                        = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        geom_info.type                                         = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        geom_info.flags                                        = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        geom_info.flags                                       |= VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_DATA_ACCESS_KHR;
        geom_info.geometryCount                                = submesh_count;
        geom_info.pGeometries                                  = as_geoms;

        p_inst->func_ptrs.vkGetAccelerationStructureBuildSizesKHR(device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &geom_info, p_submesh_tri_counts, &sizes_info);
    }

    free(as_geoms);

    // ========================================================================================
    // ========================================================================================
    // create blas buffer
    // ========================================================================================
    // ========================================================================================
    {
        vk_buffer_create_z(device, phys_dev, sizes_info.accelerationStructureSize, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &p_blas->buffer, &p_blas->memory);
    }

    // ========================================================================================
    // ========================================================================================
    // create acceleration structure
    // ========================================================================================
    // ========================================================================================
    {
        VkAccelerationStructureCreateInfoKHR info = {};
        info.sType                                = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
        info.buffer                               = p_blas->buffer;
        info.size                                 = sizes_info.accelerationStructureSize;
        info.type                                 = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

        p_inst->func_ptrs.vkCreateAccelerationStructureKHR(device, &info, nullptr, &p_blas->handle);
    }

    // ========================================================================================
    // ========================================================================================
    // create its scratch buffer
    // ========================================================================================
    // ========================================================================================
    {
        vk_hidden_device_local_buff_init_z(device, phys_dev, &p_blas->scratch_buff, sizes_info.buildScratchSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
    }

    // ========================================================================================
    // ========================================================================================
    // store blas device address so tlas can reference it
    // ========================================================================================
    // ========================================================================================
    {
        p_blas->deviceAddress = vk_get_as_dev_addr_z(p_inst, device, p_blas->handle);
    }

    // ========================================================================================
    // ========================================================================================
    // store how many tris this blas was initialised for
    // ========================================================================================
    // ========================================================================================
    {
        int count = 0;
        for (uint32_t i = 0; i < submesh_count; i++)
        {
            count += (int)p_submesh_tri_counts[i];
        }
        p_blas->num_tris_initialised = count;
    }
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_blas_record_build_tri_blas_z: Record a command to build a triangle-based BLAS into the command buffer. The vertex and index buffers
// must be populated before calling this function.
// =========================================================================================================================================
// =========================================================================================================================================
void vk_blas_record_build_tri_blas_z(
    vk_instance_zt* p_inst,
    VkDevice vk_dev,
    VkPhysicalDevice vk_phys_dev,
    VkCommandBuffer cmd_buff,
    vk_blas_zt* p_blas,
    VkDeviceAddress verts_buff_addr,
    VkDeviceAddress idcs_buff_addr,
    const uint32_t* p_mapped_idcs,
    const vk_region_handle_zt* p_submesh_verts_regions,
    uint32_t verts_region_count,
    const vk_region_handle_zt* p_submesh_idcs_regions,
    uint32_t idcs_region_count
)
{
    fatal_check_z(p_inst, "p_inst is null");
    fatal_check_z(vk_dev, "vk_dev is null");
    fatal_check_z(vk_phys_dev, "vk_phys_dev is null");
    fatal_check_z(cmd_buff, "cmd_buff is null");
    fatal_check_z(p_blas, "p_blas is null");
    fatal_check_z(p_mapped_idcs, "p_mapped_idcs is null");
    fatal_check_z(p_submesh_verts_regions, "p_submesh_verts_regions is null");
    fatal_check_z(p_submesh_idcs_regions, "p_submesh_idcs_regions is null");
    fatal_check_bool_z(verts_region_count > 0, "verts_region_count must be > 0");
    fatal_check_bool_z(idcs_region_count > 0, "idcs_region_count must be > 0");
    fatal_check_bool_z(verts_region_count == idcs_region_count, "verts_region_count must equal idcs_region_count");

    // ========================================================================================
    // ========================================================================================
    // compose as geometries
    // ========================================================================================
    // ========================================================================================
    VkAccelerationStructureGeometryKHR* asGeometries = fatal_alloc_z(sizeof(VkAccelerationStructureGeometryKHR) * idcs_region_count, "failed to allocate asGeometries");
    {
        for (uint32_t i = 0; i < idcs_region_count; i++)
        {
            const vk_region_handle_zt* region                    = &p_submesh_idcs_regions[i];

            VkAccelerationStructureGeometryTrianglesDataKHR tris = {};
            tris.sType                                           = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
            tris.vertexFormat                                    = VK_FORMAT_R32G32B32_SFLOAT;
            tris.vertexData.deviceAddress                        = verts_buff_addr;
            tris.maxVertex                                       = p_mapped_idcs[region->start_idx + region->count - 1];
            tris.vertexStride                                    = sizeof(vec3_zt);
            tris.indexType                                       = VK_INDEX_TYPE_UINT32;
            tris.indexData.deviceAddress                         = idcs_buff_addr;
            tris.transformData.deviceAddress                     = 0;
            tris.transformData.hostAddress                       = nullptr;

            VkAccelerationStructureGeometryKHR geom              = {};
            geom.sType                                           = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
            geom.flags                                           = VK_GEOMETRY_NO_DUPLICATE_ANY_HIT_INVOCATION_BIT_KHR;
            geom.geometryType                                    = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
            geom.geometry.triangles                              = tris;

            asGeometries[i]                                      = geom;
        }
    }

    // ========================================================================================
    // ========================================================================================
    // record build
    // ========================================================================================
    // ========================================================================================
    {
        VkAccelerationStructureBuildGeometryInfoKHR geomInfo         = {};
        geomInfo.sType                                               = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        geomInfo.type                                                = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        geomInfo.flags                                               = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_DATA_ACCESS_KHR;
        geomInfo.mode                                                = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        geomInfo.dstAccelerationStructure                            = p_blas->handle;
        geomInfo.geometryCount                                       = idcs_region_count;
        geomInfo.pGeometries                                         = asGeometries;
        geomInfo.scratchData.deviceAddress                           = p_blas->scratch_buff.device_addr;

        VkAccelerationStructureBuildRangeInfoKHR* rangeInfos         = fatal_alloc_z(sizeof(VkAccelerationStructureBuildRangeInfoKHR) * verts_region_count, "failed to allocate rangeInfos");
        const VkAccelerationStructureBuildRangeInfoKHR** pRangeInfos = fatal_alloc_z(sizeof(VkAccelerationStructureBuildRangeInfoKHR*) * verts_region_count, "failed to allocate pRangeInfos");

        for (uint32_t i = 0; i < verts_region_count; i++)
        {
            const vk_region_handle_zt* verts_region       = &p_submesh_verts_regions[i];
            const vk_region_handle_zt* idcs_region        = &p_submesh_idcs_regions[i];

            VkAccelerationStructureBuildRangeInfoKHR info = {};
            info.firstVertex                              = verts_region->start_idx;
            info.primitiveOffset                          = idcs_region->start_idx * sizeof(uint32_t);
            info.primitiveCount                           = idcs_region->count / 3;
            info.transformOffset                          = 0;

            rangeInfos[i]                                 = info;
            pRangeInfos[i]                                = &rangeInfos[i];
        }

        p_inst->func_ptrs.vkCmdBuildAccelerationStructuresKHR(cmd_buff, 1, &geomInfo, pRangeInfos);

        free(pRangeInfos);
        free(rangeInfos);
    }

    free(asGeometries);
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_blas_record_setup_sphere_blas_z: Initialize and record a command to build a sphere-based BLAS (using AABBs) into the command buffer.
// Creates acceleration structure buffer, scratch buffer, and records the build command.
// =========================================================================================================================================
// =========================================================================================================================================
void vk_blas_record_setup_sphere_blas_z(vk_instance_zt* p_inst, VkDevice device, VkPhysicalDevice phys_dev, VkCommandBuffer cmd_buff, vk_blas_zt* p_blas, VkDeviceAddress aabb_pos_device_addr)
{
    fatal_check_z(p_inst, "p_inst is null");
    fatal_check_z(device, "device is null");
    fatal_check_z(phys_dev, "phys_dev is null");
    fatal_check_z(cmd_buff, "cmd_buff is null");
    fatal_check_z(p_blas, "p_blas is null");

    p_blas->p_inst = p_inst;
    p_blas->device = device;

    // ========================================================================================
    // ========================================================================================
    // compose VkAccelerationStructureGeometryKHR
    // ========================================================================================
    // ========================================================================================
    VkAccelerationStructureGeometryKHR as_geom;
    {

        VkAccelerationStructureGeometryAabbsDataKHR aabbs = {};
        aabbs.sType                                       = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR;
        aabbs.stride                                      = sizeof(VkAabbPositionsKHR);
        aabbs.data.deviceAddress                          = aabb_pos_device_addr;

        VkAccelerationStructureGeometryKHR geom           = {};
        geom.sType                                        = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
        geom.flags                                        = VK_GEOMETRY_OPAQUE_BIT_KHR;
        geom.geometryType                                 = VK_GEOMETRY_TYPE_AABBS_KHR;
        geom.geometry.aabbs                               = aabbs;

        as_geom                                           = geom;
    }

    // ========================================================================================
    // ========================================================================================
    // get build sizes info
    // ========================================================================================
    // ========================================================================================
    VkAccelerationStructureBuildSizesInfoKHR sizes_info = {};
    {
        sizes_info.sType                                       = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

        VkAccelerationStructureBuildGeometryInfoKHR build_info = {};
        build_info.sType                                       = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        build_info.type                                        = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        build_info.flags                                       = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        build_info.geometryCount                               = 1;
        build_info.pGeometries                                 = &as_geom;

        uint32_t prim_counts[]                                 = {1};

        p_inst->func_ptrs.vkGetAccelerationStructureBuildSizesKHR(device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &build_info, prim_counts, &sizes_info);
    }

    // ========================================================================================
    // ========================================================================================
    // create blas buffer
    // ========================================================================================
    // ========================================================================================
    vk_buffer_create_z(device, phys_dev, sizes_info.accelerationStructureSize, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &p_blas->buffer, &p_blas->memory);

    // ========================================================================================
    // ========================================================================================
    // create acceleration structure
    // ========================================================================================
    // ========================================================================================
    {
        VkAccelerationStructureCreateInfoKHR createInfo = {};
        createInfo.sType                                = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
        createInfo.buffer                               = p_blas->buffer;
        createInfo.size                                 = sizes_info.accelerationStructureSize;
        createInfo.type                                 = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

        p_inst->func_ptrs.vkCreateAccelerationStructureKHR(device, &createInfo, nullptr, &p_blas->handle);
    }

    // ========================================================================================
    // ========================================================================================
    // create its scratch buffer
    // ========================================================================================
    // ========================================================================================
    vk_hidden_device_local_buff_init_z(device, phys_dev, &p_blas->scratch_buff, sizes_info.buildScratchSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);

    // ========================================================================================
    // ========================================================================================
    // record build
    // ========================================================================================
    // ========================================================================================
    {
        VkAccelerationStructureBuildGeometryInfoKHR build_info = {};
        build_info.sType                                       = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        build_info.type                                        = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        build_info.flags                                       = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        build_info.mode                                        = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        build_info.dstAccelerationStructure                    = p_blas->handle;
        build_info.geometryCount                               = 1;
        build_info.pGeometries                                 = &as_geom;
        build_info.scratchData.deviceAddress                   = p_blas->scratch_buff.device_addr;

        VkAccelerationStructureBuildRangeInfoKHR rangeInfos[1];
        {
            VkAccelerationStructureBuildRangeInfoKHR info = {};
            info.firstVertex                              = 0;
            info.primitiveOffset                          = 0;
            info.primitiveCount                           = 1;
            info.transformOffset                          = 0;

            rangeInfos[0]                                 = info;
        }

        const VkAccelerationStructureBuildRangeInfoKHR* pRangeInfos[] = {&rangeInfos[0]};

        p_inst->func_ptrs.vkCmdBuildAccelerationStructuresKHR(cmd_buff, 1, &build_info, pRangeInfos);
    }

    // ========================================================================================
    // ========================================================================================
    // ========================================================================================
    // ========================================================================================
    p_blas->deviceAddress = vk_buffer_device_address_z(device, p_blas->buffer);
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_blas_cleanup_z: Clean up BLAS resources. Destroys acceleration structure, buffers, and frees memory.
// =========================================================================================================================================
// =========================================================================================================================================
void vk_blas_cleanup_z(vk_blas_zt* p_blas)
{
    fatal_check_z(p_blas, "p_blas is null");

    vk_hidden_device_local_buff_cleanup_z(p_blas->device, &p_blas->scratch_buff);

    if (p_blas->handle != VK_NULL_HANDLE)
    {
        p_blas->p_inst->func_ptrs.vkDestroyAccelerationStructureKHR(p_blas->device, p_blas->handle, nullptr);
        p_blas->handle = VK_NULL_HANDLE;
    }

    if (p_blas->memory != VK_NULL_HANDLE)
    {
        vkFreeMemory(p_blas->device, p_blas->memory, nullptr);
        p_blas->memory = VK_NULL_HANDLE;
    }

    if (p_blas->buffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(p_blas->device, p_blas->buffer, nullptr);
        p_blas->buffer = VK_NULL_HANDLE;
    }
}
