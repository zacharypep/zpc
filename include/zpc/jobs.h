
#pragma once

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#ifdef __cplusplus
#define EXTERN_C extern "C"
#else
#define EXTERN_C extern
#endif

// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
typedef struct job_id_zt
{
    uint64_t value;
} job_id_zt;

#define JOB_ID_NULL \
    { \
    }

typedef enum job_status_ze
{
    JOB_STATUS_IDLE      = 0,
    JOB_STATUS_RUNNING   = 1,
    JOB_STATUS_COMPLETED = 2,
    JOB_STATUS_FAILED    = 3
} job_status_ze;

typedef struct jobs_system_zt* jobs_system_zh;

// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
EXTERN_C jobs_system_zh jobs_system_init_z(void);
EXTERN_C void jobs_system_update_z(jobs_system_zh system);
EXTERN_C void jobs_system_cleanup_z(jobs_system_zh system);
EXTERN_C bool jobs_start_z(jobs_system_zh system, const char* command, const char* working_dir, job_id_zt* p_out_job_id);
EXTERN_C void jobs_stop_z(jobs_system_zh system, job_id_zt job_id);
EXTERN_C bool jobs_is_running_z(jobs_system_zh system, job_id_zt job_id);
EXTERN_C job_status_ze jobs_get_status_z(jobs_system_zh system, job_id_zt job_id);
EXTERN_C void jobs_get_status_message_z(jobs_system_zh system, job_id_zt job_id, char* buffer, size_t buffer_size);
EXTERN_C pid_t jobs_get_pid_z(jobs_system_zh system, job_id_zt job_id);
