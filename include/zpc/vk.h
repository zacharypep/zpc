#pragma once

#include <stddef.h>
#include <stdint.h>
#include <vulkan/vulkan.h>

#include "zpc/math.h"
#include "zpc/fatal.h"

#ifdef __cplusplus
#define EXTERN_C extern "C"
#else
#define EXTERN_C extern
#endif

#define VK_CHECK(vkFunc, title) \
    { \
        VkResult result = vkFunc; \
        if (result != VK_SUCCESS) fatal_z("failed: %s, error: %d", title, result); \
    }

static const char* const VK_VALIDATION_LAYERS[]    = {"VK_LAYER_KHRONOS_validation"};

static const char* const VK_RT_DEVICE_EXTENSIONS[] = {
    VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
    VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
    VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
    VK_KHR_RAY_TRACING_POSITION_FETCH_EXTENSION_NAME,
};

static const char* const VK_MEMORY_DEVICE_EXTENSIONS[] = {
    VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME,
    VK_EXT_PAGEABLE_DEVICE_LOCAL_MEMORY_EXTENSION_NAME,
};

typedef struct vk_compute_system_internal_zt vk_compute_system_internal_zt;
typedef struct vk_graphics_system_internal_zt vk_graphics_system_internal_zt;
typedef struct vk_rt_system_internal_zt vk_rt_system_internal_zt;
typedef struct vk_descriptors_internal_zt vk_descriptors_internal_zt;

constexpr uint32_t VK_INSTANCE_MAX_SHADER_GROUPS = 1024;

#define VK_CMD_BUFF_POOL_MAX_BUFFERS 20

#define VK_MAX_SURFACE_FORMATS       64
#define VK_MAX_PRESENT_MODES         16

constexpr uint32_t VK_RT_MAX_RGEN_GROUPS = 10;
constexpr uint32_t VK_RT_MAX_MISS_GROUPS = 10;
constexpr uint32_t VK_RT_MAX_HIT_GROUPS  = 1000;

#define VK_TEX_WORK_MAX_WIDTH          4096
#define VK_TEX_WORK_MAX_HEIGHT         4096
#define VK_TEX_WORK_MAX_SIZE           (VK_TEX_WORK_MAX_WIDTH * VK_TEX_WORK_MAX_HEIGHT * 4 * 4)
#define VK_TEX_WORK_MAX_STAGED_UPLOADS 64

typedef struct
{
    void* ptr;
    VkDeviceAddress device_addr;
    uint64_t start_idx;
    uint64_t count;
} vk_region_handle_zt;

typedef struct
{
    const void* p_data;
    size_t size;
} vk_shader_zt;

typedef struct
{
    uint64_t id;
    vk_shader_zt vert;
    vk_shader_zt frag;
    vk_shader_zt comp;
    vk_shader_zt rgen;
    vk_shader_zt miss;
    vk_shader_zt chit;
    vk_shader_zt ahit;
    vk_shader_zt intr;
} vk_shader_group_zt;

typedef struct
{
    VkDevice device;
    VkDeviceMemory memory;
    VkBuffer handle;
    VkDeviceAddress deviceAddress;
    void* p_mapped;
    size_t stride;
    uint64_t max_count;
    uint64_t count;
} vk_device_buff_zt;

typedef struct
{
    VkDevice device;
    VkDeviceMemory memory;
    VkBuffer handle;
    void* p_mapped;
    size_t stride;
    uint32_t max_count;
    uint32_t count;
} vk_host_buff_zt;

typedef struct
{
    VkDevice device;
    VkDeviceMemory memory;
    VkBuffer handle;
    VkDeviceAddress deviceAddress;
    void* p_mapped_device;
    void* staging_buff;
    size_t staging_buff_size;
    size_t stride;
    uint32_t max_count;
    uint32_t count;
} vk_staged_device_buff_zt;

typedef struct
{
    VkDevice device;
    VkDeviceMemory memory;
    VkBuffer handle;
    VkDeviceAddress device_addr;
} vk_hidden_device_local_buff_zt;

typedef struct
{
    VkDevice device;
    VkDeviceMemory memory;
    bool is_init;
    VkImage handle;
    VkImageView view;
    uint32_t width;
    uint32_t height;
    uint32_t num_channels;
    size_t pixel_size;
    uint32_t mip_levels;
    VkFormat format;
} vk_device_image_zt;

