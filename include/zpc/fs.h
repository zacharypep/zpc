#pragma once

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#include "zpc/arena.h"

#ifdef __cplusplus
#define EXTERN_C extern "C"
#else
#define EXTERN_C extern
#endif

// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
typedef struct fs_file_entry_zt
{
    char* relative_path;
    char* full_path;
} fs_file_entry_zt;

typedef struct fs_file_list_zt
{
    fs_file_entry_zt* entries;
    size_t count;
    size_t capacity;
} fs_file_list_zt;

typedef struct fs_mapped_file_zt
{
    uint8_t* data;
    size_t size;
} fs_mapped_file_zt;

// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
EXTERN_C bool fs_is_valid_dir_z(const char* path);
EXTERN_C void fs_mkdir_z(const char* path, mode_t mode);
EXTERN_C void fs_collect_files_recursive_z(arena_zh arena, arena_zh scratch_arena, const char* base_dir, fs_file_list_zt* p_out_list);
EXTERN_C void fs_read_file_to_arena_z(arena_zh arena, const char* filepath, span_zt* p_out_span);
EXTERN_C void fs_map_file_readonly_z(const char* filepath, fs_mapped_file_zt* p_out_map);
EXTERN_C void fs_unmap_file_z(fs_mapped_file_zt* p_mapped_file);
EXTERN_C void fs_collect_files_by_extension_z(arena_zh arena, const char* dir_path, const char* extension, char*** file_paths_out, uint32_t* count_out);
EXTERN_C void fs_create_temp_dir_z(arena_zh arena, const char* prefix, char** temp_dir_out);
EXTERN_C char* fs_get_basename_z(arena_zh arena, const char* file_path, const char* extension_to_remove);
EXTERN_C void fs_cleanup_temp_dir_z(const char* temp_dir);
EXTERN_C void fs_write_text_file_z(const char* filepath, const char* text);
EXTERN_C void fs_write_binary_file_z(const char* filepath, const void* data, size_t size);
EXTERN_C void fs_write_binary_blob_z(const void* data, size_t element_size, size_t count, const char* output_dir, const char* prefix, char* filename_out, size_t filename_size);
