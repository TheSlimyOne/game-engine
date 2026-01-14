// App.cpp

#include "App.h"

#include <core/Systems.h>
#include <iostream>
#include <profiler/Profiler.h>

#include "../../game/ComputeTest.h"

App::App(const std::shared_ptr<World> &world)
    : m_world(world)
{}

void App::init() {
    std::cout << "Initializing App..." << std::endl;

    // Create the window
    m_window = std::make_shared<Window>();
    if (!m_window->init("BGFX Engine", 1920, 1080, false)) {
        std::cerr << "Failed to create window." << std::endl;
        return;
    }

    // Create renderer
    m_renderer = std::make_unique<Renderer>();
    if (!m_renderer->init(m_window.get())) {
        // Handle initialization error
        std::cerr << "Failed to initialize renderer" << std::endl;
        return;
    }

    // Attach window to world's input manager
    m_world->set_input_manager_window(m_window);

    // Finalize and lock in the systems
    Systems::finalize();

    // Run system startups
    Systems::fire_startup(*m_world);

    // Execute buffered mutate commands for the world from startup
    m_world->execute_commands();

    std::cout << "App Initialized!" << std::endl;
    m_initialized = true;
}
ComputeTest a;
void App::tick() {
    //Initialize the app
    if (!m_initialized) {
        std::cerr << "App is not initialized. Cannot start up." << std::endl;
        return;
    }

    Profiler::instance().new_frame();
    PROFILE_SCOPE_ORDER("App Tick", 1);

    // 1. Query input actions
    {
        PROFILE_SCOPE_ORDER("Query Inputs", 2);
        m_world->query_inputs();
    }

    // 2. Calculate fixed time intervals
    float frame_time = m_renderer->get_delta_time(); // Get raw messy time
    Profiler::instance().add_frame_time(frame_time); // Record accurate timing for FPS

    if (frame_time > 0.25f) frame_time = 0.25f; // Cap it to prevent "Spiral of Death" if the game freezes
    m_accumulator += frame_time; // Add to time bucket

    {
        PROFILE_SCOPE_ORDER("Frame Catchup", 3);

        int fixedStepsThisFrame = 0;

        // Eat time from the bucket in fixed bites
        // This while loop ensures logic ALWAYS runs at exactly FIXED_DT
        while (m_accumulator >= FIXED_DT) {
            ++fixedStepsThisFrame;
            // 3. Run game logic: execute "OnUpdate" callbacks
            // Pass the FIXED time to your update, not the variable frame_time
            {
                PROFILE_SCOPE_ORDER("Fire Updates", 4);
                Systems::fire_update(*m_world, static_cast<float>(FIXED_DT));
            }

            // 4. Execute buffered mutate commands for the world
            {
                PROFILE_SCOPE_ORDER("Execute Commands", 5);
                m_world->execute_commands();
            }

            // 5. Resolve the hierarchy
            {
                PROFILE_SCOPE_ORDER("Resolve Hierarchy Transforms", 6);
                m_world->update_transforms();
            }

            m_accumulator -= FIXED_DT;
        }

        // After the loop, record the *count* as a profiler value
        Profiler::instance().set_fixed_step_stats(
            fixedStepsThisFrame,
            FIXED_DT
        );
    }

    a.Update();

    // 6. Draw the frame
    {
        PROFILE_SCOPE_ORDER("Draw Frame", 7);
        m_renderer->begin_frame();
        m_renderer->draw_frame(*m_world);
        m_renderer->end_frame(*m_world);
    }
}

bool App::should_close() {
    return m_window->should_close();
}


void App::shutdown() {
    // Shutdown subsystems in reverse order of initialization
    std::cout << "Shutting Down App..." << std::endl;
    if (m_window) {
        m_window->shutdown();
    }
}


