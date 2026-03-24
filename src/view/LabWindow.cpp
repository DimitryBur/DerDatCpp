#include "LabWindow.hpp"
#include <imgui_stdlib.h>

void LabWindow::Render(DataModel& model, AppState& state) {
    static std::string sqlEditor = "SELECT * FROM " + (state.activeTable.empty() ? "table_name" : state.activeTable) + " LIMIT 10";
    static std::string lastError = "";
    static std::string newTableName = "query_result";
    
    float windowHeight = ImGui::GetContentRegionAvail().y;
    auto& db_con = model.GetConnection();

    // --- ВЕРХ (70%): SQL Редактор ---
    ImGui::BeginChild("SQLEditor", ImVec2(0, windowHeight * 0.45f), true);
        ImGui::TextColored(ImVec4(0, 1, 0.5f, 1), "DuckDB SQL Console");
        
        // КНОПКА ПОДСКАЗКИ [ ? ]
        ImGui::SameLine(ImGui::GetWindowWidth() - 45);
        if (ImGui::Button(" [ ? ] ")) {
            ImGui::OpenPopup("SQLHelp");
        }
        
        ImGui::SetNextWindowSize(ImVec2(450, 0)); 
        if (ImGui::BeginPopup("SQLHelp")) {
            ImGui::TextColored(ImVec4(1, 1, 0, 1), "DuckDB Syntax Hints:");
            ImGui::Separator();
            ImGui::BulletText("SELECT * FROM table_name LIMIT 100");
            ImGui::BulletText("SELECT col1, SUM(col2) FROM table GROUP BY 1");
            ImGui::BulletText("CREATE TABLE new_t AS SELECT * FROM old_t WHERE...");
            ImGui::BulletText("UPDATE table SET col = val WHERE condition");
            ImGui::BulletText("Math: median(col), quantile_cont(col, 0.95), avg(col)");
            ImGui::BulletText("Cast: CAST(column AS DOUBLE) or column::DOUBLE");
            ImGui::Spacing();
            ImGui::TextDisabled("Подсказка: Используйте кавычки \"Col Name\" для имен с пробелами");
            ImGui::EndPopup();
        }

        // Поле ввода запроса (Живое выполнение)
        if (ImGui::InputTextMultiline("##editor", &sqlEditor, ImVec2(-1, -1), ImGuiInputTextFlags_AllowTabInput)) {
            lastError = ""; // Сбрасываем ошибку при вводе
        }
    ImGui::EndChild();

    // --- ПАНЕЛЬ УПРАВЛЕНИЯ (Только сохранение) ---
    ImGui::Spacing();
    ImGui::Text("Имя новой таблицы:"); ImGui::SameLine();
    ImGui::SetNextItemWidth(250);
    ImGui::InputText("##save_as", &newTableName);
    ImGui::SameLine();
    
    if (ImGui::Button(" [ СОХРАНИТЬ РЕЗУЛЬТАТ ] ", ImVec2(250, 35))) {
        std::string createSql = "CREATE OR REPLACE TABLE \"" + newTableName + "\" AS " + sqlEditor;
        auto res = model.RunQuery(createSql);
        if (res->HasError()) lastError = res->GetError();
        else state.refreshTableList(db_con);
    }

    // Вывод ошибок парсинга SQL
    if (!lastError.empty()) {
        ImGui::TextColored(ImVec4(1, 0.4f, 0.4f, 1), "SQL Error: %s", lastError.c_str());
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
    } else if (res && !sqlEditor.empty()) {
        lastError = res->GetError(); // Захватываем ошибку "на лету"
    }
    ImGui::EndChild();
}
