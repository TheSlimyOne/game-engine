#include "editor/ProfilerPanel.h"
#include <profiler/Profiler.h>   // adjust include path
#include <imgui.h>


void ProfilerPanel::render() {
    ImGuiIO& io = ImGui::GetIO();

    // Fixed panel: 300px wide, full height, just left of Properties
    const float panelWidth = 350.0f;
    ImVec2 size(panelWidth, io.DisplaySize.y);
    ImVec2 pos(io.DisplaySize.x - 2.0f * panelWidth, 0.0f); // Properties is at x - 300

    ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(size, ImGuiCond_Always);

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoBringToFrontOnFocus;

    if (!ImGui::Begin("Profiler", nullptr, flags)) {
        ImGui::End();
        return;
    }

    // Top controls
    ImGui::Checkbox("Pause", &m_paused);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-1.0f); // use remaining width
    ImGui::SliderFloat("Graph Max (ms)", &m_graph_max, 1.0f, 100.0f, "%.1f");

    ImGui::Separator();

    const auto& profiler    = Profiler::instance();
    const auto& sectionsMap = profiler.sections();

    // Show FPS / frame-time info
    FrameStats fs = profiler.frame_stats();
    ImGui::Text("FPS inst   : %.1f", fs.last_fps);
    ImGui::Text("FPS recent : %.1f", fs.recent_avg_fps);  // <-- new, sliding window
    ImGui::Text("FPS session: %.1f", fs.avg_fps);         // lifetime
    ImGui::Text("Frame dt   : %.3f ms", fs.last_dt * 1000.0f);
    ImGui::Separator();

    // --- NEW: fixed-step info (just numbers, no graph) ---
    ImGui::Text("Fixed steps/frame : %d", fs.fixed_steps_last);
    ImGui::Text("Fixed latency     : %.2f ms", fs.fixed_latency_last_ms);
    ImGui::Separator();

    ImGui::Text("Sections: %d", (int)sectionsMap.size());
    ImGui::Text("Frame:   %llu", (unsigned long long)profiler.frame_index());
    ImGui::Separator();

    // Build sorted view of sections (by sort_order, then name)
    std::vector<const Section*> sections;
    sections.reserve(sectionsMap.size());
    for (const auto& kv : sectionsMap) {
        sections.push_back(&kv.second);
    }

    std::sort(sections.begin(), sections.end(),
        [](const Section* a, const Section* b) {
            if (a->sort_order != b->sort_order)
                return a->sort_order < b->sort_order;  // lower sort_order first
            return a->name < b->name;                  // stable tie-breaker
        });

    // Wrap all text to stay within panel width (no horizontal overflow)
    const float wrapX = ImGui::GetCursorPos().x + ImGui::GetContentRegionAvail().x;
    ImGui::PushTextWrapPos(wrapX);

    for (const Section* s : sections) {
        // Section name
        ImGui::Text("%s", s->name.c_str());

        // One metric per line
        ImGui::TextDisabled("last: %.3f ms", s->last_ms);
        ImGui::TextDisabled("avg : %.3f ms", s->avg_ms);
        ImGui::TextDisabled("max : %.3f ms", s->max_ms);

        // Graph — full width of panel, fixed height
        std::string graphId = "##prof_" + s->name;
        ImGui::PlotLines(
            graphId.c_str(),
            s->history.data(),
            (int)PROFILER_HISTORY_SIZE,
            (int)s->write_index,
            nullptr,
            0.0f,
            m_graph_max,
            ImVec2(0.0f, 50.0f) // width 0 = use full content width
        );

        ImGui::Separator();
    }

    ImGui::PopTextWrapPos();
    ImGui::End();
}