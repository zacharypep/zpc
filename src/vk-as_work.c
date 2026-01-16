#include "zpc/vk.h"

#include "zpc/fatal.h"

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_beta.h>

// =========================================================================================================================================
// =========================================================================================================================================
// vk_as_work_record_build_as_z: Record commands to rebuild acceleration structures (BLASes and TLAS) into the command buffer.
// =========================================================================================================================================
// =========================================================================================================================================
void vk_as_work_record_build_as_z(VkCommandBuffer cmd_buff, vk_instance_zt* p_inst, vk_device_buff_zt* p_idcs_buff, vk_tlas_zt* p_tlas, const vk_as_work_rebuild_info_zt* p_infos, uint32_t num_infos)
{
    fatal_check_z(cmd_buff, "cmd_buff is null");
    fatal_check_z(p_inst, "p_inst is null");
    fatal_check_z(p_idcs_buff, "p_idcs_buff is null");
    fatal_check_z(p_tlas, "p_tlas is null");
    fatal_check_z(p_infos, "p_infos is null");

    // ============================================================================================
    // ============================================================================================
    // rebuild requested blases
    // ============================================================================================
    // ============================================================================================
    for (uint32_t i = 0; i < num_infos; ++i)
    {
        const vk_as_work_rebuild_info_zt* info = &p_infos[i];
        fatal_check_z(info->p_blas, "p_blas is null");
        fatal_check_z(info->p_verts_regions, "p_verts_regions is null");
        fatal_check_z(info->p_idcs_regions, "p_idcs_regions is null");

        vk_blas_record_build_tri_blas_z(p_inst, p_inst->vk_dev, p_inst->vk_phys_dev, cmd_buff, info->p_blas, info->verts_buff_addr, p_idcs_buff->deviceAddress, (uint32_t*)p_idcs_buff->p_mapped, info->p_verts_regions, info->verts_regions_count, info->p_idcs_regions, info->idcs_regions_count);
        vk_record_buff_barrier_z(p_inst, p_inst->vk_dev, cmd_buff, info->p_blas->buffer, 0, VK_WHOLE_SIZE);
        vk_record_buff_barrier_z(p_inst, p_inst->vk_dev, cmd_buff, info->p_blas->scratch_buff.handle, 0, VK_WHOLE_SIZE);

        // todo
        // make util func
        {
            VkMemoryBarrier2 barrier   = {};
            barrier.sType              = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
            barrier.srcStageMask       = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            barrier.srcAccessMask      = VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR | VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR;
            barrier.dstStageMask       = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            barrier.dstAccessMask      = VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR | VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR;
            barrier.pNext              = NULL;

            VkDependencyInfo depInfo   = {};
            depInfo.sType              = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
            depInfo.memoryBarrierCount = 1;
            depInfo.pMemoryBarriers    = &barrier;
            depInfo.pNext              = NULL;

            p_inst->func_ptrs.vkCmdPipelineBarrier2(cmd_buff, &depInfo);
        }
    }

    // ============================================================================================
    // ============================================================================================
    // rebuild tlas
    // ============================================================================================
    // ============================================================================================
    vk_tlas_record_build_z(p_inst, cmd_buff, p_tlas);
}
