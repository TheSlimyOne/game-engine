// Editor.cpp

#include "Editor.h"

#include <platform/Window.h>
#include <editor/PropertiesPanel.h>
#include <editor/EntitiesPanel.h>
#include <editor/ProfilerPanel.h>

#include <iostream>
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>



Editor::Editor() {}

Editor::~Editor() {
    shutdown();
}

void Editor::init() {
    std::cout << "Initializing Editor..." << std::endl;

    // Create the window
    m_window = std::make_unique<Window>();
    if (!m_window->init("BGFX Engine", 1280, 720, true)) {
        std::cerr << "Failed to create window." << std::endl;
        return;
    }

    // Initialize OpengGL Loader
    glfwMakeContextCurrent(m_window->get_glfw_window());
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        fprintf(stderr, "Failed to initialize GLAD\n");
        return;
    }


    // Initialize ImGui Context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void) io;

    // Setup ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer Bindings
    ImGui_ImplGlfw_InitForOpenGL(m_window->get_glfw_window(), true);
    ImGui_ImplOpenGL3_Init("#version 150"); // Or appropriate GLSL version

    // Initialize editor components
    m_properties_panel = std::make_unique<PropertiesPanel>();
    m_entities_panel = std::make_unique<EntitiesPanel>();
    m_profiler_panel = std::make_unique<ProfilerPanel>();

    std::cout << "Editor Initialized!" << std::endl;

    m_initialized = true;
}

void Editor::tick() {
    if (!m_initialized) {
        std::cerr << "Editor is not initialized. Cannot startup." << std::endl;
        return;
    }
    // Process OS messages and user input
    m_window->poll_events();

    // Set opengl context
    glfwMakeContextCurrent(m_window->get_glfw_window());

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Render components
    m_properties_panel->render();
    m_entities_panel->render();
    m_profiler_panel->render();

    // Rendering
    ImGui::Render();
    int32_t width = m_window->get_width();
    int32_t height = m_window->get_height();

    glViewport(0, 0, width, height);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Swap buffers (your window implementation)
    glfwSwapBuffers(m_window->get_glfw_window());
}

bool Editor::should_close() {
    return m_window->should_close();
}


void Editor::shutdown() {
    // Shutdown subsystems in reverse order of initialization
    std::cout << "Shutting Down Editor..." << std::endl;
    if (m_window) {
        m_window->shutdown();
    }

    // Shutdown imgui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}
