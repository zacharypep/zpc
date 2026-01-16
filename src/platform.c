#include "zpc/platform.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "zpc/math.h"
#include <GLFW/glfw3.h>

// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    (void)scancode;
    (void)mods;

    // ====================================================================================================
    // ====================================================================================================
    // ====================================================================================================
    // ====================================================================================================
    platform_instance_zt* p_inst = (platform_instance_zt*)glfwGetWindowUserPointer(window);

    // ====================================================================================================
    // ====================================================================================================
    // Update the tracked key state for WASD and shift modifiers.
    // ====================================================================================================
    // ====================================================================================================
    bool did_shift_wasd_change;
    {
        static platform_keyboard_state_zt previous_keyboard_state = {0};

        bool* key_slot;
        {
            if (key == GLFW_KEY_W)
            {
                key_slot = &p_inst->input.keyboard.is_w_pressed;
            }
            else if (key == GLFW_KEY_A)
            {
                key_slot = &p_inst->input.keyboard.is_a_pressed;
            }
            else if (key == GLFW_KEY_S)
            {
                key_slot = &p_inst->input.keyboard.is_s_pressed;
            }
            else if (key == GLFW_KEY_D)
            {
                key_slot = &p_inst->input.keyboard.is_d_pressed;
            }
            else if (key == GLFW_KEY_LEFT_SHIFT)
            {
                key_slot = &p_inst->input.keyboard.is_shift_pressed;
            }
            else if (key == GLFW_KEY_SPACE)
            {
                key_slot = &p_inst->input.keyboard.is_space_pressed;
            }
            else
            {
                key_slot = NULL;
            }
        }

        if (key_slot != NULL)
        {
            if (action == GLFW_PRESS)
            {
                *key_slot = true;
            }
            else if (action == GLFW_RELEASE)
            {
                *key_slot = false;
            }
        }

        did_shift_wasd_change = previous_keyboard_state.is_shift_pressed != p_inst->input.keyboard.is_shift_pressed || previous_keyboard_state.is_w_pressed != p_inst->input.keyboard.is_w_pressed || previous_keyboard_state.is_a_pressed != p_inst->input.keyboard.is_a_pressed
                             || previous_keyboard_state.is_s_pressed != p_inst->input.keyboard.is_s_pressed || previous_keyboard_state.is_d_pressed != p_inst->input.keyboard.is_d_pressed;

        if (did_shift_wasd_change)
        {
            event_trigger_z(p_inst->input.on_keyboard_state_changed, NULL);
            previous_keyboard_state = p_inst->input.keyboard;
        }
    }

    // ====================================================================================================
    // ====================================================================================================
    // Broadcast per-key events for hotkeys that should fire on press or repeat.
    // ====================================================================================================
    // ====================================================================================================
    {
        const bool is_press_or_repeat = action == GLFW_PRESS || action == GLFW_REPEAT;
        if (is_press_or_repeat)
        {
            platform_key_down_evt_zt evt;
            if (key == GLFW_KEY_GRAVE_ACCENT)
            {
                evt.key = PLATFORM_KEY_BACKTICK;
                event_trigger_z(p_inst->input.on_key_down, &evt);
            }
            else if (key == GLFW_KEY_T)
            {
                evt.key = PLATFORM_KEY_T;
                event_trigger_z(p_inst->input.on_key_down, &evt);
            }
            else if (key == GLFW_KEY_E)
            {
                evt.key = PLATFORM_KEY_E;
                event_trigger_z(p_inst->input.on_key_down, &evt);
            }
            else if (key == GLFW_KEY_Z)
            {
                evt.key = PLATFORM_KEY_Z;
                event_trigger_z(p_inst->input.on_key_down, &evt);
            }
            else if (key == GLFW_KEY_X)
            {
                evt.key = PLATFORM_KEY_X;
                event_trigger_z(p_inst->input.on_key_down, &evt);
            }
            else if (key == GLFW_KEY_SPACE)
            {
                evt.key = PLATFORM_KEY_SPACE;
                event_trigger_z(p_inst->input.on_key_down, &evt);
            }
            else if (key == GLFW_KEY_ENTER)
            {
                evt.key = PLATFORM_KEY_ENTER;
                event_trigger_z(p_inst->input.on_key_down, &evt);
            }
            else if (key == GLFW_KEY_BACKSPACE)
            {
                evt.key = PLATFORM_KEY_BACKSPACE;
                event_trigger_z(p_inst->input.on_key_down, &evt);
            }
            else if (key == GLFW_KEY_ESCAPE)
            {
                evt.key = PLATFORM_KEY_ESCAPE;
                event_trigger_z(p_inst->input.on_key_down, &evt);
            }
            else if (key == GLFW_KEY_F)
            {
                evt.key = PLATFORM_KEY_F;
                event_trigger_z(p_inst->input.on_key_down, &evt);
            }
        }
    }

    // ====================================================================================================
    // ====================================================================================================
    // ====================================================================================================
    // ====================================================================================================
    {
        if (did_shift_wasd_change)
        {
            static vec2_zt previous_wasd_axis = {0.0f, 0.0f};

            vec2_zt current_wasd_axis = {0.0f, 0.0f};
            {
                if (p_inst->input.keyboard.is_d_pressed) current_wasd_axis.x += 1.0f;
                if (p_inst->input.keyboard.is_a_pressed) current_wasd_axis.x -= 1.0f;
                if (p_inst->input.keyboard.is_w_pressed) current_wasd_axis.y += 1.0f;
                if (p_inst->input.keyboard.is_s_pressed) current_wasd_axis.y -= 1.0f;

                current_wasd_axis = vec2_normalize_z(current_wasd_axis);
            }

            if (!vec2_eq_z(previous_wasd_axis, current_wasd_axis))
            {
                platform_wasd_axis_changed_evt_zt evt;
                evt.axis = current_wasd_axis;
                event_trigger_z(p_inst->input.on_wasd_axis_changed, &evt);
                previous_wasd_axis = current_wasd_axis;
            }
        }
    }
}

// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
    platform_instance_zt* p_inst = (platform_instance_zt*)glfwGetWindowUserPointer(window);
    platform_mouse_state_zt* mouse_state = &p_inst->input.mouse;
    platform_keyboard_state_zt* keyboard_state = &p_inst->input.keyboard;

    // ====================================================================================================
    // ====================================================================================================
    // Rate-limit high frequency input to roughly 100 Hz to reduce event spam.
    // ====================================================================================================
    // ====================================================================================================
    {
        static double last_time = 0.0;
        const double current_time = glfwGetTime();
        const double elapsed_ms = (current_time - last_time) * 1000.0;
        if (elapsed_ms < 10.0)
        {
            return;
        }
        last_time = current_time;
    }

    // ====================================================================================================
    // ====================================================================================================
    // Convert screen coordinates to normalised window space.
    // ====================================================================================================
    // ====================================================================================================
    vec2_zt new_pos;
    {
        int width = 0;
        int height = 0;
        glfwGetWindowSize(window, &width, &height);

        if (width == 0 || height == 0)
        {
            return;
        }

        const vec2_zt window_size = {(float)width, (float)height};
        new_pos = vec2_div_z((vec2_zt){(float)xpos, (float)ypos}, window_size);
    }

    const vec2_zt prev_pos = mouse_state->mouse_position;
    const vec2_zt delta = vec2_sub_z(new_pos, prev_pos);

    mouse_state->mouse_position = new_pos;

    // ====================================================================================================
    // ====================================================================================================
    // Emit pointer move and optional drag events.
    // ====================================================================================================
    // ====================================================================================================
    {
        platform_mouse_move_evt_zt move_evt;
        move_evt.pos = new_pos;
        move_evt.d = delta;
        event_trigger_z(p_inst->input.on_mouse_moved, &move_evt);

        if (mouse_state->left_button_pressed || mouse_state->middle_button_pressed || mouse_state->right_button_pressed)
        {
            platform_mouse_drag_evt_zt drag_evt;
            drag_evt.pos = new_pos;
            drag_evt.d = delta;
            drag_evt.left_button_pressed = mouse_state->left_button_pressed;
            drag_evt.middle_button_pressed = mouse_state->middle_button_pressed;
            drag_evt.right_button_pressed = mouse_state->right_button_pressed;
            drag_evt.is_shift_button_pressed = keyboard_state->is_shift_pressed;
            event_trigger_z(p_inst->input.on_mouse_dragged, &drag_evt);
        }
    }
}

// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    (void)mods;

    platform_instance_zt* p_inst = (platform_instance_zt*)glfwGetWindowUserPointer(window);
    platform_mouse_state_zt* mouse_state = &p_inst->input.mouse;

    // ====================================================================================================
    // ====================================================================================================
    // Handle left mouse button transitions.
    // ====================================================================================================
    // ====================================================================================================
    {
        if (button == GLFW_MOUSE_BUTTON_LEFT)
        {
            if (action == GLFW_PRESS)
            {
                mouse_state->left_button_pressed = true;
                platform_pointer_down_evt_zt down_evt;
                down_evt.pos = mouse_state->mouse_position;
                event_trigger_z(p_inst->input.on_pointer_down, &down_evt);
                platform_pointer_click_evt_zt click_evt;
                click_evt.pos = mouse_state->mouse_position;
                event_trigger_z(p_inst->input.on_pointer_clicked, &click_evt);
            }
            else if (action == GLFW_RELEASE)
            {
                mouse_state->left_button_pressed = false;
                platform_pointer_up_evt_zt up_evt;
                up_evt.pos = mouse_state->mouse_position;
                event_trigger_z(p_inst->input.on_pointer_up, &up_evt);
            }
            return;
        }
    }

    // ====================================================================================================
    // ====================================================================================================
    // Handle right mouse button transitions.
    // ====================================================================================================
    // ====================================================================================================
    {
        if (button == GLFW_MOUSE_BUTTON_RIGHT)
        {
            if (action == GLFW_PRESS)
            {
                mouse_state->right_button_pressed = true;
                platform_pointer_right_click_evt_zt evt;
                evt.pos = mouse_state->mouse_position;
                event_trigger_z(p_inst->input.on_pointer_right_clicked, &evt);
            }
            else if (action == GLFW_RELEASE)
            {
                mouse_state->right_button_pressed = false;
            }
            return;
        }
    }

    // ====================================================================================================
    // ====================================================================================================
    // Handle middle mouse button transitions.
    // ====================================================================================================
    // ====================================================================================================
    {
        if (button == GLFW_MOUSE_BUTTON_MIDDLE)
        {
            if (action == GLFW_PRESS)
            {
                mouse_state->middle_button_pressed = true;
            }
            else if (action == GLFW_RELEASE)
            {
                mouse_state->middle_button_pressed = false;
            }
        }
    }
}

// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    (void)window;
    (void)xoffset;

    platform_instance_zt* p_inst = (platform_instance_zt*)glfwGetWindowUserPointer(window);
    platform_mouse_scroll_evt_zt evt;
    evt.d = (float)yoffset;
    event_trigger_z(p_inst->input.on_mouse_scrolled, &evt);
}

// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
static void character_callback(GLFWwindow* window, unsigned int codepoint)
{
    platform_instance_zt* p_inst = (platform_instance_zt*)glfwGetWindowUserPointer(window);
    platform_char_typed_evt_zt evt;
    evt.unicode = codepoint;
    event_trigger_z(p_inst->input.on_char_typed, &evt);
}

// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
void platform_init_z(platform_instance_zt* p_inst, int window_w, int window_h)
{
    // ====================================================================================================
    // ====================================================================================================
    // Initialize event handles
    // ====================================================================================================
    // ====================================================================================================
    {
        p_inst->input.on_mouse_moved = event_init_z();
        p_inst->input.on_pointer_down = event_init_z();
        p_inst->input.on_pointer_up = event_init_z();
        p_inst->input.on_pointer_clicked = event_init_z();
        p_inst->input.on_pointer_right_clicked = event_init_z();
        p_inst->input.on_mouse_dragged = event_init_z();
        p_inst->input.on_mouse_scrolled = event_init_z();
        p_inst->input.on_key_down = event_init_z();
        p_inst->input.on_wasd_axis_changed = event_init_z();
        p_inst->input.on_keyboard_state_changed = event_init_z();
        p_inst->input.on_char_typed = event_init_z();
    }

    // ====================================================================================================
    // ====================================================================================================
    // Initialize state
    // ====================================================================================================
    // ====================================================================================================
    {
        memset(&p_inst->input.keyboard, 0, sizeof(p_inst->input.keyboard));
        memset(&p_inst->input.mouse, 0, sizeof(p_inst->input.mouse));
    }

    // ====================================================================================================
    // ====================================================================================================
    // Initialise GLFW and create a window without an OpenGL context.
    // ====================================================================================================
    // ====================================================================================================
    {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        p_inst->window.glfw_window = glfwCreateWindow(window_w, window_h, "zp_platform", NULL, NULL);
    }

    // ====================================================================================================
    // ====================================================================================================
    // Register this instance and the callbacks that translate GLFW events.
    // ====================================================================================================
    // ====================================================================================================
    {
        glfwSetWindowUserPointer((GLFWwindow*)p_inst->window.glfw_window, p_inst);
        glfwSetKeyCallback((GLFWwindow*)p_inst->window.glfw_window, key_callback);
        glfwSetCursorPosCallback((GLFWwindow*)p_inst->window.glfw_window, cursor_position_callback);
        glfwSetMouseButtonCallback((GLFWwindow*)p_inst->window.glfw_window, mouse_button_callback);
        glfwSetScrollCallback((GLFWwindow*)p_inst->window.glfw_window, scroll_callback);
        glfwSetCharCallback((GLFWwindow*)p_inst->window.glfw_window, character_callback);
    }
}

// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
bool platform_should_close_z(platform_instance_zt* p_inst)
{
    return glfwWindowShouldClose((GLFWwindow*)p_inst->window.glfw_window) != 0;
}

// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
void platform_poll_events_z(platform_instance_zt* p_inst)
{
    (void)p_inst;
    glfwPollEvents();
}

// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
void platform_cleanup_z(platform_instance_zt* p_inst)
{
    // ====================================================================================================
    // ====================================================================================================
    // Destroy event handles
    // ====================================================================================================
    // ====================================================================================================
    {
        event_destroy_z(p_inst->input.on_mouse_moved);
        event_destroy_z(p_inst->input.on_pointer_down);
        event_destroy_z(p_inst->input.on_pointer_up);
        event_destroy_z(p_inst->input.on_pointer_clicked);
        event_destroy_z(p_inst->input.on_pointer_right_clicked);
        event_destroy_z(p_inst->input.on_mouse_dragged);
        event_destroy_z(p_inst->input.on_mouse_scrolled);
        event_destroy_z(p_inst->input.on_key_down);
        event_destroy_z(p_inst->input.on_wasd_axis_changed);
        event_destroy_z(p_inst->input.on_keyboard_state_changed);
        event_destroy_z(p_inst->input.on_char_typed);
    }

    glfwDestroyWindow((GLFWwindow*)p_inst->window.glfw_window);
    p_inst->window.glfw_window = NULL;
    glfwTerminate();
}

// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
void platform_get_framebuffer_size_z(platform_instance_zt* p_inst, int* p_width, int* p_height)
{
    glfwGetFramebufferSize((GLFWwindow*)p_inst->window.glfw_window, p_width, p_height);
}

// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
void platform_wait_for_window_z(platform_instance_zt* p_inst)
{
    // ====================================================================================================
    // ====================================================================================================
    // Poll framebuffer size until a non-zero surface is reported.
    // ====================================================================================================
    // ====================================================================================================
    {
        int width = 0;
        int height = 0;
        do
        {
            glfwGetFramebufferSize((GLFWwindow*)p_inst->window.glfw_window, &width, &height);
            if (width == 0 || height == 0)
            {
                glfwWaitEvents();
            }
        } while (width == 0 || height == 0);
    }
}

// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
// =========================================================================================================================================
void platform_set_window_title_z(platform_instance_zt* p_inst, const char* title)
{
    glfwSetWindowTitle((GLFWwindow*)p_inst->window.glfw_window, title);
}