typedef struct
{
    VkDevice device;
    VkDeviceMemory memory;
    VkImage handle;
    VkImageView view;
    uint32_t width;
    uint32_t height;
    uint32_t num_channels;
    size_t pixel_size;
    uint32_t mip_levels;
    VkFormat format;
    void* p_mapped;
} vk_host_visible_image_zt;

typedef struct
{
    vk_device_image_zt* p_images;
    uint32_t max_count;
} vk_device_local_image_array_zt;

typedef struct
{
    PFN_vkQueueSubmit2 vkQueueSubmit2;
    PFN_vkCmdPipelineBarrier2 vkCmdPipelineBarrier2;
    PFN_vkCmdBeginRendering vkCmdBeginRendering;
    PFN_vkCmdEndRendering vkCmdEndRendering;
    PFN_vkCmdBlitImage2 vkCmdBlitImage2;

    PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR;
    PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR;
    PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructureKHR;
    PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR;
    PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR;
    PFN_vkCmdTraceRaysKHR vkCmdTraceRaysKHR;
    PFN_vkGetRayTracingShaderGroupHandlesKHR vkGetRayTracingShaderGroupHandlesKHR;
    PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelinesKHR;

    PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT;
} vk_instance_func_ptrs_zt;

typedef struct
{
    vk_compute_system_internal_zt* p_i;
} vk_compute_system_zt;

typedef struct
{
    vk_graphics_system_internal_zt* p_i;
} vk_graphics_system_zt;

typedef struct
{
    vk_rt_system_internal_zt* p_i;
} vk_rt_system_zt;

typedef struct
{
    VkPipelineLayout vk_pipeline_layout;
    VkBuffer desc_buff_handle;
    VkDeviceMemory desc_buff_memory;
    VkDeviceAddress desc_buff_device_addr;
    void* desc_buff_p_mapped;
    vk_descriptors_internal_zt* p_i;
} vk_descriptors_system_zt;

typedef struct
{
    vk_region_handle_zt region;
    vk_device_image_zt* p_img;
} vk_tex_work_staged_upload_zt;

typedef struct
{
    vk_tex_work_staged_upload_zt staged_uploads[VK_TEX_WORK_MAX_STAGED_UPLOADS];
    uint32_t staged_uploads_count;
    vk_host_buff_zt staging_buff;
} vk_tex_system_zt;

typedef struct
{
    vk_instance_func_ptrs_zt func_ptrs;

    bool using_vk_1_2;

    VkPhysicalDevice vk_phys_dev;
    VkDevice vk_dev;

    vk_shader_group_zt shader_groups[VK_INSTANCE_MAX_SHADER_GROUPS];
    uint32_t num_shader_groups;

    vk_descriptors_system_zt desc_sys;
    vk_compute_system_zt compute_sys;
    vk_graphics_system_zt graphics_sys;
    vk_rt_system_zt rt_sys;
    vk_tex_system_zt tex_sys;
} vk_instance_zt;

typedef struct
{
    vk_instance_zt* p_inst;
    VkCommandPool command_pool;
    VkCommandBuffer buffs[VK_CMD_BUFF_POOL_MAX_BUFFERS];
    uint32_t curr_idx;
} vk_cmd_buff_pool_zt;

typedef struct
{
    VkSemaphore semaphore;
    VkPipelineStageFlags2 stage;
} vk_semaphore_stage_pair_zt;

typedef struct
{
    VkSemaphore semaphore;
    VkPipelineStageFlags2 stage;
    uint64_t value;
} vk_semaphore_timeline_pair_zt;

typedef struct
{
    rect_zt scissor_nrm;
    uint64_t shader_group;
    bool is_point_draw;
    bool is_line_draw;
    bool is_alpha_blend;
    bool should_depth_test;
    bool should_depth_write;
    uint32_t idx_count;
    uint32_t inst_count;
    VkDeviceAddress p_per_draw;
} vk_graphics_draw_req_zt;

