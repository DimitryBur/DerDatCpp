#include "LabWindow.hpp"
#include <imgui_stdlib.h>

void LabWindow::Render(DataModel& model, AppState& state) {
    static std::string sqlEditor = "SELECT * FROM active_table LIMIT 10";
    static std::string lastError = "";
    static std::string newTableName = "query_result";
    float windowHeight = ImGui::GetContentRegionAvail().y;
    auto& db_con = model.GetConnection();

    // --- ВЕРХ (70%): SQL Редактор ---
    ImGui::BeginChild("SQLEditor", ImVec2(0, windowHeight * 0.4f), true);
    ImGui::TextColored(ImVec4(0, 1, 0.5f, 1), "DuckDB SQL Console");
    ImGui::SameLine(ImGui::GetWindowWidth() - 40);
    if (ImGui::Button(" ? ")) {
        ImGui::OpenPopup("SQLHelp");
    }

    // Подсказка по синтаксису (из твоего паспорта)
    if (ImGui::BeginPopup("SQLHelp")) {
        ImGui::Text("DuckDB Hints:");
        ImGui::BulletText("SELECT * FROM table_name");
        ImGui::BulletText("SUM, AVG, MEDIAN, QUANTILE");
        ImGui::BulletText("GROUP BY, ORDER BY");
        ImGui::EndPopup();
    }

    // Поле ввода запроса
    ImGui::InputTextMultiline("##editor", &sqlEditor, ImVec2(-1, -1), ImGuiInputTextFlags_AllowTabInput);
    ImGui::EndChild();

    // Панель управления между окнами
    if (ImGui::Button(" [ ВЫПОЛНИТЬ ] ", ImVec2(150, 35))) {
        lastError = "";
        // Просто проверяем запрос через RunQuery
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(200);
    ImGui::InputText("##save_as", &newTableName);
    ImGui::SameLine();
    if (ImGui::Button(" [ СОХРАНИТЬ ] ", ImVec2(150, 35))) {
        std::string createSql = "CREATE OR REPLACE TABLE \"" + newTableName + "\" AS " + sqlEditor;
        auto res = model.RunQuery(createSql);
        if (res->HasError()) lastError = res->GetError();
        else state.refreshTableList(db_con);
    }

    if (!lastError.empty()) {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "Error: %s", lastError.c_str());
    }

    ImGui::Separator();

    // --- НИЗ (Остаток): Таблица результата ---
    ImGui::BeginChild("SQLResult", ImVec2(0, 0), true);
    auto res = model.RunQuery(sqlEditor);
    if (res && !res->HasError()) {
        int cols = (int)res->ColumnCount();
        if (ImGui::BeginTable("ResultTable", cols, ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg)) {
            for (int i = 0; i < cols; i++) ImGui::TableSetupColumn(res->ColumnName(i).c_str());
            ImGui::TableHeadersRow();
            while (auto chunk = res->Fetch()) {
                for (size_t r = 0; r < chunk->size(); r++) {
                    ImGui::TableNextRow();
                    for (int c = 0; c < cols; c++) {
                        ImGui::TableSetColumnIndex(c);
                        ImGui::TextUnformatted(chunk->GetValue(c, r).ToString().c_str());
                    }
                }
            }
            ImGui::EndTable();
        }
    }
    ImGui::EndChild();
}
