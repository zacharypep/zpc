#pragma once

#include "zpc/events.h"
#include "zpc/math.h"

#include <stdbool.h>
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
typedef struct
{
    void* glfw_window;
} platform_window_state_zt;

// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
typedef struct
{
    vec2_zt pos;
    vec2_zt d;
} platform_mouse_move_evt_zt;

typedef struct
{
    vec2_zt pos;
} platform_pointer_evt_zt;

typedef platform_pointer_evt_zt platform_pointer_down_evt_zt;
typedef platform_pointer_evt_zt platform_pointer_up_evt_zt;
typedef platform_pointer_evt_zt platform_pointer_click_evt_zt;
typedef platform_pointer_evt_zt platform_pointer_right_click_evt_zt;

typedef struct
{
    vec2_zt pos;
    vec2_zt d;
    bool left_button_pressed;
    bool middle_button_pressed;
    bool right_button_pressed;
    bool is_shift_button_pressed;
} platform_mouse_drag_evt_zt;

typedef struct
{
    float d;
} platform_mouse_scroll_evt_zt;

typedef enum
{
    PLATFORM_KEY_BACKTICK,
    PLATFORM_KEY_T,
    PLATFORM_KEY_E,
    PLATFORM_KEY_Z,
    PLATFORM_KEY_X,
    PLATFORM_KEY_SPACE,
    PLATFORM_KEY_ENTER,
    PLATFORM_KEY_BACKSPACE,
    PLATFORM_KEY_ESCAPE,
    PLATFORM_KEY_F
} platform_keys_zt;

typedef struct
{
    platform_keys_zt key;
} platform_key_down_evt_zt;

typedef struct
{
    vec2_zt axis;
} platform_wasd_axis_changed_evt_zt;

typedef struct
{
    uint32_t unicode;
} platform_char_typed_evt_zt;

typedef struct
{
    bool is_shift_pressed;
    bool is_space_pressed;
    bool is_w_pressed;
    bool is_a_pressed;
    bool is_s_pressed;
    bool is_d_pressed;
} platform_keyboard_state_zt;

typedef struct
{
    vec2_zt mouse_position;
    bool left_button_pressed;
    bool middle_button_pressed;
    bool right_button_pressed;
} platform_mouse_state_zt;

typedef struct
{
    platform_keyboard_state_zt keyboard;
    platform_mouse_state_zt mouse;
    event_zh on_mouse_moved;
    event_zh on_pointer_down;
    event_zh on_pointer_up;
    event_zh on_pointer_clicked;
    event_zh on_pointer_right_clicked;
    event_zh on_mouse_dragged;
    event_zh on_mouse_scrolled;
    event_zh on_key_down;
    event_zh on_wasd_axis_changed;
    event_zh on_keyboard_state_changed;
    event_zh on_char_typed;
} platform_input_state_zt;

// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
typedef struct
{
    platform_window_state_zt window;
    platform_input_state_zt input;
} platform_instance_zt;

// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
EXTERN_C void platform_init_z(platform_instance_zt* p_inst, int window_w, int window_h);
EXTERN_C bool platform_should_close_z(platform_instance_zt* p_inst);
EXTERN_C void platform_poll_events_z(platform_instance_zt* p_inst);
EXTERN_C void platform_cleanup_z(platform_instance_zt* p_inst);
EXTERN_C void platform_get_framebuffer_size_z(platform_instance_zt* p_inst, int* p_width, int* p_height);
EXTERN_C void platform_wait_for_window_z(platform_instance_zt* p_inst);
EXTERN_C void platform_set_window_title_z(platform_instance_zt* p_inst, const char* title);
