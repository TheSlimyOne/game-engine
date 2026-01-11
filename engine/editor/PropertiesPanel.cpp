// PropertiesPanel.cpp

#include "PropertiesPanel.h"
#include <imgui.h>

void PropertiesPanel::render() {
    ImVec2 size = ImVec2(300, ImGui::GetIO().DisplaySize.y); // Full height
    ImVec2 pos = ImVec2(ImGui::GetIO().DisplaySize.x - size.x, 0); // Top-right

    ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(size, ImGuiCond_Always);

    ImGui::Begin("Properties", nullptr,
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoBringToFrontOnFocus);

    ImGui::Text("Properties go here!");
    ImGui::End();
}
