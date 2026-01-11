#include "InputManager.h"

#include <iostream>
#include <GLFW/glfw3.h>

constexpr int MOUSE_BUTTON_OFFSET = 1000;

InputManager::InputManager()
    :   m_current_mouse_pos(0.0f, 0.0f),
        m_previous_mouse_pos(0.0f, 0.0f)
{}

void InputManager::set_window(const std::shared_ptr<Window> &window) {
    if (m_window) {
        glfwSetWindowUserPointer(m_window->get_glfw_window(), nullptr);
    }
    m_window = window;

    if (m_window) {
        glfwSetWindowUserPointer(m_window->get_glfw_window(), this);

        // Set GLFW callbacks
        glfwSetKeyCallback(m_window->get_glfw_window(), key_callback);
        glfwSetMouseButtonCallback(m_window->get_glfw_window(), mouse_button_callback);
        glfwSetCursorPosCallback(m_window->get_glfw_window(), cursor_position_callback);
        glfwSetScrollCallback(m_window->get_glfw_window(), scroll_callback);

        // Seed the initial position immediately
        double x, y;
        glfwGetCursorPos(m_window->get_glfw_window(), &x, &y);
        m_current_mouse_pos = Vec2(static_cast<float>(x), static_cast<float>(y));
        m_previous_mouse_pos = m_current_mouse_pos;
    }
}

void InputManager::set_cursor_mode(CursorMode mode) {
    if (!m_window) {
        return;
    }

    int glfw_mode;
    switch (mode) {
        case CursorMode::Normal:
            glfw_mode = GLFW_CURSOR_NORMAL;
            break;
        case CursorMode::Hidden:
            glfw_mode = GLFW_CURSOR_HIDDEN;
            break;
        case CursorMode::Disabled:
            glfw_mode = GLFW_CURSOR_DISABLED;
            break;
        default:
            glfw_mode = GLFW_CURSOR_NORMAL;
            break;
    }

    glfwSetInputMode(m_window->get_glfw_window(), GLFW_CURSOR, glfw_mode);
}

void InputManager::query_inputs() {
    // If this is the first time we are querying inputs, sync the previous
    // position to the current position to prevent a huge initial delta.
    if (m_startup_frame) {
        m_previous_mouse_pos = m_current_mouse_pos;
        m_startup_frame = false;
    }

    // Process actions and trigger callbacks
    Vec2 raw_delta = m_current_mouse_pos - m_previous_mouse_pos;

    const float epsilon = 2.0f;
    if (raw_delta.norm() < epsilon) {
        m_mouse_delta = Vec2(0.0f);
    } else {
        const float sensitivity = 0.1f;
        m_mouse_delta = raw_delta * sensitivity;
    }

    Vec2 mouse_delta = get_mouse_delta();

    for (auto& pair: m_actions) {
        auto& action = pair.second;
        bool is_active = false;

        // Check if any of the action's compound bindings are met
        for (const auto& compound : action.compounds) {
            if (is_compound_active(compound)) {
                is_active = true;
                break;
            }
        }


        // Determine the current state of the action
        ActionState previous_state = m_previous_action_states.count(action.name)
            ? m_previous_action_states[action.name] : ActionState::Idle;
        ActionState current_state = ActionState::Idle;

        if (is_active) {
            if (previous_state == ActionState::Idle || previous_state == ActionState::Released) {
                current_state = ActionState::Pressed;
            } else {
                current_state = ActionState::Held;
            }
        } else {
            if (previous_state == ActionState::Pressed || previous_state == ActionState::Held) {
                current_state = ActionState::Released;
            } else {
                current_state = ActionState::Idle;
            }
        }

        m_action_states[action.name] = current_state;

        // Fire callbacks based on the new state
        switch (current_state) {
            case ActionState::Pressed:
                if (action.on_press) {
                    action.on_press();
                }
                break;
            case ActionState::Released:
                if (action.on_release) {
                    action.on_release();
                }
                break;
            case ActionState::Held:
                if (action.on_hold) {
                    action.on_hold();
                }
                break;
            default:
                break;
        }

        // Fire continuous callback if the action is active and has one
        if (is_active && action.on_continuous) {
            action.on_continuous(mouse_delta);
        }
    }

    // Update preivous states
    m_previous_key_states = m_current_key_states;
    m_previous_action_states = m_action_states;
    m_previous_mouse_pos = m_current_mouse_pos;
}