typedef struct
{
    uint64_t shader_group;
    VkDeviceAddress p_per_dispatch;
    uint32_t num_groups_x;
    uint32_t num_groups_y;
    uint32_t num_groups_z;
} vk_compute_dispatch_req_zt;

typedef struct
{
    uint64_t rgen_group;
    uint64_t miss_group;
    uint64_t* p_hit_groups;
    uint32_t num_hit_groups;
    VkDeviceAddress p_per_trace;
    uint32_t width;
    uint32_t height;
} vk_rt_trace_req_zt;

typedef struct
{
    VkSurfaceCapabilitiesKHR capabilities;
    VkSurfaceFormatKHR formats[VK_MAX_SURFACE_FORMATS];
    uint32_t format_count;
    VkPresentModeKHR present_modes[VK_MAX_PRESENT_MODES];
    uint32_t present_mode_count;
} vk_swapchain_support_details_zt;

typedef struct
{
    uint32_t graphics_family;
    uint32_t present_family;
    bool has_graphics_family;
    bool has_present_family;
} vk_queue_families_details_zt;

typedef struct
{
    VkSampler handle;
} vk_sampler_zt;

typedef struct
{
    vk_instance_zt* p_inst;
    uint32_t MAX_INSTS;
    VkAccelerationStructureKHR handle;
    VkDeviceMemory memory;
    VkBuffer buffer;
    vk_device_buff_zt insts_buff;
    vk_hidden_device_local_buff_zt scratch_buff;
} vk_tlas_zt;

typedef struct
{
    vk_instance_zt* p_inst;
    VkDevice device;
    VkAccelerationStructureKHR handle;
    VkDeviceMemory memory;
    VkBuffer buffer;
    uint64_t deviceAddress;
    vk_hidden_device_local_buff_zt scratch_buff;
    int num_tris_initialised;
} vk_blas_zt;

typedef struct
{
    vk_blas_zt* p_blas;
    VkDeviceAddress verts_buff_addr;
    const vk_region_handle_zt* p_verts_regions;
    uint32_t verts_regions_count;
    const vk_region_handle_zt* p_idcs_regions;
    uint32_t idcs_regions_count;
} vk_as_work_rebuild_info_zt;

typedef struct
{
    const uint8_t* p_bytes;
    size_t bytes_size;
    vk_device_image_zt* p_img;
} vk_tex_upload_req_zt;

