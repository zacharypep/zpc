#pragma once

#include <stddef.h>
#include <stdint.h>

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
typedef struct json_value_zt* json_zh;

enum json_type
{
    JSON_TYPE_NULL,
    JSON_TYPE_OBJECT,
    JSON_TYPE_ARRAY,
    JSON_TYPE_STRING,
    JSON_TYPE_INTEGER,
    JSON_TYPE_BOOLEAN,
    JSON_TYPE_REAL
};

// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
EXTERN_C json_zh json_object_z(arena_zh arena);
EXTERN_C json_zh json_array_z(arena_zh arena);
EXTERN_C json_zh json_string_z(arena_zh arena, const char* str);
EXTERN_C json_zh json_integer_z(arena_zh arena, int64_t value);
EXTERN_C json_zh json_boolean_z(arena_zh arena, bool value);
EXTERN_C json_zh json_real_z(arena_zh arena, double value);

EXTERN_C void json_object_set_z(json_zh object, const char* key, json_zh value);
EXTERN_C void json_array_append_z(json_zh array, json_zh value);

EXTERN_C char* json_dumps_z(arena_zh arena, json_zh value);

EXTERN_C json_zh json_loads_z(arena_zh arena, const char* json_str);
EXTERN_C json_zh json_load_file_z(arena_zh arena, const char* filepath);

EXTERN_C enum json_type json_type_z(json_zh value);

EXTERN_C bool json_object_has_z(json_zh object, const char* key);
EXTERN_C bool json_object_has_string_z(json_zh object, const char* key);
EXTERN_C bool json_object_has_integer_z(json_zh object, const char* key);
EXTERN_C bool json_object_has_real_z(json_zh object, const char* key);
EXTERN_C bool json_object_has_boolean_z(json_zh object, const char* key);
EXTERN_C bool json_object_has_object_z(json_zh object, const char* key);
EXTERN_C bool json_object_has_array_z(json_zh object, const char* key);

EXTERN_C const char* json_object_get_string_z(json_zh object, const char* key);
EXTERN_C int64_t json_object_get_integer_z(json_zh object, const char* key);
EXTERN_C double json_object_get_real_z(json_zh object, const char* key);
EXTERN_C bool json_object_get_boolean_z(json_zh object, const char* key);
EXTERN_C json_zh json_object_get_object_z(json_zh object, const char* key);
EXTERN_C json_zh json_object_get_array_z(json_zh object, const char* key);

EXTERN_C size_t json_array_size_z(json_zh array);
EXTERN_C const char* json_array_get_string_z(json_zh array, size_t index);
EXTERN_C int64_t json_array_get_integer_z(json_zh array, size_t index);
EXTERN_C double json_array_get_real_z(json_zh array, size_t index);
EXTERN_C json_zh json_array_get_object_z(json_zh array, size_t index);
EXTERN_C json_zh json_array_get_array_z(json_zh array, size_t index);
EXTERN_C bool json_array_get_boolean_z(json_zh array, size_t index);