void InputManager::add_action(const std::string &name, KeyCode default_key) {
    InputBinding binding = {InputType::Key, static_cast<int>(default_key)};
    add_action(name, CompoundBinding{{binding}});
}

void InputManager::add_action(const std::string &name, MouseButton default_btn) {
    int code = static_cast<int>(default_btn) + MOUSE_BUTTON_OFFSET;
    InputBinding binding = {InputType::MouseButton, code};
    add_action(name, CompoundBinding{{binding}});
}

void InputManager::add_action(const std::string &name, CompoundBinding binding) {
    if (m_actions.find(name) == m_actions.end()) {
        m_actions[name] = InputAction{name, {binding}};
    } else {
        m_actions[name].compounds.push_back(binding);
    }
}

void InputManager::register_callback(const std::string &name, ActionState state, std::function<void()> callback) {
    auto it = m_actions.find(name);
    if (it != m_actions.end()) {
        switch (state) {
            case ActionState::Pressed:
                it->second.on_press = callback;
                break;
            case ActionState::Released:
                it->second.on_release = callback;
                break;
            case ActionState::Held:
                it->second.on_hold = callback;
                break;
            default:
                break;
        }
    } else {
        std::cerr << "Warning: Attempted to register callback for non-existent action: " << name << std::endl;
    }
}

void InputManager::register_continuous_callback(const std::string &name, std::function<void(Vec2)> callback) {
    auto it = m_actions.find(name);
    if (it != m_actions.end()) {
        it->second.on_continuous = callback;
    } else {
        std::cerr << "Warning: Attempted to register continuous callback for non-existent action: " << name << std::endl;
    }
}

ActionState InputManager::get_action_state(const std::string &name) {
    if (m_action_states.count(name)) {
        return m_action_states[name];
    }
    return ActionState::Idle;
}

bool InputManager::is_action_pressed(const std::string &name) {
    return get_action_state(name) == ActionState::Pressed;
}

bool InputManager::is_action_held(const std::string &name) {
    ActionState state = get_action_state(name);
    return state == ActionState::Held || state == ActionState::Pressed;
}

bool InputManager::is_action_released(const std::string &name) {
    return get_action_state(name) == ActionState::Released;
}

Vec2 InputManager::get_mouse_position() {
    return m_current_mouse_pos;
}

Vec2 InputManager::get_mouse_delta() {
    return m_mouse_delta;
}

void InputManager::get_mouse_position_int(int& x, int& y) const {
    x = static_cast<int>(m_current_mouse_pos.x);
    y = static_cast<int>(m_current_mouse_pos.y);
}

bool InputManager::is_compound_active(const CompoundBinding &compound) {
    // Check if all required kes/buttons are held down
    for (const auto& binding : compound.bindings) {
        bool is_held = false;
        if (m_current_key_states.count(binding.code)) {
            int state = m_current_key_states[binding.code];
            is_held = (state == GLFW_PRESS || state == GLFW_REPEAT);
        }
        if (!is_held) {
            return false;
        }
    }

    // If it also requires mouse movement, check the delta
    if (compound.requires_mouse_move) {
        if (get_mouse_delta().x == 0.0f && get_mouse_delta().y == 0.0f) {
            return false;
        }
    }

    return true;
}

