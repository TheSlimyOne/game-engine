#ifndef GAME_PROFILERPANEL_H
#define GAME_PROFILERPANEL_H

class ProfilerPanel {
public:
    ProfilerPanel() = default;
    void render();

private:
    bool  m_paused      = false;
    float m_graph_max   = 20.0f; // ms range for graphs
};

#endif //GAME_PROFILERPANEL_H