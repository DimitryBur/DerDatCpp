#include "PlotWindow.hpp"
#include <vector>

void PlotWindow::Render(DataModel& model, AppState& state) {
    static int axisX = 0;
    static int axisY = 0;
    static int plotType = 0; // 0: Line, 1: Scatter, 2: Bar
    static std::vector<double> dataX, dataY;
    static std::string lastPlotted = "";

    float windowWidth = ImGui::GetContentRegionAvail().x;
    const auto& cols = state.columnList;

    // Защита от вылета при смене таблицы
    if (axisX >= (int)cols.size()) axisX = 0;
    if (axisY >= (int)cols.size()) axisY = 0;

    // --- ЛЕВАЯ ПАНЕЛЬ (Конструктор осей) ---
    ImGui::BeginChild("PlotControls", ImVec2(windowWidth * 0.25f, 0), true);
        ImGui::TextColored(ImVec4(0, 1, 1, 1), "КОНСТРУКТОР ОСЕЙ");
        ImGui::Separator();

        ImGui::Text("Ось X:");
        ImGui::SetNextItemWidth(-1);
        if (ImGui::BeginCombo("##x_axis", cols.empty() ? "-" : cols[axisX].c_str())) {
            for (int i = 0; i < (int)cols.size(); i++) 
                if (ImGui::Selectable(cols[i].c_str(), axisX == i)) axisX = i;
            ImGui::EndCombo();
        }

        ImGui::Spacing();

        ImGui::Text("Ось Y:");
        ImGui::SetNextItemWidth(-1);
        if (ImGui::BeginCombo("##y_axis", cols.empty() ? "-" : cols[axisY].c_str())) {
            for (int i = 0; i < (int)cols.size(); i++) 
                if (ImGui::Selectable(cols[i].c_str(), axisY == i)) axisY = i;
            ImGui::EndCombo();
        }

        ImGui::Spacing();

        // ВЫБОР ТИПА ГРАФИКА
        ImGui::Text("Тип графика:");
        ImGui::SetNextItemWidth(-1);
        ImGui::Combo("##plot_type", &plotType, "Линии (Line)\0Точки (Scatter)\0Столбцы (Bar)\0");

        ImGui::Spacing();
        
        if (ImGui::Button(" ПОСТРОИТЬ ", ImVec2(-1, 40)) && !state.activeTable.empty() && !cols.empty()) {
            dataX.clear(); dataY.clear();
            std::string sql = "SELECT TRY_CAST(\"" + cols[axisX] + "\" AS DOUBLE), "
                              "TRY_CAST(\"" + cols[axisY] + "\" AS DOUBLE) "
                              "FROM \"" + state.activeTable + "\" "
                              "WHERE \"" + cols[axisX] + "\" IS NOT NULL LIMIT 5000";
            
            auto res = model.RunQuery(sql);
            if (res && !res->HasError()) {
                while (auto chunk = res->Fetch()) {
                    for (size_t i = 0; i < chunk->size(); i++) {
                        dataX.push_back(chunk->GetValue(0, i).GetValue<double>());
                        dataY.push_back(chunk->GetValue(1, i).GetValue<double>());
                    }
                }
                lastPlotted = cols[axisY] + " / " + cols[axisX];
            }
        }
    ImGui::EndChild();

    ImGui::SameLine();

    // --- ПРАВАЯ ЧАСТЬ (Холст ImPlot) ---
    ImGui::BeginChild("PlotCanvas", ImVec2(0, 0), true);
        if (ImPlot::BeginPlot("##Visual", ImVec2(-1, -1))) {
            ImPlot::SetupAxes(cols.empty() ? "X" : cols[axisX].c_str(), 
                              cols.empty() ? "Y" : cols[axisY].c_str());
            
            if (!dataX.empty()) {
                // Отрисовка в зависимости от выбора
                switch (plotType) {
                    case 0: ImPlot::PlotLine(lastPlotted.c_str(), dataX.data(), dataY.data(), (int)dataX.size()); break;
                    case 1: ImPlot::PlotScatter(lastPlotted.c_str(), dataX.data(), dataY.data(), (int)dataX.size()); break;
                    case 2: ImPlot::PlotBars(lastPlotted.c_str(), dataX.data(), dataY.data(), (int)dataX.size(),0.67); break;
                }
            }
            ImPlot::EndPlot();
        }
    ImGui::EndChild();
}
