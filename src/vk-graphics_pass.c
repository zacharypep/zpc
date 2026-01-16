#include "zpc/vk.h"

#include "zpc/fatal.h"
#include "zpc/math.h"

#include <stdlib.h>
#include <string.h>

// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
#define IMPL p_inst->graphics_sys.p_i

// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
typedef struct
{
    VkFormat colour_format;
    VkFormat depth_format;
    uint64_t shader_group;
    uint32_t attachment_count;
} vk_graphics_pipeline_key_zt;

// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
typedef struct
{
    vk_graphics_pipeline_key_zt key;
    VkPipeline pipeline;
} vk_graphics_pipeline_entry_zt;

// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
struct vk_graphics_system_internal_zt
{
    PFN_vkCmdSetPolygonModeEXT vkCmdSetPolygonModeEXT;
    PFN_vkCmdSetColorBlendEnableEXT vkCmdSetColorBlendEnableEXT;

    PFN_vkCmdBindDescriptorBuffersEXT vkCmdBindDescriptorBuffersEXT;
    PFN_vkCmdSetDescriptorBufferOffsetsEXT vkCmdSetDescriptorBufferOffsetsEXT;

    vk_graphics_pipeline_entry_zt* pipelines;
    uint32_t pipelines_count;
    uint32_t pipelines_capacity;
};

// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
static bool pipeline_key_equal(const vk_graphics_pipeline_key_zt* a, const vk_graphics_pipeline_key_zt* b)
{
    return a->colour_format == b->colour_format && a->depth_format == b->depth_format && a->shader_group == b->shader_group && a->attachment_count == b->attachment_count;
}

// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
static VkPipeline find_pipeline(vk_graphics_system_internal_zt* impl, const vk_graphics_pipeline_key_zt* key)
{
    for (uint32_t i = 0; i < impl->pipelines_count; i++)
    {
        if (pipeline_key_equal(&impl->pipelines[i].key, key))
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
static bool pipeline_exists(vk_graphics_system_internal_zt* impl, const vk_graphics_pipeline_key_zt* key)
{
    return find_pipeline(impl, key) != VK_NULL_HANDLE;
}

// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
static void add_pipeline(vk_graphics_system_internal_zt* impl, const vk_graphics_pipeline_key_zt* key, VkPipeline pipeline)
{
    if (impl->pipelines_count >= impl->pipelines_capacity)
    {
        uint32_t new_capacity                        = impl->pipelines_capacity == 0 ? 16 : impl->pipelines_capacity * 2;
        vk_graphics_pipeline_entry_zt* new_pipelines = (vk_graphics_pipeline_entry_zt*)realloc(impl->pipelines, new_capacity * sizeof(vk_graphics_pipeline_entry_zt));
        if (new_pipelines == nullptr) fatal_z("failed to realloc pipelines");
        impl->pipelines          = new_pipelines;
        impl->pipelines_capacity = new_capacity;
    }
    impl->pipelines[impl->pipelines_count].key      = *key;
    impl->pipelines[impl->pipelines_count].pipeline = pipeline;
    impl->pipelines_count++;
}

// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
void vk_graphics_pass_init_z(vk_instance_zt* p_inst)
{
    fatal_check_z(p_inst, "p_inst is null");

    // =============================================================================================
    // =============================================================================================
    // =============================================================================================
    // =============================================================================================
    {
        IMPL                     = (vk_graphics_system_internal_zt*)fatal_alloc_z(sizeof(vk_graphics_system_internal_zt), "failed to allocate graphics system internals");
        IMPL->pipelines          = nullptr;
        IMPL->pipelines_count    = 0;
        IMPL->pipelines_capacity = 0;
    }

    // =============================================================================================
    // =============================================================================================
    // =============================================================================================
    // =============================================================================================
    {
        IMPL->vkCmdSetPolygonModeEXT             = (PFN_vkCmdSetPolygonModeEXT)vkGetDeviceProcAddr(p_inst->vk_dev, "vkCmdSetPolygonModeEXT");
        IMPL->vkCmdSetColorBlendEnableEXT        = (PFN_vkCmdSetColorBlendEnableEXT)vkGetDeviceProcAddr(p_inst->vk_dev, "vkCmdSetColorBlendEnableEXT");
        IMPL->vkCmdBindDescriptorBuffersEXT      = (PFN_vkCmdBindDescriptorBuffersEXT)vkGetDeviceProcAddr(p_inst->vk_dev, "vkCmdBindDescriptorBuffersEXT");
        IMPL->vkCmdSetDescriptorBufferOffsetsEXT = (PFN_vkCmdSetDescriptorBufferOffsetsEXT)vkGetDeviceProcAddr(p_inst->vk_dev, "vkCmdSetDescriptorBufferOffsetsEXT");
    }
}

// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
void vk_graphics_pass_record_cmd_buff_z(vk_instance_zt* p_inst, VkCommandBuffer cmd_buff, vk_device_image_zt** p_colours, uint32_t num_colour_attachments, vk_device_image_zt* p_depth, bool should_clear, vk_graphics_draw_req_zt* p_draw_reqs, uint32_t num_draw_reqs)
{
    fatal_check_z(p_inst, "p_inst is null");
    fatal_check_z(cmd_buff, "cmd_buff is null");
    fatal_check_z(p_colours, "p_colours is null");
    fatal_check_z(p_depth, "p_depth is null");

    if (num_colour_attachments == 0)
    {
        return;
    }

    // =============================================================================================
    // =============================================================================================
    // =============================================================================================
    // =============================================================================================
    {
        for (uint32_t i = 0; i < num_draw_reqs; i++)
        {
            const vk_graphics_draw_req_zt* draw_req = &p_draw_reqs[i];

            vk_graphics_pipeline_key_zt key         = {
                        .colour_format    = p_colours[0]->format,
                        .depth_format     = p_depth->format,
                        .shader_group     = draw_req->shader_group,
                        .attachment_count = num_colour_attachments,
            };

            if (!pipeline_exists(IMPL, &key))
            {
                VkShaderModule v;
                VkShaderModule f;

                vk_shader_group_zt* sg = vk_find_shader_group_z(p_inst, draw_req->shader_group);
                if (sg == nullptr) fatal_z("shader group not found");
                vk_shader_module_create_from_shader_z(p_inst->vk_dev, &v, &sg->vert);
                vk_shader_module_create_from_shader_z(p_inst->vk_dev, &f, &sg->frag);

                VkPipelineShaderStageCreateInfo v_info               = {0};
                v_info.sType                                         = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                v_info.stage                                         = VK_SHADER_STAGE_VERTEX_BIT;
                v_info.module                                        = v;
                v_info.pName                                         = "main";

                VkPipelineShaderStageCreateInfo f_info               = {0};
                f_info.sType                                         = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                f_info.stage                                         = VK_SHADER_STAGE_FRAGMENT_BIT;
                f_info.module                                        = f;
                f_info.pName                                         = "main";

                VkPipelineShaderStageCreateInfo shader_stages[2]     = {v_info, f_info};

                VkPipelineVertexInputStateCreateInfo vert_state_info = {0};
                vert_state_info.sType                                = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
                vert_state_info.vertexBindingDescriptionCount        = 0;
                vert_state_info.vertexAttributeDescriptionCount      = 0;
                vert_state_info.pVertexBindingDescriptions           = nullptr;
                vert_state_info.pVertexAttributeDescriptions         = nullptr;

                VkPipelineInputAssemblyStateCreateInfo input_info    = {0};
                input_info.sType                                     = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
                input_info.topology                                  = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
                input_info.primitiveRestartEnable                    = VK_FALSE;

                VkPipelineViewportStateCreateInfo viewport_state     = {0};
                viewport_state.sType                                 = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
                viewport_state.viewportCount                         = 1;
                viewport_state.scissorCount                          = 1;

                VkPipelineRasterizationStateCreateInfo rasterizer    = {0};
                rasterizer.sType                                     = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
                rasterizer.depthClampEnable                          = VK_FALSE;
                rasterizer.rasterizerDiscardEnable                   = VK_FALSE;
                rasterizer.polygonMode                               = VK_POLYGON_MODE_FILL;
                rasterizer.lineWidth                                 = 1.0f;
                rasterizer.cullMode                                  = VK_CULL_MODE_NONE;
                rasterizer.frontFace                                 = VK_FRONT_FACE_COUNTER_CLOCKWISE;
                rasterizer.depthBiasEnable                           = VK_FALSE;

                VkPipelineMultisampleStateCreateInfo multisamp       = {0};
                multisamp.sType                                      = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
                multisamp.sampleShadingEnable                        = VK_FALSE;
                multisamp.rasterizationSamples                       = VK_SAMPLE_COUNT_1_BIT;

                VkPipelineColorBlendAttachmentState* attach_infos    = (VkPipelineColorBlendAttachmentState*)fatal_alloc_z(num_colour_attachments * sizeof(VkPipelineColorBlendAttachmentState), "failed to allocate attach infos");
                for (uint32_t j = 0; j < num_colour_attachments; j++)
                {
                    attach_infos[j].colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
                    attach_infos[j].blendEnable         = VK_TRUE;
                    attach_infos[j].srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
                    attach_infos[j].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
                    attach_infos[j].colorBlendOp        = VK_BLEND_OP_ADD;
                    attach_infos[j].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
                    attach_infos[j].dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
                    attach_infos[j].alphaBlendOp        = VK_BLEND_OP_ADD;
                }

                VkPipelineColorBlendStateCreateInfo colour_blend = {0};
                colour_blend.sType                               = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
                colour_blend.logicOpEnable                       = VK_FALSE;
                colour_blend.attachmentCount                     = num_colour_attachments;
                colour_blend.pAttachments                        = attach_infos;

                VkDynamicState dynamic_states[]                  = {
                    VK_DYNAMIC_STATE_VIEWPORT,
                    VK_DYNAMIC_STATE_SCISSOR,
                    VK_DYNAMIC_STATE_POLYGON_MODE_EXT,
                    VK_DYNAMIC_STATE_LINE_WIDTH,
                    VK_DYNAMIC_STATE_CULL_MODE,
                    VK_DYNAMIC_STATE_FRONT_FACE,
                    VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE,
                    VK_DYNAMIC_STATE_DEPTH_WRITE_ENABLE,
                    VK_DYNAMIC_STATE_DEPTH_COMPARE_OP,
                    VK_DYNAMIC_STATE_COLOR_BLEND_ENABLE_EXT,
                };
                VkPipelineDynamicStateCreateInfo dynamic_state = {0};
                dynamic_state.sType                            = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
                dynamic_state.dynamicStateCount                = sizeof(dynamic_states) / sizeof(dynamic_states[0]);
                dynamic_state.pDynamicStates                   = dynamic_states;

                VkFormat* colour_formats                       = (VkFormat*)fatal_alloc_z(num_colour_attachments * sizeof(VkFormat), "failed to allocate colour formats");
                for (uint32_t j = 0; j < num_colour_attachments; j++)
                {
                    colour_formats[j] = p_colours[j]->format;
                }

                VkPipelineRenderingCreateInfo rendering_info              = {0};
                rendering_info.sType                                      = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
                rendering_info.viewMask                                   = 0;
                rendering_info.colorAttachmentCount                       = num_colour_attachments;
                rendering_info.pColorAttachmentFormats                    = colour_formats;
                rendering_info.depthAttachmentFormat                      = p_depth->format;
                rendering_info.stencilAttachmentFormat                    = VK_FORMAT_UNDEFINED;
                rendering_info.pNext                                      = nullptr;

                VkPipelineDepthStencilStateCreateInfo depth_stencil_state = {0};
                depth_stencil_state.sType                                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
                depth_stencil_state.depthTestEnable                       = VK_TRUE;
                depth_stencil_state.depthWriteEnable                      = VK_TRUE;
                depth_stencil_state.depthCompareOp                        = VK_COMPARE_OP_LESS;
                depth_stencil_state.depthBoundsTestEnable                 = VK_FALSE;
                depth_stencil_state.stencilTestEnable                     = VK_FALSE;

                VkGraphicsPipelineCreateInfo pipeline_info                = {0};
                pipeline_info.sType                                       = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
                pipeline_info.stageCount                                  = 2;
                pipeline_info.pStages                                     = shader_stages;
                pipeline_info.pVertexInputState                           = &vert_state_info;
                pipeline_info.pInputAssemblyState                         = &input_info;
                pipeline_info.pViewportState                              = &viewport_state;
                pipeline_info.pRasterizationState                         = &rasterizer;
                pipeline_info.pMultisampleState                           = &multisamp;
                pipeline_info.pColorBlendState                            = &colour_blend;
                pipeline_info.pDynamicState                               = &dynamic_state;
                pipeline_info.layout                                      = p_inst->desc_sys.vk_pipeline_layout;
                pipeline_info.renderPass                                  = nullptr;
                pipeline_info.pDepthStencilState                          = &depth_stencil_state;
                pipeline_info.subpass                                     = 0;
                pipeline_info.basePipelineHandle                          = VK_NULL_HANDLE;
                pipeline_info.pNext                                       = &rendering_info;
                pipeline_info.flags                                       = VK_PIPELINE_CREATE_DESCRIPTOR_BUFFER_BIT_EXT;

                VkPipeline pipeline;
                VK_CHECK(vkCreateGraphicsPipelines(p_inst->vk_dev, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pipeline), "vkCreateGraphicsPipelines");
                add_pipeline(IMPL, &key, pipeline);

                free(attach_infos);
                free(colour_formats);

                vkDestroyShaderModule(p_inst->vk_dev, v, nullptr);
                vkDestroyShaderModule(p_inst->vk_dev, f, nullptr);
            }
        }
    }

    // =============================================================================================
    // =============================================================================================
    // begin rendering
    // =============================================================================================
    // =============================================================================================
    {
        VkRenderingAttachmentInfo* colour_attachments = (VkRenderingAttachmentInfo*)fatal_alloc_z(num_colour_attachments * sizeof(VkRenderingAttachmentInfo), "failed to allocate colour attachments");
        for (uint32_t i = 0; i < num_colour_attachments; i++)
        {
            colour_attachments[i] = (VkRenderingAttachmentInfo){
                .sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                .pNext       = nullptr,
                .imageView   = p_colours[i]->view,
                .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .loadOp      = should_clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD,
                .storeOp     = VK_ATTACHMENT_STORE_OP_STORE,
                .clearValue  = {.color = {0.0f, 0.0f, 0.0f, 0.0f}},
            };
        }

        const VkRenderingAttachmentInfo depth = {
            .sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .pNext       = nullptr,
            .imageView   = p_depth->view,
            .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
            .loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp     = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .clearValue  = {.depthStencil = {1.0f, 0}},
        };

        const VkRenderingInfo info = {
            .sType                = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .pNext                = nullptr,
            .renderArea           = {.offset = {0, 0}, .extent = {p_colours[0]->width, p_colours[0]->height}},
            .layerCount           = 1,
            .colorAttachmentCount = num_colour_attachments,
            .pColorAttachments    = colour_attachments,
            .pDepthAttachment     = &depth,
            .pStencilAttachment   = nullptr,
        };
        vkCmdBeginRendering(cmd_buff, &info);

        free(colour_attachments);
    }

    // =============================================================================================
    // =============================================================================================
    // dyn state
    // =============================================================================================
    // =============================================================================================
    {
        const VkViewport viewport = {
            .x        = 0.0f,
            .y        = 0.0f,
            .width    = (float)p_colours[0]->width,
            .height   = (float)p_colours[0]->height,
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
        };
        vkCmdSetViewport(cmd_buff, 0, 1, &viewport);
    }

    // =============================================================================================
    // =============================================================================================
    // per draw req
    // =============================================================================================
    // =============================================================================================
    {
        for (uint32_t i = 0; i < num_draw_reqs; i++)
        {
            const vk_graphics_draw_req_zt* draw_req = &p_draw_reqs[i];

            const VkRect2D scissor                  = {
                                 .offset = { (int32_t)(draw_req->scissor_nrm.xy.x * p_colours[0]->width),  (int32_t)(draw_req->scissor_nrm.xy.y * p_colours[0]->height)},
                                 .extent = {(uint32_t)(draw_req->scissor_nrm.wh.x * p_colours[0]->width), (uint32_t)(draw_req->scissor_nrm.wh.y * p_colours[0]->height)},
            };
            vkCmdSetScissor(cmd_buff, 0, 1, &scissor);

            if (draw_req->is_point_draw)
            {
                IMPL->vkCmdSetPolygonModeEXT(cmd_buff, VK_POLYGON_MODE_POINT);
                vkCmdSetLineWidth(cmd_buff, 1.0f);
            }
            else if (draw_req->is_line_draw)
            {
                IMPL->vkCmdSetPolygonModeEXT(cmd_buff, VK_POLYGON_MODE_LINE);
                vkCmdSetLineWidth(cmd_buff, 5.0f);
            }
            else
            {
                IMPL->vkCmdSetPolygonModeEXT(cmd_buff, VK_POLYGON_MODE_FILL);
                vkCmdSetLineWidth(cmd_buff, 1.0f);
            }

            vkCmdSetCullMode(cmd_buff, VK_CULL_MODE_NONE);
            vkCmdSetFrontFace(cmd_buff, VK_FRONT_FACE_COUNTER_CLOCKWISE);
            vkCmdSetDepthTestEnable(cmd_buff, draw_req->should_depth_test ? VK_TRUE : VK_FALSE);
            vkCmdSetDepthWriteEnable(cmd_buff, draw_req->should_depth_write ? VK_TRUE : VK_FALSE);
            vkCmdSetDepthCompareOp(cmd_buff, VK_COMPARE_OP_LESS);

            VkBool32 enableBlend    = draw_req->is_alpha_blend ? VK_TRUE : VK_FALSE;
            VkBool32* blend_enables = (VkBool32*)fatal_alloc_z(num_colour_attachments * sizeof(VkBool32), "failed to allocate blend enables");
            for (uint32_t j = 0; j < num_colour_attachments; j++)
            {
                blend_enables[j] = enableBlend;
            }
            IMPL->vkCmdSetColorBlendEnableEXT(cmd_buff, 0, num_colour_attachments, blend_enables);
            free(blend_enables);

            vk_graphics_pipeline_key_zt key = {
                .colour_format    = p_colours[0]->format,
                .depth_format     = p_depth->format,
                .shader_group     = draw_req->shader_group,
                .attachment_count = num_colour_attachments,
            };
            VkPipeline pipeline = find_pipeline(IMPL, &key);
            if (pipeline == VK_NULL_HANDLE) fatal_z("pipeline not found");
            vkCmdBindPipeline(cmd_buff, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

            // desc buffer
            {
                VkDescriptorBufferBindingInfoEXT bindInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT};
                bindInfo.address                          = p_inst->desc_sys.desc_buff_device_addr;
                bindInfo.usage                            = VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT;
                IMPL->vkCmdBindDescriptorBuffersEXT(cmd_buff, 1, &bindInfo);

                uint32_t bufIndex      = 0;
                VkDeviceSize setOffset = 0;
                IMPL->vkCmdSetDescriptorBufferOffsetsEXT(cmd_buff, VK_PIPELINE_BIND_POINT_GRAPHICS, p_inst->desc_sys.vk_pipeline_layout, 0, 1, &bufIndex, &setOffset);
            }

            vkCmdPushConstants(cmd_buff, p_inst->desc_sys.vk_pipeline_layout, VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(VkDeviceAddress), &draw_req->p_per_draw);
            vkCmdDraw(cmd_buff, draw_req->idx_count, draw_req->inst_count, 0, 0);
        }
    }

    // =============================================================================================
    // =============================================================================================
    // finish
    // =============================================================================================
    // =============================================================================================
    {
        vkCmdEndRendering(cmd_buff);
    }
}

// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
void vk_graphics_pass_cleanup_z(vk_instance_zt* p_inst)
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
