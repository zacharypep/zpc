#include "zpc/vk.h"

#include "zpc/fatal.h"

#include <vulkan/vulkan.h>
#include <string.h>

// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
// vk_cmd_buff_pool_init_z: Initialize a command buffer pool. Allocates MAX_BUFFERS command buffers. The pool must be cleaned up with
// vk_cmd_buff_pool_cleanup_z.
// =========================================================================================================================================
// =========================================================================================================================================
void vk_cmd_buff_pool_init_z(vk_instance_zt* p_inst, vk_cmd_buff_pool_zt* p_pool, uint32_t queue_family_idx)
{
    fatal_check_z(p_inst, "p_inst is null");
    fatal_check_z(p_pool, "p_pool is null");

    p_pool->p_inst   = p_inst;
    p_pool->curr_idx = 0;

    // =============================================================================================
    // =============================================================================================
    // create command pool
    // =============================================================================================
    // =============================================================================================
    {
        VkCommandPoolCreateInfo info = {};
        info.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        info.queueFamilyIndex        = queue_family_idx;

        VK_CHECK(vkCreateCommandPool(p_inst->vk_dev, &info, nullptr, &p_pool->command_pool), "failed to create command pool");
    }

    // =============================================================================================
    // =============================================================================================
    // allocate command buffers
    // =============================================================================================
    // =============================================================================================
    {
        VkCommandBufferAllocateInfo alloc_info = {};
        alloc_info.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        alloc_info.commandPool                 = p_pool->command_pool;
        alloc_info.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        alloc_info.commandBufferCount          = VK_CMD_BUFF_POOL_MAX_BUFFERS;

        VK_CHECK(vkAllocateCommandBuffers(p_inst->vk_dev, &alloc_info, p_pool->buffs), "allocating command buffers");
    }
}

// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
// vk_cmd_buff_pool_reset_z: Reset the command pool and reset the current buffer index to zero.
// =========================================================================================================================================
// =========================================================================================================================================
void vk_cmd_buff_pool_reset_z(vk_cmd_buff_pool_zt* p_pool)
{
    fatal_check_z(p_pool, "p_pool is null");

    VK_CHECK(vkResetCommandPool(p_pool->p_inst->vk_dev, p_pool->command_pool, 0), "resetting command pool");
    p_pool->curr_idx = 0;
}

// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
// vk_cmd_buff_pool_acquire_z: Acquire the next available command buffer and begin recording. Returns the command buffer handle.
// =========================================================================================================================================
// =========================================================================================================================================
VkCommandBuffer vk_cmd_buff_pool_acquire_z(vk_cmd_buff_pool_zt* p_pool)
{
    fatal_check_z(p_pool, "p_pool is null");
    fatal_check_bool_z(p_pool->curr_idx < VK_CMD_BUFF_POOL_MAX_BUFFERS, "out of command buffers");

    VkCommandBuffer buff = p_pool->buffs[p_pool->curr_idx];
    p_pool->curr_idx++;

    // =============================================================================================
    // =============================================================================================
    // begin command buffer
    // =============================================================================================
    // =============================================================================================
    {
        VkCommandBufferBeginInfo begin_info = {};
        begin_info.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags                    = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        VK_CHECK(vkBeginCommandBuffer(buff, &begin_info), "beginning command buffer");
    }

    return buff;
}

// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
// vk_cmd_buff_pool_get_pool_z: Get the underlying Vulkan command pool handle.
// =========================================================================================================================================
// =========================================================================================================================================
VkCommandPool vk_cmd_buff_pool_get_pool_z(const vk_cmd_buff_pool_zt* p_pool)
{
    fatal_check_z(p_pool, "p_pool is null");
    return p_pool->command_pool;
}

// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
// vk_cmd_buff_pool_submit_z: Submit a command buffer to a queue. Ends the command buffer recording and submits it with the specified
// semaphore waits/signals and optional fence. Pass nullptr and 0 for empty arrays.
// =========================================================================================================================================
// =========================================================================================================================================
void vk_cmd_buff_pool_submit_z(
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
)
{
    fatal_check_z(p_pool, "p_pool is null");
    fatal_check_bool_z(buff != VK_NULL_HANDLE, "buff is null");
    fatal_check_bool_z(queue != VK_NULL_HANDLE, "queue is null");

    vkEndCommandBuffer(buff);

    // =============================================================================================
    // =============================================================================================
    // build wait semaphore infos
    // =============================================================================================
    // =============================================================================================
    uint32_t total_wait_count                                          = wait_count + timeline_wait_count;
    VkSemaphoreSubmitInfo wait_infos[VK_CMD_BUFF_POOL_MAX_BUFFERS * 2] = {}; // Reasonable max size, zero-initialized
    {
        fatal_check_bool_z(total_wait_count <= sizeof(wait_infos) / sizeof(wait_infos[0]), "too many wait semaphores");

        for (uint32_t i = 0; i < wait_count; i++)
        {
            wait_infos[i].sType     = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
            wait_infos[i].pNext     = nullptr;
            wait_infos[i].semaphore = p_wait_pairs[i].semaphore;
            wait_infos[i].value     = 0;
            wait_infos[i].stageMask = p_wait_pairs[i].stage;
        }

        for (uint32_t i = 0; i < timeline_wait_count; i++)
        {
            uint32_t idx              = wait_count + i;
            wait_infos[idx].sType     = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
            wait_infos[idx].pNext     = nullptr;
            wait_infos[idx].semaphore = p_timeline_waits[i].semaphore;
            wait_infos[idx].value     = p_timeline_waits[i].value;
            wait_infos[idx].stageMask = p_timeline_waits[i].stage;
        }
    }

    // =============================================================================================
    // =============================================================================================
    // build signal semaphore infos
    // =============================================================================================
    // =============================================================================================
    uint32_t total_signal_count                                          = signal_count + timeline_signal_count;
    VkSemaphoreSubmitInfo signal_infos[VK_CMD_BUFF_POOL_MAX_BUFFERS * 2] = {}; // Reasonable max size, zero-initialized
    {
        fatal_check_bool_z(total_signal_count <= sizeof(signal_infos) / sizeof(signal_infos[0]), "too many signal semaphores");

        for (uint32_t i = 0; i < signal_count; i++)
        {
            signal_infos[i].sType     = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
            signal_infos[i].pNext     = nullptr;
            signal_infos[i].semaphore = p_signal_pairs[i].semaphore;
            signal_infos[i].value     = 0;
            signal_infos[i].stageMask = p_signal_pairs[i].stage;
        }

        for (uint32_t i = 0; i < timeline_signal_count; i++)
        {
            uint32_t idx                = signal_count + i;
            signal_infos[idx].sType     = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
            signal_infos[idx].pNext     = nullptr;
            signal_infos[idx].semaphore = p_timeline_signals[i].semaphore;
            signal_infos[idx].value     = p_timeline_signals[i].value;
            signal_infos[idx].stageMask = p_timeline_signals[i].stage;
        }
    }

    // =============================================================================================
    // =============================================================================================
    // build submit info
    // =============================================================================================
    // =============================================================================================
    VkCommandBufferSubmitInfo cmd_buff_info = {};
    cmd_buff_info.sType                     = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
    cmd_buff_info.commandBuffer             = buff;
    cmd_buff_info.deviceMask                = 0;
    cmd_buff_info.pNext                     = nullptr;

    VkSubmitInfo2 info                      = {};
    info.sType                              = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    info.flags                              = 0;
    info.waitSemaphoreInfoCount             = total_wait_count;
    info.pWaitSemaphoreInfos                = total_wait_count > 0 ? wait_infos : nullptr;
    info.signalSemaphoreInfoCount           = total_signal_count;
    info.pSignalSemaphoreInfos              = total_signal_count > 0 ? signal_infos : nullptr;
    info.commandBufferInfoCount             = 1;
    info.pCommandBufferInfos                = &cmd_buff_info;
    info.pNext                              = nullptr;

    VkResult result;
    if (p_pool->p_inst->using_vk_1_2)
    {
        result = p_pool->p_inst->func_ptrs.vkQueueSubmit2(queue, 1, &info, fence);
    }
    else
    {
        result = vkQueueSubmit2(queue, 1, &info, fence);
    }
    if (result != VK_SUCCESS) fatal_z("failed: submitting command buffer, error: %d", result);
}

// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
// vk_cmd_buff_pool_cleanup_z: Clean up command buffer pool resources. Frees command buffers and destroys the command pool.
// =========================================================================================================================================
// =========================================================================================================================================
void vk_cmd_buff_pool_cleanup_z(vk_cmd_buff_pool_zt* p_pool)
{
    fatal_check_z(p_pool, "p_pool is null");

    if (p_pool->command_pool != VK_NULL_HANDLE)
    {
        vkFreeCommandBuffers(p_pool->p_inst->vk_dev, p_pool->command_pool, VK_CMD_BUFF_POOL_MAX_BUFFERS, p_pool->buffs);
        vkDestroyCommandPool(p_pool->p_inst->vk_dev, p_pool->command_pool, nullptr);
        p_pool->command_pool = VK_NULL_HANDLE;
    }
}
