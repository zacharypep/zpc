#pragma once

#include <stddef.h>

#ifdef __cplusplus
#define EXTERN_C extern "C"
#else
#define EXTERN_C extern
#endif

// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
typedef struct repl_zt* repl_zh;

typedef bool (*repl_command_callback_t)(size_t token_count, const char* const* tokens, void* user_data);

typedef struct repl_command_zt
{
    const char* name;
    const char* const* aliases;
    size_t alias_count;
    repl_command_callback_t callback;
    void* user_data;
    const char* description;
} repl_command_zt;

typedef struct repl_config_zt
{
    const char* prompt;
    const char* banner;
    const repl_command_zt* commands;
    size_t command_count;
} repl_config_zt;

// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
EXTERN_C repl_zh repl_init_z(const repl_config_zt* config);
EXTERN_C bool repl_update_z(repl_zh repl);
EXTERN_C void repl_destroy_z(repl_zh repl);
