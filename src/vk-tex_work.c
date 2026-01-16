#include "zpc/vk.h"

#include "zpc/fatal.h"

#include <stdint.h>
#include <vulkan/vulkan.h>

// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
void vk_tex_sys_init_z(vk_instance_zt* p_inst)
{
    fatal_check_z(p_inst, "p_instance is null");

    // =============================================================================================
    // =============================================================================================
    // setup staging buff
    // =============================================================================================
    // =============================================================================================
    {
        vk_host_buff_init_z(p_inst->vk_dev, p_inst->vk_phys_dev, &p_inst->tex_sys.staging_buff, 1, VK_TEX_WORK_MAX_SIZE, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    }
}

// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
uint32_t vk_tex_sys_update_buffs_z(vk_instance_zt* p_inst, vk_tex_upload_req_zt* p_requests, uint32_t requests_count)
{
    fatal_check_z(p_inst, "p_inst is null");
    fatal_check_z(p_requests, "p_requests is null");

    vk_host_buff_reset_z(&p_inst->tex_sys.staging_buff);
    p_inst->tex_sys.staged_uploads_count = 0;

    // =============================================================================================
    // =============================================================================================
    // process requests from the queue
    // =============================================================================================
    // =============================================================================================
    uint32_t processed_count             = 0;
    {
        while (processed_count < requests_count)
        {
            vk_tex_upload_req_zt* p_request = &p_requests[processed_count];
            size_t request_size             = p_request->bytes_size;

            if (request_size > UINT32_MAX)
            {
                fatal_z("request size exceeds uint32_t maximum: (%zu > %u)", request_size, UINT32_MAX);
            }

            uint32_t request_size_u32 = (uint32_t)request_size;

            if (p_inst->tex_sys.staging_buff.max_count < p_inst->tex_sys.staging_buff.count + request_size_u32)
            {
                break;
            }

            if (p_inst->tex_sys.staged_uploads_count >= VK_TEX_WORK_MAX_STAGED_UPLOADS)
            {
                fatal_z("staged uploads count exceeds maximum: (%u >= %u)", p_inst->tex_sys.staged_uploads_count, VK_TEX_WORK_MAX_STAGED_UPLOADS);
            }

            vk_region_handle_zt region                    = vk_host_buff_push_z(&p_inst->tex_sys.staging_buff, p_request->p_bytes, request_size_u32);

            vk_tex_work_staged_upload_zt* p_staged_upload = &p_inst->tex_sys.staged_uploads[p_inst->tex_sys.staged_uploads_count];
            p_staged_upload->region                       = region;
            p_staged_upload->p_img                        = p_request->p_img;

            p_inst->tex_sys.staged_uploads_count++;
            processed_count++;
        }
    }

    return processed_count;
}

// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
void vk_tex_sys_record_cmd_buff_z(vk_instance_zt* p_inst, VkCommandBuffer cmd_buff)
{
    fatal_check_z(p_inst, "p_inst is null");
    fatal_check_z(cmd_buff, "cmd_buff is null");

    // =============================================================================================
    // =============================================================================================
    // record copy commands for each staged upload
    // =============================================================================================
    // =============================================================================================
    {
        for (uint32_t i = 0; i < p_inst->tex_sys.staged_uploads_count; i++)
        {
            vk_tex_work_staged_upload_zt* p_staged_upload = &p_inst->tex_sys.staged_uploads[i];

            vk_record_trans_image_layout_z(p_inst, cmd_buff, p_staged_upload->p_img->handle, p_staged_upload->p_img->format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

            VkDeviceSize offset = (VkDeviceSize)p_staged_upload->region.start_idx * (VkDeviceSize)p_inst->tex_sys.staging_buff.stride;
            vk_record_copy_buffer_to_image_z(
                cmd_buff,
                p_inst->tex_sys.staging_buff.handle,
                offset,
                p_staged_upload->p_img->handle,
                p_staged_upload->p_img->width,
                p_staged_upload->p_img->height,
                p_staged_upload->p_img->num_channels,
                p_staged_upload->p_img->pixel_size,
                p_staged_upload->p_img->mip_levels
            );

            vk_record_trans_image_layout_z(p_inst, cmd_buff, p_staged_upload->p_img->handle, p_staged_upload->p_img->format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }
    }
}