KeyCode InputManager::translate_glfw_key(int glfw_key) {
    switch (glfw_key) {
        case GLFW_KEY_A: return KeyCode::A; case GLFW_KEY_B: return KeyCode::B;
        case GLFW_KEY_C: return KeyCode::C; case GLFW_KEY_D: return KeyCode::D;
        case GLFW_KEY_E: return KeyCode::E; case GLFW_KEY_F: return KeyCode::F;
        case GLFW_KEY_G: return KeyCode::G; case GLFW_KEY_H: return KeyCode::H;
        case GLFW_KEY_I: return KeyCode::I; case GLFW_KEY_J: return KeyCode::J;
        case GLFW_KEY_K: return KeyCode::K; case GLFW_KEY_L: return KeyCode::L;
        case GLFW_KEY_M: return KeyCode::M; case GLFW_KEY_N: return KeyCode::N;
        case GLFW_KEY_O: return KeyCode::O; case GLFW_KEY_P: return KeyCode::P;
        case GLFW_KEY_Q: return KeyCode::Q; case GLFW_KEY_R: return KeyCode::R;
        case GLFW_KEY_S: return KeyCode::S; case GLFW_KEY_T: return KeyCode::T;
        case GLFW_KEY_U: return KeyCode::U; case GLFW_KEY_V: return KeyCode::V;
        case GLFW_KEY_W: return KeyCode::W; case GLFW_KEY_X: return KeyCode::X;
        case GLFW_KEY_Y: return KeyCode::Y; case GLFW_KEY_Z: return KeyCode::Z;
        case GLFW_KEY_0: return KeyCode::Num0; case GLFW_KEY_1: return KeyCode::Num1;
        case GLFW_KEY_2: return KeyCode::Num2; case GLFW_KEY_3: return KeyCode::Num3;
        case GLFW_KEY_4: return KeyCode::Num4; case GLFW_KEY_5: return KeyCode::Num5;
        case GLFW_KEY_6: return KeyCode::Num6; case GLFW_KEY_7: return KeyCode::Num7;
        case GLFW_KEY_8: return KeyCode::Num8; case GLFW_KEY_9: return KeyCode::Num9;
        case GLFW_KEY_SPACE: return KeyCode::Space;
        case GLFW_KEY_ENTER: return KeyCode::Enter;
        case GLFW_KEY_ESCAPE: return KeyCode::Escape;
        case GLFW_KEY_TAB: return KeyCode::Tab;
        case GLFW_KEY_LEFT_SHIFT: return KeyCode::LeftShift;
        case GLFW_KEY_LEFT_CONTROL: return KeyCode::LeftControl;
        case GLFW_KEY_LEFT_ALT: return KeyCode::LeftAlt;
        case GLFW_KEY_RIGHT_SHIFT: return KeyCode::RightShift;
        case GLFW_KEY_RIGHT_CONTROL: return KeyCode::RightControl;
        case GLFW_KEY_RIGHT_ALT: return KeyCode::RightAlt;
        case GLFW_KEY_UP: return KeyCode::Up;
        case GLFW_KEY_DOWN: return KeyCode::Down;
        case GLFW_KEY_LEFT: return KeyCode::Left;
        case GLFW_KEY_RIGHT: return KeyCode::Right;
        default: return KeyCode::Unknown;
    }
}

MouseButton InputManager::translate_glfw_mouse_button(int glfw_mouse_button) {
    switch (glfw_mouse_button) {
        case GLFW_MOUSE_BUTTON_LEFT: return MouseButton::Left;
        case GLFW_MOUSE_BUTTON_RIGHT: return MouseButton::Right;
        case GLFW_MOUSE_BUTTON_MIDDLE: return MouseButton::Middle;
        default: return MouseButton::Unknown;
    }
}

void InputManager::key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (InputManager* manager = static_cast<InputManager*>(glfwGetWindowUserPointer(window))) {
        manager->handle_key(key, action);
    }
}

void InputManager::mouse_button_callback(GLFWwindow *window, int button, int action, int mods) {
    if (InputManager* manager = static_cast<InputManager*>(glfwGetWindowUserPointer(window))) {
        manager->handle_mouse_button(button, action);
    }
}

void InputManager::cursor_position_callback(GLFWwindow *window, double xpos, double ypos) {
    if (InputManager* manager = static_cast<InputManager*>(glfwGetWindowUserPointer(window))) {
        manager->handle_cursor_position(xpos, ypos);
    }
}

void InputManager::scroll_callback(GLFWwindow *window, double x_offset, double y_offset) {
    if (InputManager* manager = static_cast<InputManager*>(glfwGetWindowUserPointer(window))) {
        manager->handle_scroll(x_offset, y_offset);
    }
}

void InputManager::handle_key(int key, int action) {
    KeyCode internal_code = translate_glfw_key(key);
    if (internal_code != KeyCode::Unknown) {
        m_current_key_states[static_cast<int>(internal_code)] = action;
    }
}

void InputManager::handle_mouse_button(int button, int action) {
    MouseButton internal_button = translate_glfw_mouse_button(button);
    if (internal_button != MouseButton::Unknown) {
        m_current_key_states[static_cast<int>(internal_button) + MOUSE_BUTTON_OFFSET] = action;
    }
}

void InputManager::handle_cursor_position(double xpos, double ypos) {
    Vec2 new_pos(static_cast<float>(xpos), static_cast<float>(ypos));
    m_current_mouse_pos = new_pos;
}

void InputManager::handle_scroll(double x_offset, double y_offset) {

}