EXTERN_C uint32_t vk_aligned_size_u32_z(uint32_t value, uint32_t alignment);
EXTERN_C size_t vk_aligned_size_sz_z(size_t value, size_t alignment);
EXTERN_C VkDeviceSize vk_aligned_size_vk_z(VkDeviceSize value, VkDeviceSize alignment);
EXTERN_C VkDeviceAddress vk_buffer_device_address_z(VkDevice device, VkBuffer buffer);
EXTERN_C uint32_t vk_find_memory_type_z(VkPhysicalDevice phys_dev, uint32_t filter, VkMemoryPropertyFlags prop_flags);
EXTERN_C VkTransformMatrixKHR vk_transform_matrix_from_mat4_z(const mat4_zt* p_mat);
EXTERN_C void vk_record_copy_buffer_to_image_z(VkCommandBuffer cmd_buff, VkBuffer buff, VkDeviceSize offset, VkImage dst, uint32_t width, uint32_t height, uint32_t num_channels, size_t pixel_size, uint32_t mip_levels);
EXTERN_C void vk_shader_module_create_z(VkDevice device, VkShaderModule* p_shader_module, const void* p_code, size_t code_size);
EXTERN_C void vk_shader_module_create_from_shader_z(VkDevice device, VkShaderModule* p_shader_module, const vk_shader_zt* p_shader);
EXTERN_C void vk_buffer_create_z(VkDevice device, VkPhysicalDevice phys_dev, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags props, VkBuffer* p_buffer, VkDeviceMemory* p_memory);
EXTERN_C void vk_descriptors_init_z(VkDevice device, VkPhysicalDevice phys_dev, vk_descriptors_system_zt* p_sys, size_t max_textures, size_t max_fonts);
EXTERN_C void vk_descriptors_cleanup_z(VkDevice device, vk_descriptors_system_zt* p_sys);
EXTERN_C void vk_descriptors_set_samp_z(VkDevice device, vk_descriptors_system_zt* p_sys, VkSampler sampler);
EXTERN_C void vk_descriptors_set_panel_z(VkDevice device, vk_descriptors_system_zt* p_sys, uint32_t idx, VkImageView image_view);
EXTERN_C void vk_descriptors_set_tex_z(VkDevice device, vk_descriptors_system_zt* p_sys, uint32_t idx, VkDescriptorImageInfo img_info);
EXTERN_C void vk_descriptors_set_font_z(VkDevice device, vk_descriptors_system_zt* p_sys, uint32_t idx, VkDescriptorImageInfo img_info);
EXTERN_C void vk_descriptors_set_gbuff_base_colour_z(VkDevice device, vk_descriptors_system_zt* p_sys, uint32_t idx, VkImageView image_view);
EXTERN_C void vk_descriptors_set_gbuff_roughness_z(VkDevice device, vk_descriptors_system_zt* p_sys, uint32_t idx, VkImageView image_view);
EXTERN_C void vk_descriptors_set_gbuff_specular_z(VkDevice device, vk_descriptors_system_zt* p_sys, uint32_t idx, VkImageView image_view);
EXTERN_C void vk_descriptors_set_gbuff_metallic_z(VkDevice device, vk_descriptors_system_zt* p_sys, uint32_t idx, VkImageView image_view);
EXTERN_C void vk_descriptors_set_gbuff_emission_z(VkDevice device, vk_descriptors_system_zt* p_sys, uint32_t idx, VkImageView image_view);
EXTERN_C void vk_descriptors_set_gbuff_pos_world_z(VkDevice device, vk_descriptors_system_zt* p_sys, uint32_t idx, VkImageView image_view);
EXTERN_C void vk_descriptors_set_gbuff_nrm_world_z(VkDevice device, vk_descriptors_system_zt* p_sys, uint32_t idx, VkImageView image_view);
EXTERN_C void vk_descriptors_set_gbuff_ao_z(VkDevice device, vk_descriptors_system_zt* p_sys, uint32_t idx, VkImageView image_view);
EXTERN_C void vk_descriptors_set_gbuff_ao_sampled_z(VkDevice device, vk_descriptors_system_zt* p_sys, uint32_t idx, VkImageView image_view);
EXTERN_C void vk_descriptors_set_tlas_z(VkDevice device, vk_descriptors_system_zt* p_sys, vk_tlas_zt* p_tlas);
EXTERN_C void vk_device_image_init_z(
    VkDevice device, VkPhysicalDevice phys_dev, vk_device_image_zt* p_img, uint32_t width, uint32_t height, uint32_t num_channels, size_t pixel_size, uint32_t mip_levels, VkFormat format, VkImageUsageFlags usage, VkImageAspectFlags flags, const uint32_t* p_queue_families, uint32_t queue_family_count
);
EXTERN_C VkDescriptorImageInfo vk_device_image_desc_info_z(const vk_device_image_zt* p_img, VkImageLayout img_layout);
EXTERN_C void vk_device_image_cleanup_z(vk_device_image_zt* p_img);
EXTERN_C bool vk_device_image_copy_to_rgba8_z(const vk_device_image_zt* p_img, VkPhysicalDevice phys_dev, VkQueue queue, VkCommandPool cmd_pool, VkImageLayout current_layout, uint8_t* out_buffer, uint32_t buffer_capacity);
EXTERN_C void vk_host_visible_image_init_z(
    VkDevice device,
    VkPhysicalDevice phys_dev,
    vk_host_visible_image_zt* p_img,
    uint32_t width,
    uint32_t height,
    uint32_t num_channels,
    size_t pixel_size,
    uint32_t mip_levels,
    VkFormat format,
    VkImageUsageFlags usage,
    VkImageAspectFlags flags,
    const uint32_t* p_queue_families,
    uint32_t queue_family_count
);
EXTERN_C VkDescriptorImageInfo vk_host_visible_image_desc_info_z(const vk_host_visible_image_zt* p_img, VkImageLayout img_layout);
EXTERN_C void vk_host_visible_image_cleanup_z(vk_host_visible_image_zt* p_img);
EXTERN_C void vk_device_local_image_array_init_z(VkDevice device, VkPhysicalDevice phys_dev, vk_device_local_image_array_zt* p_arr, uint32_t max_count);
EXTERN_C void vk_device_local_image_array_desc_infos_z(const vk_device_local_image_array_zt* p_arr, VkImageLayout img_layout, VkDescriptorImageInfo* out_infos);
EXTERN_C vk_device_image_zt* vk_device_local_image_array_get_z(vk_device_local_image_array_zt* p_arr, uint32_t idx);
EXTERN_C void vk_device_local_image_array_cleanup_z(vk_device_local_image_array_zt* p_arr);
EXTERN_C void vk_cmd_buff_pool_init_z(vk_instance_zt* p_inst, vk_cmd_buff_pool_zt* p_pool, uint32_t queue_family_idx);
EXTERN_C void vk_cmd_buff_pool_reset_z(vk_cmd_buff_pool_zt* p_pool);
EXTERN_C VkCommandBuffer vk_cmd_buff_pool_acquire_z(vk_cmd_buff_pool_zt* p_pool);
EXTERN_C VkCommandPool vk_cmd_buff_pool_get_pool_z(const vk_cmd_buff_pool_zt* p_pool);
EXTERN_C void vk_cmd_buff_pool_submit_z(
    vk_cmd_buff_pool_zt* p_pool,
    VkCommandBuffer buff,
    VkQueue queue,
    const vk_semaphore_stage_pair_zt* p_wait_pairs,
    uint32_t wait_count,
    const vk_semaphore_stage_pair_zt* p_signal_pairs,
    uint32_t signal_count,
    VkFence fence,
    const vk_semaphore_timeline_pair_zt* p_timeline_waits,
    uint32_t timeline_wait_count,
    const vk_semaphore_timeline_pair_zt* p_timeline_signals,
    uint32_t timeline_signal_count
);

