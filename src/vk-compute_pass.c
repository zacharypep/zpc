#include "zpc/vk.h"

#include "zpc/fatal.h"

#include <stdlib.h>
#include <string.h>

// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
#define IMPL p_inst->compute_sys.p_i

// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
typedef struct
{
    uint64_t shader_group;
    VkPipeline pipeline;
} vk_compute_pipeline_entry_zt;

// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
struct vk_compute_system_internal_zt
{
    // todo
    // move these to shared part of zgpu inst
    PFN_vkCmdBindDescriptorBuffersEXT vkCmdBindDescriptorBuffersEXT;
    PFN_vkCmdSetDescriptorBufferOffsetsEXT vkCmdSetDescriptorBufferOffsetsEXT;

    vk_compute_pipeline_entry_zt* pipelines;
    uint32_t pipelines_count;
    uint32_t pipelines_capacity;
};

// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
static VkPipeline find_pipeline(vk_compute_system_internal_zt* impl, uint64_t shader_group)
{
    for (uint32_t i = 0; i < impl->pipelines_count; i++)
    {
        if (impl->pipelines[i].shader_group == shader_group)
        {
            return impl->pipelines[i].pipeline;
        }
    }
    return VK_NULL_HANDLE;
}

// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
static bool pipeline_exists(vk_compute_system_internal_zt* impl, uint64_t shader_group)
{
    return find_pipeline(impl, shader_group) != VK_NULL_HANDLE;
}

// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
static void add_pipeline(vk_compute_system_internal_zt* impl, uint64_t shader_group, VkPipeline pipeline)
{
    if (impl->pipelines_count >= impl->pipelines_capacity)
    {
        uint32_t new_capacity                       = impl->pipelines_capacity == 0 ? 16 : impl->pipelines_capacity * 2;
        vk_compute_pipeline_entry_zt* new_pipelines = (vk_compute_pipeline_entry_zt*)realloc(impl->pipelines, new_capacity * sizeof(vk_compute_pipeline_entry_zt));
        if (new_pipelines == nullptr) fatal_z("failed to realloc pipelines");
        impl->pipelines          = new_pipelines;
        impl->pipelines_capacity = new_capacity;
    }
    impl->pipelines[impl->pipelines_count].shader_group = shader_group;
    impl->pipelines[impl->pipelines_count].pipeline     = pipeline;
    impl->pipelines_count++;
}

// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
void vk_compute_pass_init_z(vk_instance_zt* p_inst)
{
    fatal_check_z(p_inst, "p_inst is null");

    // =============================================================================================
    // =============================================================================================
    // =============================================================================================
    // =============================================================================================
    {
        IMPL                     = (vk_compute_system_internal_zt*)fatal_alloc_z(sizeof(vk_compute_system_internal_zt), "failed to allocate compute system internals");
        IMPL->pipelines          = nullptr;
        IMPL->pipelines_count    = 0;
        IMPL->pipelines_capacity = 0;
    }

    // =============================================================================================
    // =============================================================================================
    // =============================================================================================
    // =============================================================================================
    {
        IMPL->vkCmdBindDescriptorBuffersEXT      = (PFN_vkCmdBindDescriptorBuffersEXT)vkGetDeviceProcAddr(p_inst->vk_dev, "vkCmdBindDescriptorBuffersEXT");
        IMPL->vkCmdSetDescriptorBufferOffsetsEXT = (PFN_vkCmdSetDescriptorBufferOffsetsEXT)vkGetDeviceProcAddr(p_inst->vk_dev, "vkCmdSetDescriptorBufferOffsetsEXT");
    }
}

// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
void vk_compute_pass_record_cmd_buff_z(vk_instance_zt* p_inst, VkCommandBuffer cmd_buff, vk_compute_dispatch_req_zt* p_dispatch_reqs, uint32_t num_dispatch_reqs)
{
    fatal_check_z(p_inst, "p_inst is null");
    fatal_check_z(cmd_buff, "cmd_buff is null");
    fatal_check_z(p_dispatch_reqs, "p_dispatch_reqs is null");

    // =============================================================================================
    // =============================================================================================
    // =============================================================================================
    // =============================================================================================
    {
        for (uint32_t i = 0; i < num_dispatch_reqs; i++)
        {
            const vk_compute_dispatch_req_zt* dispatch_req = &p_dispatch_reqs[i];

            if (!pipeline_exists(IMPL, dispatch_req->shader_group))
            {
                VkShaderModule sh_mod;
                vk_shader_group_zt* sg = vk_find_shader_group_z(p_inst, dispatch_req->shader_group);
                if (sg == nullptr) fatal_z("shader group not found");
                vk_shader_module_create_from_shader_z(p_inst->vk_dev, &sh_mod, &sg->comp);

                VkPipelineShaderStageCreateInfo shader_info = {0};
                shader_info.sType                           = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                shader_info.stage                           = VK_SHADER_STAGE_COMPUTE_BIT;
                shader_info.module                          = sh_mod;
                shader_info.pName                           = "main";

                VkComputePipelineCreateInfo pipeline_info   = {0};
                pipeline_info.sType                         = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
                pipeline_info.pNext                         = nullptr;
                pipeline_info.flags                         = 0;
                pipeline_info.stage                         = shader_info;
                pipeline_info.layout                        = p_inst->desc_sys.vk_pipeline_layout;
                pipeline_info.basePipelineHandle            = VK_NULL_HANDLE;
                pipeline_info.basePipelineIndex             = 0;
                pipeline_info.flags                         = VK_PIPELINE_CREATE_DESCRIPTOR_BUFFER_BIT_EXT;

                VkPipeline pipeline;
                VK_CHECK(vkCreateComputePipelines(p_inst->vk_dev, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pipeline), "vkCreateComputePipelines");
                add_pipeline(IMPL, dispatch_req->shader_group, pipeline);

                vkDestroyShaderModule(p_inst->vk_dev, sh_mod, nullptr);
            }
        }
    }

    // =============================================================================================
    // =============================================================================================
    // dispatch
    // =============================================================================================
    // =============================================================================================
    {
        for (uint32_t i = 0; i < num_dispatch_reqs; i++)
        {
            const vk_compute_dispatch_req_zt* dispatch_req = &p_dispatch_reqs[i];

            VkPipeline pipeline                            = find_pipeline(IMPL, dispatch_req->shader_group);
            if (pipeline == VK_NULL_HANDLE) fatal_z("pipeline not found");
            vkCmdBindPipeline(cmd_buff, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);

            // desc buffer
            {
                VkDescriptorBufferBindingInfoEXT bindInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT};
                bindInfo.address                          = p_inst->desc_sys.desc_buff_device_addr;
                bindInfo.usage                            = VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT;
                IMPL->vkCmdBindDescriptorBuffersEXT(cmd_buff, 1, &bindInfo);

                uint32_t bufIndex      = 0;
                VkDeviceSize setOffset = 0;
                IMPL->vkCmdSetDescriptorBufferOffsetsEXT(cmd_buff, VK_PIPELINE_BIND_POINT_COMPUTE, p_inst->desc_sys.vk_pipeline_layout, 0, 1, &bufIndex, &setOffset);
            }

            vkCmdPushConstants(cmd_buff, p_inst->desc_sys.vk_pipeline_layout, VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(VkDeviceAddress), &dispatch_req->p_per_dispatch);
            vkCmdDispatch(cmd_buff, dispatch_req->num_groups_x, dispatch_req->num_groups_y, dispatch_req->num_groups_z);
        }
    }
}

// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
void vk_compute_pass_cleanup_z(vk_instance_zt* p_inst)
{
    fatal_check_z(p_inst, "p_inst is null");

    if (IMPL != nullptr)
    {
        if (IMPL->pipelines != nullptr)
        {
            free(IMPL->pipelines);
        }
        free(IMPL);
        IMPL = nullptr;
    }
}

// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
#undef IMPL
