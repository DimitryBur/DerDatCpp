#include "PlotWindow.hpp"

void PlotWindow::Render(DataModel& model, AppState& state) {
    static int axisX = 0;
    static int axisY = 0;
    static std::vector<double> dataX, dataY;
    static std::string lastPlotted = "";

    float windowWidth = ImGui::GetContentRegionAvail().x;
    const auto& cols = state.columnList;

    // --- ЛЕВАЯ ПАНЕЛЬ (Конструктор осей) ---
    ImGui::BeginChild("PlotControls", ImVec2(windowWidth * 0.25f, 0), true);
    ImGui::TextColored(ImVec4(0, 1, 1, 1), "КОНСТРУКТОР ОСЕЙ");
    ImGui::Separator();

    ImGui::Text("Ось X:");
    ImGui::SetNextItemWidth(-1);
    if (ImGui::BeginCombo("##x_axis", cols.empty() ? "-" : cols[axisX].c_str())) {
        for (int i = 0; i < (int)cols.size(); i++) if (ImGui::Selectable(cols[i].c_str(), axisX == i)) axisX = i;
        ImGui::EndCombo();
    }

    ImGui::Spacing();

    ImGui::Text("Ось Y:");
    ImGui::SetNextItemWidth(-1);
    if (ImGui::BeginCombo("##y_axis", cols.empty() ? "-" : cols[axisY].c_str())) {
        for (int i = 0; i < (int)cols.size(); i++) if (ImGui::Selectable(cols[i].c_str(), axisY == i)) axisY = i;
        ImGui::EndCombo();
    }

    ImGui::Spacing();
    
    // Кнопка построения (выкачиваем данные из DuckDB)
    if (ImGui::Button(" ПОСТРОИТЬ ", ImVec2(-1, 40)) && !state.activeTable.empty() && !cols.empty()) {
        dataX.clear(); dataY.clear();
        std::string sql = "SELECT \"" + cols[axisX] + "\", \"" + cols[axisY] + "\" FROM \"" + state.activeTable + "\" WHERE \"" + cols[axisY] + "\" IS NOT NULL LIMIT 10000";
        auto res = model.RunQuery(sql);
        while (auto chunk = res->Fetch()) {
            for (size_t i = 0; i < chunk->size(); i++) {
                dataX.push_back(chunk->GetValue(0, i).GetValue<double>());
                dataY.push_back(chunk->GetValue(1, i).GetValue<double>());
            }
        }
        lastPlotted = cols[axisY] + " vs " + cols[axisX];
    }
    ImGui::EndChild();

    ImGui::SameLine();

    // --- ПРАВАЯ ЧАСТЬ (Холст ImPlot) ---
    ImGui::BeginChild("PlotCanvas", ImVec2(0, 0), true);
    if (ImPlot::BeginPlot("Визуализация данных", ImVec2(-1, -1))) {
        ImPlot::SetupAxes(cols.empty() ? "X" : cols[axisX].c_str(), cols.empty() ? "Y" : cols[axisY].c_str());
        
        if (!dataX.empty()) {
            // Тип графика можно менять (Line, Scatter и т.д.)
            ImPlot::PlotLine(lastPlotted.c_str(), dataX.data(), dataY.data(), (int)dataX.size());
            ImPlot::PlotScatter("Точки", dataX.data(), dataY.data(), (int)dataX.size());
        }
        ImPlot::EndPlot();
    }
    ImGui::EndChild();
}