EXTERN_C void vk_cmd_buff_pool_cleanup_z(vk_cmd_buff_pool_zt* p_pool);
EXTERN_C void vk_device_buff_init_z(VkDevice device, VkPhysicalDevice phys_dev, vk_device_buff_zt* p_buff, size_t stride, uint64_t max_count, VkBufferUsageFlags usage);
EXTERN_C vk_region_handle_zt vk_device_buff_bump_z(vk_device_buff_zt* p_buff, uint64_t num);
EXTERN_C vk_region_handle_zt vk_device_buff_bump_aligned_z(vk_device_buff_zt* p_buff, VkDeviceSize size, VkDeviceSize alignment);
EXTERN_C vk_region_handle_zt vk_device_buff_push_z(vk_device_buff_zt* p_buff, const void* src, uint64_t num);
EXTERN_C void vk_device_buff_remove_z(vk_device_buff_zt* p_buff, vk_region_handle_zt region);
EXTERN_C size_t vk_device_buff_size_z(const vk_device_buff_zt* p_buff);
EXTERN_C VkDescriptorBufferInfo vk_device_buff_desc_info_z(const vk_device_buff_zt* p_buff);
EXTERN_C void vk_device_buff_reset_z(vk_device_buff_zt* p_buff);
EXTERN_C void vk_device_buff_cleanup_z(VkDevice device, vk_device_buff_zt* p_buff);
EXTERN_C void vk_host_buff_init_z(VkDevice device, VkPhysicalDevice phys_dev, vk_host_buff_zt* p_buff, size_t stride, uint32_t max_count, VkBufferUsageFlags usage);
EXTERN_C vk_region_handle_zt vk_host_buff_bump_z(vk_host_buff_zt* p_buff, uint32_t num);
EXTERN_C vk_region_handle_zt vk_host_buff_push_z(vk_host_buff_zt* p_buff, const void* src, uint32_t num);
EXTERN_C void vk_host_buff_remove_z(vk_host_buff_zt* p_buff, vk_region_handle_zt region);
EXTERN_C size_t vk_host_buff_size_z(const vk_host_buff_zt* p_buff);
EXTERN_C void vk_host_buff_reset_z(vk_host_buff_zt* p_buff);
EXTERN_C void vk_host_buff_cleanup_z(VkDevice device, vk_host_buff_zt* p_buff);
EXTERN_C void vk_staged_device_buff_init_z(VkDevice device, VkPhysicalDevice phys_dev, vk_staged_device_buff_zt* p_buff, size_t stride, uint32_t max_count, VkBufferUsageFlags usage);
EXTERN_C vk_region_handle_zt vk_staged_device_buff_bump_z(vk_staged_device_buff_zt* p_buff, uint32_t num);
EXTERN_C vk_region_handle_zt vk_staged_device_buff_push_z(vk_staged_device_buff_zt* p_buff, const void* src, uint32_t num);
EXTERN_C void vk_staged_device_buff_remove_z(vk_staged_device_buff_zt* p_buff, vk_region_handle_zt region);
EXTERN_C void vk_staged_device_buff_push_device_z(vk_staged_device_buff_zt* p_buff);
EXTERN_C size_t vk_staged_device_buff_size_z(const vk_staged_device_buff_zt* p_buff);
EXTERN_C VkDescriptorBufferInfo vk_staged_device_buff_desc_info_z(const vk_staged_device_buff_zt* p_buff);
EXTERN_C void vk_staged_device_buff_reset_z(vk_staged_device_buff_zt* p_buff);
EXTERN_C void vk_staged_device_buff_cleanup_z(VkDevice device, vk_staged_device_buff_zt* p_buff);
EXTERN_C void vk_hidden_device_local_buff_init_z(VkDevice device, VkPhysicalDevice phys_dev, vk_hidden_device_local_buff_zt* p_buff, VkDeviceSize buff_size, VkBufferUsageFlags usage);
EXTERN_C void vk_hidden_device_local_buff_cleanup_z(VkDevice device, vk_hidden_device_local_buff_zt* p_buff);
EXTERN_C void vk_graphics_pass_init_z(vk_instance_zt* p_inst);
EXTERN_C void vk_graphics_pass_cleanup_z(vk_instance_zt* p_inst);
EXTERN_C void vk_graphics_pass_record_cmd_buff_z(vk_instance_zt* p_inst, VkCommandBuffer cmd_buff, vk_device_image_zt** p_colours, uint32_t num_colour_attachments, vk_device_image_zt* p_depth, bool should_clear, vk_graphics_draw_req_zt* p_draw_reqs, uint32_t num_draw_reqs);
EXTERN_C void vk_compute_pass_init_z(vk_instance_zt* p_inst);
EXTERN_C void vk_compute_pass_cleanup_z(vk_instance_zt* p_inst);
EXTERN_C void vk_compute_pass_record_cmd_buff_z(vk_instance_zt* p_inst, VkCommandBuffer cmd_buff, vk_compute_dispatch_req_zt* p_dispatch_reqs, uint32_t num_dispatch_reqs);
EXTERN_C void vk_rt_pass_init_z(vk_instance_zt* p_inst);
EXTERN_C void vk_rt_pass_cleanup_z(vk_instance_zt* p_inst);
EXTERN_C void vk_rt_pass_record_cmd_buff_z(vk_instance_zt* p_inst, VkCommandBuffer cmd_buff, vk_rt_trace_req_zt* p_trace_req);
EXTERN_C vk_shader_group_zt* vk_find_shader_group_z(vk_instance_zt* p_inst, uint64_t id);
EXTERN_C vk_shader_group_zt* vk_get_or_add_shader_group_z(vk_instance_zt* p_inst, uint64_t id);
EXTERN_C void vk_init_instance_func_ptrs_z(vk_instance_zt* p_inst, VkInstance vk_instance, bool enable_validation);
EXTERN_C void vk_init_device_func_ptrs_z(vk_instance_zt* p_inst, VkDevice device, bool using_vk_1_2, bool enable_rt);
EXTERN_C void vk_setup_debug_messenger_z(VkInstance vk_instance, VkDebugUtilsMessengerEXT* p_debug_msgr);
EXTERN_C bool vk_is_validation_supported_z(void);
EXTERN_C vk_swapchain_support_details_zt vk_query_swapchain_support_z(VkSurfaceKHR surface, VkPhysicalDevice phys_dev);
EXTERN_C vk_queue_families_details_zt vk_query_queue_families_z(VkPhysicalDevice phys_dev, VkSurfaceKHR surface);
EXTERN_C void vk_create_swapchain_z(
    VkDevice vk_dev,
    VkSurfaceKHR vk_surface,
    VkPhysicalDevice vk_phys_dev,
    VkFormat swapchain_ideal_format,
    VkColorSpaceKHR swapchain_ideal_colour_space,
    VkPresentModeKHR swapchain_ideal_present_mode,
    VkPresentModeKHR swapchain_fallback_present_mode,
    uint32_t framebuffer_width,
    uint32_t framebuffer_height,
    VkSwapchainKHR* p_vk_swapchain,
    VkImage* p_out_swapchain_imgs,
    uint32_t* p_out_swapchain_img_count,
    uint32_t swapchain_imgs_capacity,
    VkFormat* p_out_swapchain_format,
    VkExtent2D* p_out_swapchain_extent
);
EXTERN_C void vk_init_2_z(vk_instance_zt* p_inst, size_t max_textures, size_t max_fonts);
EXTERN_C VkDeviceAddress vk_get_as_dev_addr_z(vk_instance_zt* p_inst, VkDevice device, VkAccelerationStructureKHR as);
EXTERN_C uint32_t vk_get_candidate_phys_devs_z(VkInstance vk_instance, VkPhysicalDevice* p_out_devices, uint32_t capacity);
EXTERN_C void vk_record_trans_image_layout_z(vk_instance_zt* p_inst, VkCommandBuffer cmd_buff, VkImage image, VkFormat format, VkImageLayout prev_layout, VkImageLayout next_layout);
EXTERN_C void vk_record_blit_z(vk_instance_zt* p_inst, VkCommandBuffer cmd_buff, VkImage src, VkImage dst, VkExtent2D src_size, VkExtent2D dst_size);
EXTERN_C void vk_record_buff_barrier_z(vk_instance_zt* p_inst, VkDevice device, VkCommandBuffer cmd_buff, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size);
EXTERN_C void vk_sampler_init_z(VkDevice device, vk_sampler_zt* p_sampler);
EXTERN_C VkDescriptorImageInfo vk_sampler_desc_info_z(const vk_sampler_zt* p_sampler);
EXTERN_C void vk_tlas_init_z(vk_instance_zt* p_inst, VkDevice device, VkPhysicalDevice phys_dev, vk_tlas_zt* p_tlas, uint32_t MAX_INSTS);
EXTERN_C void vk_tlas_record_build_z(vk_instance_zt* p_inst, VkCommandBuffer cmd_buff, vk_tlas_zt* p_tlas);
EXTERN_C VkWriteDescriptorSetAccelerationStructureKHR vk_tlas_desc_info_z(const vk_tlas_zt* p_tlas);
EXTERN_C void vk_tlas_cleanup_z(vk_tlas_zt* p_tlas);
EXTERN_C void vk_blas_init_tri_blas_z(vk_instance_zt* p_inst, VkDevice device, VkPhysicalDevice phys_dev, vk_blas_zt* p_blas, const uint32_t* p_submesh_tri_counts, uint32_t submesh_count);
EXTERN_C void vk_blas_record_build_tri_blas_z(
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
);
EXTERN_C void vk_blas_record_setup_sphere_blas_z(vk_instance_zt* p_inst, VkDevice device, VkPhysicalDevice phys_dev, VkCommandBuffer cmd_buff, vk_blas_zt* p_blas, VkDeviceAddress aabb_pos_device_addr);
EXTERN_C void vk_blas_cleanup_z(vk_blas_zt* p_blas);
EXTERN_C void vk_as_work_record_build_as_z(VkCommandBuffer cmd_buff, vk_instance_zt* p_inst, vk_device_buff_zt* p_idcs_buff, vk_tlas_zt* p_tlas, const vk_as_work_rebuild_info_zt* p_infos, uint32_t num_infos);
EXTERN_C void vk_tex_sys_init_z(vk_instance_zt* p_inst);
EXTERN_C uint32_t vk_tex_sys_update_buffs_z(vk_instance_zt* p_inst, vk_tex_upload_req_zt* p_requests, uint32_t requests_count);
EXTERN_C void vk_tex_sys_record_cmd_buff_z(vk_instance_zt* p_inst, VkCommandBuffer cmd_buff);
