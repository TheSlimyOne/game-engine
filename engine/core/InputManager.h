#ifndef GAME_INPUTMANAGER_H
#define GAME_INPUTMANAGER_H

#include "Types.h"
#include <platform/Window.h>
#include <map>
#include <string>
#include <vector>
#include <memory>
#include <functional>


class alignas(16) InputManager {
public:
    InputManager();
    ~InputManager() = default;

    void set_window(const std::shared_ptr<Window>& window);
    void set_cursor_mode(CursorMode mode);
    void query_inputs();

    // Action definition
    void add_action(const std::string& name, KeyCode default_key);
    void add_action(const std::string& name, MouseButton default_btn);
    void add_action(const std::string& name, CompoundBinding binding);

    // Callback registration
    void register_callback(const std::string& name, ActionState state, std::function<void()> callback);
    void register_continuous_callback(const std::string& name, std::function<void(Vec2)> callback);

    // Polling
    ActionState get_action_state(const std::string& name);
    bool is_action_pressed(const std::string& name);
    bool is_action_held(const std::string& name);
    bool is_action_released(const std::string& name);

    Vec2 get_mouse_position();
    Vec2 get_mouse_delta();

    void get_mouse_position_int(int& x, int& y) const;

private:
    bool is_compound_active(const CompoundBinding& compound);

    static KeyCode translate_glfw_key(int glfw_key);
    static MouseButton translate_glfw_mouse_button(int glfw_mouse_button);

    static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
    static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
    static void scroll_callback(GLFWwindow* window, double x_offset, double y_offset);

    void handle_key(int key, int action);
    void handle_mouse_button(int button, int action);
    void handle_cursor_position(double xpos, double ypos);
    void handle_scroll(double x_offset, double y_offset);

    std::shared_ptr<Window> m_window;

    std::map<int, int> m_current_key_states;
    std::map<int, int> m_previous_key_states;

    std::map<std::string, InputAction> m_actions;

    std::map<std::string, ActionState> m_action_states;
    std::map<std::string, ActionState> m_previous_action_states;

    Vec2 m_current_mouse_pos;
    Vec2 m_previous_mouse_pos;
    Vec2 m_mouse_delta;

    bool m_startup_frame = true; // This needs to be placed after Vec2 bc of memory alignment
};

#endif //GAME_INPUTMANAGER_H