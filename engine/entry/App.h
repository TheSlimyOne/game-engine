// App.h

#ifndef GAME_APP_H
#define GAME_APP_H

#include <memory>

#include <platform/Window.h>
#include <core/World.h>
#include <core/Renderer.h>
#include <core/InputManager.h>


class App {
public:
    App(const std::shared_ptr<World> &world);

    void init();
    void tick();
    bool should_close();
private:
    void shutdown();

    bool m_initialized = false;
    float m_accumulator = 0.0;
    const float FIXED_DT = 1.0 / 144.0;

    // Pointers to engine subsystems
    std::shared_ptr<Window> m_window;
    std::unique_ptr<Renderer> m_renderer;
    std::unique_ptr<InputManager> m_input_manager;
    std::shared_ptr<World> m_world;
};

#endif //GAME_APP_H