#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
#define EXTERN_C extern "C"
#else
#define EXTERN_C extern
#endif

// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
typedef struct local_ipc_peer_zt* local_ipc_peer_zh;

// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
EXTERN_C local_ipc_peer_zh local_ipc_init_z(const char* socket_path, bool server);
EXTERN_C void local_ipc_exit_z(local_ipc_peer_zh peer);
EXTERN_C bool local_ipc_try_accept_z(local_ipc_peer_zh peer);
EXTERN_C void local_ipc_send_z(local_ipc_peer_zh peer, const uint8_t* data, size_t data_size);
EXTERN_C bool local_ipc_try_recv_z(local_ipc_peer_zh peer, uint8_t* out_data, size_t* p_out_size, size_t out_capacity);
EXTERN_C void local_ipc_send_fds_z(local_ipc_peer_zh peer, const int* fds, size_t fd_count);
EXTERN_C bool local_ipc_try_recv_fds_z(local_ipc_peer_zh peer, int* out_fds, size_t* p_out_count, size_t max_fds);
