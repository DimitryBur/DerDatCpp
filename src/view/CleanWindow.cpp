#pragma once
#include <imgui.h>
#include <string>
#include <vector>
#include "../model/DataModel.hpp"

class CleanWindow {
public:
    static void Render(DataModel& model, std::string& activeTable, std::string& activeCol) {
        float windowHeight = ImGui::GetContentRegionAvail().y;

        // --- ВЕРХ (70%): Таблица-превью ---
        ImGui::BeginChild("CleanPreview", ImVec2(0, windowHeight * 0.7f), true);
        if (!activeTable.empty()) {
            ImGui::Text("Таблица: %s (Колонка: %s)", activeTable.c_str(), activeCol.empty() ? "не выбрана" : activeCol.c_str());
            auto res = model.RunQuery("SELECT * FROM '" + activeTable + "' LIMIT 50");
            if (res && !res->HasError()) {
                if (ImGui::BeginTable("CleanTable", (int)res->ColumnCount(), ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg)) {
                    for (int i = 0; i < (int)res->ColumnCount(); i++) 
                        ImGui::TableSetupColumn(res->ColumnName(i).c_str());
                    ImGui::TableHeadersRow();

                    while (auto chunk = res->Fetch()) {
                        for (duckdb::idx_t r = 0; r < chunk->size(); r++) {
                            ImGui::TableNextRow();
                            for (int c = 0; c < (int)res->ColumnCount(); c++) {
                                ImGui::TableSetColumnIndex(c);
                                if (res->ColumnName(c) == activeCol)
                                    ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, ImGui::GetColorU32(ImVec4(0.1f, 0.3f, 0.5f, 1.0f)));
                                ImGui::TextUnformatted(chunk->GetValue(c, r).ToString().c_str());
                            }
                        }
                    }
                    ImGui::EndTable();
                }
            }
        } else {
            ImGui::SetCursorPos(ImVec2(ImGui::GetWindowWidth()/2 - 100, ImGui::GetWindowHeight()/2));
            ImGui::TextDisabled("Выберите таблицу в Header для просмотра");
        }
        ImGui::EndChild();

        ImGui::Separator();

        // --- НИЗ (30%): Панель кнопок операций (ВСЕГДА ВИДНЫ) ---
        ImGui::BeginChild("ToolsArea");
        
        ImGui::Columns(3, "CleanColumns", false);

        // 1. Структура и Качество
        ImGui::TextColored(ImVec4(1, 1, 0, 1), "СТРУКТУРА / КАЧЕСТВО");
        if (ImGui::Button("УДАЛИТЬ NULL", ImVec2(-1, 30)) && !activeCol.empty()) {
            model.RunQuery("DELETE FROM '" + activeTable + "' WHERE \"" + activeCol + "\" IS NULL");
        }
        if (ImGui::Button("УДАЛИТЬ КОЛОНКУ", ImVec2(-1, 30)) && !activeCol.empty()) {
            model.RunQuery("ALTER TABLE '" + activeTable + "' DROP COLUMN \"" + activeCol + "\"");
            activeCol = "";
        }

        ImGui::NextColumn();

        // 2. Парсинг и Трансформация
        ImGui::TextColored(ImVec4(1, 1, 0, 1), "ТРАНСФОРМАЦИЯ");
        if (ImGui::Button("ИЗВЛЕЧЬ ЧИСЛО", ImVec2(-1, 30)) && !activeCol.empty()) {
            model.RunQuery("UPDATE '" + activeTable + "' SET \"" + activeCol + "\" = regexp_extract(\"" + activeCol + "\", '([0-9]+[.,]?[0-9]*)')");
        }
        if (ImGui::Button("ПЕРЕИМЕНОВАТЬ", ImVec2(-1, 30)) && !activeCol.empty()) {
             // Тут можно вызвать модальное окно ввода имени
        }

        ImGui::NextColumn();

        // 3. Заполнение и Выбросы
        ImGui::TextColored(ImVec4(1, 1, 0, 1), "АНОМАЛИИ");
        static float threshold = 10.0f;
        ImGui::SetNextItemWidth(-1);
        ImGui::DragFloat("##trash", &threshold, 0.5f, 0.0f, 50.0f, "Порог: %.1f%%");
        
        if (ImGui::Button("УДАЛИТЬ ВЫБРОСЫ", ImVec2(-1, 30)) && !activeCol.empty()) {
            std::string q = std::to_string(1.0f - threshold / 100.0f);
            model.RunQuery("DELETE FROM '" + activeTable + "' WHERE \"" + activeCol + "\" > (SELECT quantile(\"" + activeCol + "\", " + q + ") FROM '" + activeTable + "')");
        }
        
        static int fillMode = 0;
        ImGui::SetNextItemWidth(-1);
        ImGui::Combo("##fill", &fillMode, "MEDIAN\0CONSTANT\0MODA\0SMART\0");
        if (ImGui::Button("ЗАПОЛНИТЬ ПУСТОТЫ", ImVec2(-1, 30)) && !activeCol.empty()) {
            // Логика заполнения в зависимости от fillMode
        }

        ImGui::EndChild();
    }
};
