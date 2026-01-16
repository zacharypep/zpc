#include "zpc/vk.h"

#include "zpc/fatal.h"

#include <stdlib.h>
#include <string.h>

// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
#define IMPL p_inst->rt_sys.p_i

// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
typedef struct
{
    uint64_t rgen_group;
    uint64_t miss_group;
    uint64_t hit_groups[VK_RT_MAX_HIT_GROUPS];
    uint32_t num_hit_groups;
} vk_rt_pipeline_key_zt;

// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
typedef struct
{
    vk_rt_pipeline_key_zt key;
    VkPipeline pipeline;
    uint8_t* group_handles;
    uint32_t group_handles_size;
} vk_rt_pipeline_entry_zt;

// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
struct vk_rt_system_internal_zt
{
    PFN_vkCmdBindDescriptorBuffersEXT vkCmdBindDescriptorBuffersEXT;
    PFN_vkCmdSetDescriptorBufferOffsetsEXT vkCmdSetDescriptorBufferOffsetsEXT;

    uint32_t handle_size;
    uint32_t handle_size_aligned;
    uint32_t group_base_alignment;

    vk_device_buff_zt sbt_buff;

    vk_rt_pipeline_entry_zt* pipelines;
    uint32_t pipelines_count;
    uint32_t pipelines_capacity;
};

// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
static bool pipeline_key_equal(const vk_rt_pipeline_key_zt* a, const vk_rt_pipeline_key_zt* b)
{
    if (a->rgen_group != b->rgen_group) return false;
    if (a->miss_group != b->miss_group) return false;
    if (a->num_hit_groups != b->num_hit_groups) return false;
    for (uint32_t i = 0; i < a->num_hit_groups; i++)
    {
        if (a->hit_groups[i] != b->hit_groups[i]) return false;
    }
    return true;
}

// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
static vk_rt_pipeline_entry_zt* find_pipeline(vk_rt_system_internal_zt* impl, const vk_rt_pipeline_key_zt* key)
{
    for (uint32_t i = 0; i < impl->pipelines_count; i++)
    {
        if (pipeline_key_equal(&impl->pipelines[i].key, key))
        {
            return &impl->pipelines[i];
        }
    }
    return nullptr;
}

// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
static void add_pipeline(vk_rt_system_internal_zt* impl, const vk_rt_pipeline_key_zt* key, VkPipeline pipeline, uint8_t* group_handles, uint32_t group_handles_size)
{
    if (impl->pipelines_count >= impl->pipelines_capacity)
    {
        uint32_t new_capacity                  = impl->pipelines_capacity == 0 ? 16 : impl->pipelines_capacity * 2;
        vk_rt_pipeline_entry_zt* new_pipelines = (vk_rt_pipeline_entry_zt*)realloc(impl->pipelines, new_capacity * sizeof(vk_rt_pipeline_entry_zt));
        if (new_pipelines == nullptr) fatal_z("failed to realloc pipelines");
        impl->pipelines          = new_pipelines;
        impl->pipelines_capacity = new_capacity;
    }
    impl->pipelines[impl->pipelines_count].key                = *key;
    impl->pipelines[impl->pipelines_count].pipeline           = pipeline;
    impl->pipelines[impl->pipelines_count].group_handles      = group_handles;
    impl->pipelines[impl->pipelines_count].group_handles_size = group_handles_size;
    impl->pipelines_count++;
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_rt_pass_init_z: Initialize the RT pass system. Must be called after vk_descriptors_init_z.
// =========================================================================================================================================
// =========================================================================================================================================
void vk_rt_pass_init_z(vk_instance_zt* p_inst)
{
    fatal_check_z(p_inst, "p_inst is null");

    // =============================================================================================
    // =============================================================================================
    // allocate internal structure
    // =============================================================================================
    // =============================================================================================
    {
        IMPL                     = (vk_rt_system_internal_zt*)fatal_alloc_z(sizeof(vk_rt_system_internal_zt), "failed to allocate rt system internals");
        IMPL->pipelines          = nullptr;
        IMPL->pipelines_count    = 0;
        IMPL->pipelines_capacity = 0;
    }

    // =============================================================================================
    // =============================================================================================
    // get function pointers
    // =============================================================================================
    // =============================================================================================
    {
        IMPL->vkCmdBindDescriptorBuffersEXT      = (PFN_vkCmdBindDescriptorBuffersEXT)vkGetDeviceProcAddr(p_inst->vk_dev, "vkCmdBindDescriptorBuffersEXT");
        IMPL->vkCmdSetDescriptorBufferOffsetsEXT = (PFN_vkCmdSetDescriptorBufferOffsetsEXT)vkGetDeviceProcAddr(p_inst->vk_dev, "vkCmdSetDescriptorBufferOffsetsEXT");
    }

    // =============================================================================================
    // =============================================================================================
    // get RT pipeline properties
    // =============================================================================================
    // =============================================================================================
    {
        VkPhysicalDeviceRayTracingPipelinePropertiesKHR rt_props = {0};
        rt_props.sType                                           = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;

        VkPhysicalDeviceProperties2 props                        = {0};
        props.sType                                              = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
        props.pNext                                              = &rt_props;

        vkGetPhysicalDeviceProperties2(p_inst->vk_phys_dev, &props);

        IMPL->handle_size          = rt_props.shaderGroupHandleSize;
        IMPL->handle_size_aligned  = vk_aligned_size_u32_z(rt_props.shaderGroupHandleSize, rt_props.shaderGroupHandleAlignment);
        IMPL->group_base_alignment = rt_props.shaderGroupBaseAlignment;
    }

    // =============================================================================================
    // =============================================================================================
    // create SBT buffer
    // =============================================================================================
    // =============================================================================================
    {
        uint32_t rgen_section_size = vk_aligned_size_u32_z(IMPL->handle_size_aligned * VK_RT_MAX_RGEN_GROUPS, IMPL->group_base_alignment);
        uint32_t miss_section_size = vk_aligned_size_u32_z(IMPL->handle_size_aligned * VK_RT_MAX_MISS_GROUPS, IMPL->group_base_alignment);
        uint32_t hits_section_size = vk_aligned_size_u32_z(IMPL->handle_size_aligned * VK_RT_MAX_HIT_GROUPS, IMPL->group_base_alignment);

        vk_device_buff_init_z(p_inst->vk_dev, p_inst->vk_phys_dev, &IMPL->sbt_buff, sizeof(uint8_t), rgen_section_size + miss_section_size + hits_section_size, VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
    }
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_rt_pass_cleanup_z: Clean up RT pass system resources.
// =========================================================================================================================================
// =========================================================================================================================================
void vk_rt_pass_cleanup_z(vk_instance_zt* p_inst)
{
    fatal_check_z(p_inst, "p_inst is null");

    if (IMPL != nullptr)
    {
        vk_device_buff_cleanup_z(p_inst->vk_dev, &IMPL->sbt_buff);

        if (IMPL->pipelines != nullptr)
        {
            for (uint32_t i = 0; i < IMPL->pipelines_count; i++)
            {
                vkDestroyPipeline(p_inst->vk_dev, IMPL->pipelines[i].pipeline, nullptr);
                free(IMPL->pipelines[i].group_handles);
            }
            free(IMPL->pipelines);
        }
        free(IMPL);
        IMPL = nullptr;
    }
}

// =========================================================================================================================================
// =========================================================================================================================================
// vk_rt_pass_record_cmd_buff_z: Record a ray tracing dispatch into the command buffer. Updates pipeline and SBT as needed.
// =========================================================================================================================================
// =========================================================================================================================================
void vk_rt_pass_record_cmd_buff_z(vk_instance_zt* p_inst, VkCommandBuffer cmd_buff, vk_rt_trace_req_zt* p_trace_req)
{
    fatal_check_z(p_inst, "p_inst is null");
    fatal_check_z(cmd_buff, "cmd_buff is null");
    fatal_check_z(p_trace_req, "p_trace_req is null");

    // =============================================================================================
    // =============================================================================================
    // build pipeline key
    // =============================================================================================
    // =============================================================================================
    vk_rt_pipeline_key_zt key = {0};
    {
        key.rgen_group     = p_trace_req->rgen_group;
        key.miss_group     = p_trace_req->miss_group;
        key.num_hit_groups = p_trace_req->num_hit_groups;
        for (uint32_t i = 0; i < p_trace_req->num_hit_groups && i < VK_RT_MAX_HIT_GROUPS; i++)
        {
            key.hit_groups[i] = p_trace_req->p_hit_groups[i];
        }
    }

    // =============================================================================================
    // =============================================================================================
    // create pipeline if not cached
    // =============================================================================================
    // =============================================================================================
    vk_rt_pipeline_entry_zt* p_entry = find_pipeline(IMPL, &key);
    {
        if (p_entry == nullptr)
        {
            // ======================================================================================
            // ======================================================================================
            // collect shader modules and build stage/group infos
            // ======================================================================================
            // ======================================================================================
            VkPipelineShaderStageCreateInfo* stage_infos      = nullptr;
            VkRayTracingShaderGroupCreateInfoKHR* group_infos = nullptr;
            VkShaderModule* modules                           = nullptr;
            uint32_t stage_count                              = 0;
            uint32_t group_count                              = 0;
            uint32_t module_count                             = 0;
            {
                uint32_t max_stages  = 1 + 1 + (p_trace_req->num_hit_groups * 3);
                uint32_t max_groups  = 1 + 1 + p_trace_req->num_hit_groups;
                uint32_t max_modules = max_stages;

                stage_infos          = (VkPipelineShaderStageCreateInfo*)fatal_alloc_z(max_stages * sizeof(VkPipelineShaderStageCreateInfo), "failed to allocate stage infos");
                group_infos          = (VkRayTracingShaderGroupCreateInfoKHR*)fatal_alloc_z(max_groups * sizeof(VkRayTracingShaderGroupCreateInfoKHR), "failed to allocate group infos");
                modules              = (VkShaderModule*)fatal_alloc_z(max_modules * sizeof(VkShaderModule), "failed to allocate modules");

                // rgen
                {
                    vk_shader_group_zt* sg = vk_find_shader_group_z(p_inst, p_trace_req->rgen_group);
                    if (sg == nullptr) fatal_z("rgen shader group not found");

                    VkShaderModule sm;
                    vk_shader_module_create_from_shader_z(p_inst->vk_dev, &sm, &sg->rgen);
                    modules[module_count++]                    = sm;

                    VkPipelineShaderStageCreateInfo stage_info = {0};
                    stage_info.sType                           = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                    stage_info.stage                           = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
                    stage_info.module                          = sm;
                    stage_info.pName                           = "main";
                    stage_infos[stage_count++]                 = stage_info;

                    VkRayTracingShaderGroupCreateInfoKHR group = {0};
                    group.sType                                = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
                    group.type                                 = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
                    group.generalShader                        = stage_count - 1;
                    group.closestHitShader                     = VK_SHADER_UNUSED_KHR;
                    group.anyHitShader                         = VK_SHADER_UNUSED_KHR;
                    group.intersectionShader                   = VK_SHADER_UNUSED_KHR;
                    group_infos[group_count++]                 = group;
                }

                // miss
                {
                    vk_shader_group_zt* sg = vk_find_shader_group_z(p_inst, p_trace_req->miss_group);
                    if (sg == nullptr) fatal_z("miss shader group not found");

                    VkShaderModule sm;
                    vk_shader_module_create_from_shader_z(p_inst->vk_dev, &sm, &sg->miss);
                    modules[module_count++]                    = sm;

                    VkPipelineShaderStageCreateInfo stage_info = {0};
                    stage_info.sType                           = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                    stage_info.stage                           = VK_SHADER_STAGE_MISS_BIT_KHR;
                    stage_info.module                          = sm;
                    stage_info.pName                           = "main";
                    stage_infos[stage_count++]                 = stage_info;

                    VkRayTracingShaderGroupCreateInfoKHR group = {0};
                    group.sType                                = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
                    group.type                                 = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
                    group.generalShader                        = stage_count - 1;
                    group.closestHitShader                     = VK_SHADER_UNUSED_KHR;
                    group.anyHitShader                         = VK_SHADER_UNUSED_KHR;
                    group.intersectionShader                   = VK_SHADER_UNUSED_KHR;
                    group_infos[group_count++]                 = group;
                }

                // hit groups
                for (uint32_t i = 0; i < p_trace_req->num_hit_groups; i++)
                {
                    vk_shader_group_zt* sg = vk_find_shader_group_z(p_inst, p_trace_req->p_hit_groups[i]);
                    if (sg == nullptr) fatal_z("hit shader group not found");

                    bool has_chit     = sg->chit.p_data != nullptr;
                    bool has_ahit     = sg->ahit.p_data != nullptr;
                    bool has_intr     = sg->intr.p_data != nullptr;

                    uint32_t chit_idx = VK_SHADER_UNUSED_KHR;
                    uint32_t ahit_idx = VK_SHADER_UNUSED_KHR;
                    uint32_t intr_idx = VK_SHADER_UNUSED_KHR;

                    if (has_chit)
                    {
                        VkShaderModule sm;
                        vk_shader_module_create_from_shader_z(p_inst->vk_dev, &sm, &sg->chit);
                        modules[module_count++]                    = sm;

                        VkPipelineShaderStageCreateInfo stage_info = {0};
                        stage_info.sType                           = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                        stage_info.stage                           = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
                        stage_info.module                          = sm;
                        stage_info.pName                           = "main";
                        chit_idx                                   = stage_count;
                        stage_infos[stage_count++]                 = stage_info;
                    }

                    if (has_ahit)
                    {
                        VkShaderModule sm;
                        vk_shader_module_create_from_shader_z(p_inst->vk_dev, &sm, &sg->ahit);
                        modules[module_count++]                    = sm;

                        VkPipelineShaderStageCreateInfo stage_info = {0};
                        stage_info.sType                           = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                        stage_info.stage                           = VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
                        stage_info.module                          = sm;
                        stage_info.pName                           = "main";
                        ahit_idx                                   = stage_count;
                        stage_infos[stage_count++]                 = stage_info;
                    }

                    if (has_intr)
                    {
                        VkShaderModule sm;
                        vk_shader_module_create_from_shader_z(p_inst->vk_dev, &sm, &sg->intr);
                        modules[module_count++]                    = sm;

                        VkPipelineShaderStageCreateInfo stage_info = {0};
                        stage_info.sType                           = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                        stage_info.stage                           = VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
                        stage_info.module                          = sm;
                        stage_info.pName                           = "main";
                        intr_idx                                   = stage_count;
                        stage_infos[stage_count++]                 = stage_info;
                    }

                    VkRayTracingShaderGroupTypeKHR type        = has_intr ? VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR : VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;

                    VkRayTracingShaderGroupCreateInfoKHR group = {0};
                    group.sType                                = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
                    group.type                                 = type;
                    group.generalShader                        = VK_SHADER_UNUSED_KHR;
                    group.closestHitShader                     = chit_idx;
                    group.anyHitShader                         = ahit_idx;
                    group.intersectionShader                   = intr_idx;
                    group_infos[group_count++]                 = group;
                }
            }

            // ======================================================================================
            // ======================================================================================
            // create pipeline
            // ======================================================================================
            // ======================================================================================
            VkPipeline pipeline;
            {
                VkRayTracingPipelineCreateInfoKHR info = {0};
                info.sType                             = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
                info.stageCount                        = stage_count;
                info.pStages                           = stage_infos;
                info.groupCount                        = group_count;
                info.pGroups                           = group_infos;
                info.maxPipelineRayRecursionDepth      = 1;
                info.layout                            = p_inst->desc_sys.vk_pipeline_layout;
                info.flags                             = VK_PIPELINE_CREATE_DESCRIPTOR_BUFFER_BIT_EXT;

                VK_CHECK(p_inst->func_ptrs.vkCreateRayTracingPipelinesKHR(p_inst->vk_dev, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &info, nullptr, &pipeline), "vkCreateRayTracingPipelinesKHR");
            }

            // ======================================================================================
            // ======================================================================================
            // get shader group handles
            // ======================================================================================
            // ======================================================================================
            uint8_t* group_handles      = nullptr;
            uint32_t group_handles_size = 0;
            {
                group_handles_size = group_count * IMPL->handle_size;
                group_handles      = (uint8_t*)fatal_alloc_z(group_handles_size, "failed to allocate group handles");

                VK_CHECK(p_inst->func_ptrs.vkGetRayTracingShaderGroupHandlesKHR(p_inst->vk_dev, pipeline, 0, group_count, group_handles_size, group_handles), "vkGetRayTracingShaderGroupHandlesKHR");
            }

            // ======================================================================================
            // ======================================================================================
            // cleanup shader modules
            // ======================================================================================
            // ======================================================================================
            {
                for (uint32_t i = 0; i < module_count; i++)
                {
                    vkDestroyShaderModule(p_inst->vk_dev, modules[i], nullptr);
                }
                free(modules);
                free(stage_infos);
                free(group_infos);
            }

            // ======================================================================================
            // ======================================================================================
            // add to cache
            // ======================================================================================
            // ======================================================================================
            {
                add_pipeline(IMPL, &key, pipeline, group_handles, group_handles_size);
                p_entry = find_pipeline(IMPL, &key);
            }
        }
    }

    // =============================================================================================
    // =============================================================================================
    // update SBT buffer
    // =============================================================================================
    // =============================================================================================
    uint32_t rgen_section_size;
    uint32_t miss_section_size;
    uint32_t hits_section_size;
    {
        rgen_section_size   = vk_aligned_size_u32_z(IMPL->handle_size_aligned * 1, IMPL->group_base_alignment);
        miss_section_size   = vk_aligned_size_u32_z(IMPL->handle_size_aligned * 1, IMPL->group_base_alignment);
        hits_section_size   = vk_aligned_size_u32_z(IMPL->handle_size_aligned * p_trace_req->num_hit_groups, IMPL->group_base_alignment);

        uint8_t* p_sbt      = (uint8_t*)IMPL->sbt_buff.p_mapped;
        uint8_t* p_sbt_rgen = p_sbt;
        uint8_t* p_sbt_miss = p_sbt_rgen + rgen_section_size;
        uint8_t* p_sbt_hits = p_sbt_miss + miss_section_size;

        uint8_t* handles    = p_entry->group_handles;

        memcpy(p_sbt_rgen, handles, IMPL->handle_size);
        memcpy(p_sbt_miss, handles + IMPL->handle_size, IMPL->handle_size);

        for (uint32_t i = 0; i < p_trace_req->num_hit_groups; i++)
        {
            memcpy(p_sbt_hits + (i * IMPL->handle_size_aligned), handles + ((2 + i) * IMPL->handle_size), IMPL->handle_size);
        }
    }

    // =============================================================================================
    // =============================================================================================
    // bind pipeline and descriptors
    // =============================================================================================
    // =============================================================================================
    {
        vkCmdBindPipeline(cmd_buff, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, p_entry->pipeline);

        VkDescriptorBufferBindingInfoEXT bindInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT};
        bindInfo.address                          = p_inst->desc_sys.desc_buff_device_addr;
        bindInfo.usage                            = VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT;
        IMPL->vkCmdBindDescriptorBuffersEXT(cmd_buff, 1, &bindInfo);

        uint32_t bufIndex      = 0;
        VkDeviceSize setOffset = 0;
        IMPL->vkCmdSetDescriptorBufferOffsetsEXT(cmd_buff, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, p_inst->desc_sys.vk_pipeline_layout, 0, 1, &bufIndex, &setOffset);
    }

    // =============================================================================================
    // =============================================================================================
    // push constants and trace rays
    // =============================================================================================
    // =============================================================================================
    {
        vkCmdPushConstants(
            cmd_buff,
            p_inst->desc_sys.vk_pipeline_layout,
            VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR | VK_SHADER_STAGE_INTERSECTION_BIT_KHR,
            0,
            sizeof(VkDeviceAddress),
            &p_trace_req->p_per_trace
        );

        VkDeviceAddress sbt_addr                       = IMPL->sbt_buff.deviceAddress;

        VkStridedDeviceAddressRegionKHR rgen_entry     = {0};
        rgen_entry.deviceAddress                       = sbt_addr;
        rgen_entry.stride                              = rgen_section_size;
        rgen_entry.size                                = rgen_section_size;

        VkStridedDeviceAddressRegionKHR miss_entry     = {0};
        miss_entry.deviceAddress                       = sbt_addr + rgen_section_size;
        miss_entry.stride                              = IMPL->handle_size_aligned;
        miss_entry.size                                = miss_section_size;

        VkStridedDeviceAddressRegionKHR hit_entry      = {0};
        hit_entry.deviceAddress                        = sbt_addr + rgen_section_size + miss_section_size;
        hit_entry.stride                               = IMPL->handle_size_aligned;
        hit_entry.size                                 = hits_section_size;

        VkStridedDeviceAddressRegionKHR callable_entry = {0};

        p_inst->func_ptrs.vkCmdTraceRaysKHR(cmd_buff, &rgen_entry, &miss_entry, &hit_entry, &callable_entry, p_trace_req->width, p_trace_req->height, 1);
    }
}

// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
#undef IMPL
