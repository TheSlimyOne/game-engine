#include "Window.h"
#include <iostream>

#include <bgfx/platform.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

// Include the platform-specific native window handle header
#if BX_PLATFORM_LINUX
#define GLFW_EXPOSE_NATIVE_X11
#elif BX_PLATFORM_WINDOWS
#define GLFW_EXPOSE_NATIVE_WIN32
#elif BX_PLATFORM_OSX
#define GLFW_EXPOSE_NATIVE_COCOA
#endif
#include <GLFW/glfw3native.h>

bool Window::m_glfw_initialized = false;

Window::Window()
    :   m_handle(nullptr),
        m_width(0),
        m_height(0),
        m_mouse_x(0),
        m_mouse_y(0),
        m_mouse_scroll(0),
        m_last_char(0)
{}

Window::~Window() {
    // Shutdown is called here to ensure cleanup even if not explicitly called
    shutdown();
}

bool Window::init(const char* title, uint32_t width, uint32_t height, bool opengl_context = false) {
    if (!m_glfw_initialized) {
        if (!glfwInit()) {
            std::cerr << "ERROR: Failed to initialize GLFW!" << std::endl;
            return false;
        }
        m_glfw_initialized = true;
    }

    // Conditionaly tell GLFW to make an OpenGL context. If true, the window will be used for the editor.
    // If false, the window will be used for rendering the main game scene with BGFX.
    if (opengl_context) {
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    } else {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    }

    m_handle = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!m_handle) {
        std::cerr << "ERROR: Failed to create GLFW window!" << std::endl;
        glfwTerminate();
        return false;
    }

    // Set this class instance for callbacks
    glfwSetWindowUserPointer(m_handle, this);

    // Set callbacks
    glfwSetCursorPosCallback(m_handle, Window::cursor_position_callback);
    glfwSetMouseButtonCallback(m_handle, Window::mouse_button_callback);
    glfwSetScrollCallback(m_handle, Window::scroll_callback);
    glfwSetCharCallback(m_handle, Window::char_callback);

    // Init width and height
    m_width = width;
    m_height = height;

    return true;
}

void Window::shutdown() {
    if (m_handle) {
        glfwDestroyWindow(m_handle);
        m_handle = nullptr;
    }
    glfwTerminate();
}

bool Window::should_close() {
    return glfwWindowShouldClose(m_handle);
}

void Window::poll_events() {
    glfwPollEvents();
}

void* Window::get_native_handle() {
#if BX_PLATFORM_WINDOWS
    return glfwGetWin32Window(m_handle);
#elif BX_PLATFORM_LINUX || BX_PLATFORM_BSD
    return (void*)(uintptr_t)glfwGetX11Window(m_handle);
#elif BX_PLATFORM_OSX
    return glfwGetCocoaWindow(m_handle);
#else
    return nullptr;
#endif
}

int32_t Window::get_width() {
    int width, height;
    glfwGetFramebufferSize(m_handle, &width, &height);
    m_width = width;
    return m_width;
}
int32_t Window::get_height() {
    int width, height;
    glfwGetFramebufferSize(m_handle, &width, &height);
    m_height = height;
    return m_height;
}
int32_t Window::get_mouse_x() { return m_mouse_x; }
int32_t Window::get_mouse_y() { return m_mouse_y; }
uint8_t Window::get_mouse_buttons() { return m_mouse_buttons; }
int32_t Window::get_mouse_scroll() { int s = m_mouse_scroll; m_mouse_scroll = 0; return s; }
int32_t Window::get_last_char() { int c = m_last_char; m_last_char = -1; return c; }


void Window::cursor_position_callback(GLFWwindow *window, double xpos, double ypos) {
    Window* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
    self->m_mouse_x = static_cast<int32_t>(xpos);
    self->m_mouse_y = static_cast<int32_t>(ypos);
}

void Window::mouse_button_callback(GLFWwindow* window, int button, int action, int /*mods*/) {
    Window* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (button == GLFW_MOUSE_BUTTON_LEFT)
        self->m_mouse_buttons = action == GLFW_PRESS ? (self->m_mouse_buttons | 0x01) : (self->m_mouse_buttons & ~0x01);
    if (button == GLFW_MOUSE_BUTTON_RIGHT)
        self->m_mouse_buttons = action == GLFW_PRESS ? (self->m_mouse_buttons | 0x02) : (self->m_mouse_buttons & ~0x02);
    if (button == GLFW_MOUSE_BUTTON_MIDDLE)
        self->m_mouse_buttons = action == GLFW_PRESS ? (self->m_mouse_buttons | 0x04) : (self->m_mouse_buttons & ~0x04);
}

void Window::scroll_callback(GLFWwindow* window, double /*xoffset*/, double yoffset) {
    Window* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
    self->m_mouse_scroll += static_cast<int32_t>(yoffset);
}

void Window::char_callback(GLFWwindow* window, unsigned int codepoint) {
    Window* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
    self->m_last_char = static_cast<int32_t>(codepoint);
}