#ifndef GAME_EDITOR_H
#define GAME_EDITOR_H

#include <memory>

class Window;

class PropertiesPanel;
class EntitiesPanel;
class ProfilerPanel;

class Editor {
public:
    Editor();
    ~Editor();

    void init();
    void tick();
    bool should_close();
private:
    void shutdown();
    bool m_initialized = false;

    // Pointers to engine subsystems
    std::unique_ptr<Window> m_window;
    std::unique_ptr<PropertiesPanel> m_properties_panel;
    std::unique_ptr<EntitiesPanel> m_entities_panel;
    std::unique_ptr<ProfilerPanel> m_profiler_panel;
};

#endif //GAME_EDITOR_H