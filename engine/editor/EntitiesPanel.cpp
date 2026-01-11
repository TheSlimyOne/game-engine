// EntitiesPanel.cpp

#include "EntitiesPanel.h"
#include <imgui.h>

EntitiesPanel::EntitiesPanel() {}

void EntitiesPanel::render() {
    ImVec2 size = ImVec2(300, 400);
    ImVec2 pos = ImVec2(0, 0);

    ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(size, ImGuiCond_Always);

    ImGui::Begin("World", nullptr,
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoBringToFrontOnFocus
    );

    ImGui::AlignTextToFramePadding();
    ImGui::Text("View:");
    ImGui::SameLine();
    if (ImGui::RadioButton("List", !m_show_hierarchy)) {
        m_show_hierarchy = false;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Hierarchy", m_show_hierarchy)) {
        m_show_hierarchy = true;
    }
    ImGui::SameLine(0, 16);

    if (ImGui::Button("Add")) {

    }


    ImGui::Separator();

    if (m_show_hierarchy) {
        render_hierarchy_view();
    } else {
        render_list_view();
    }

    ImGui::End();
}

void EntitiesPanel::render_list_view() {

}

void EntitiesPanel::render_hierarchy_view() {

}



