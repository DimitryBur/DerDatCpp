#pragma once
#include <imgui.h>
#include <nfd.hpp>
#include <string>
#include <vector>
#include "../model/DataModel.hpp"

class ImportWindow {
public:
    static void Render(DataModel& model, std::string& activeTable) {
        static int selU = 0; 
        static int selJ = 0;
        float totalHeight = ImGui::GetContentRegionAvail().y;
        auto tables = model.GetTableNames();

        // 1. ВЕРХ: Список файлов (Фикс 100px)
        ImGui::BeginChild("FilesList", ImVec2(0, 100), true);
        if (ImGui::BeginTable("TGrid", 1, ImGuiTableFlags_Borders)) {
            for (const auto& t : tables) {
                ImGui::TableNextRow(); ImGui::TableSetColumnIndex(0);
                if (ImGui::Selectable(t.c_str(), activeTable == t)) activeTable = t;
            }
            ImGui::EndTable();
        }
        ImGui::EndChild();

        // 2. ЦЕНТР: Превью (Фикс высота: остаток минус место под стат и кнопки)
        ImGui::BeginChild("PreviewArea", ImVec2(0, totalHeight - 220), true);
        if (!activeTable.empty()) {
            auto res = model.RunQuery("SELECT * FROM \"" + activeTable + "\" LIMIT 50");
            if (res && !res->HasError()) {
                int cols = (int)res->ColumnCount();
                if (ImGui::BeginTable("PGrid", cols, ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY)) {
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
        }
        ImGui::EndChild();

        // 3. ПАНЕЛЬ СТАТИСТИКИ (Над кнопками)
        ImGui::Separator();
        if (!activeTable.empty()) {
            // Выводим кол-во строк и колонок в одну строку
            ImGui::Text("Таблица: "); ImGui::SameLine();
            ImGui::TextColored(ImVec4(0, 1, 1, 1), "%s", activeTable.c_str()); ImGui::SameLine(250);
            ImGui::Text("Строк: "); ImGui::SameLine();
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "%lld", model.GetRowCount(activeTable)); ImGui::SameLine(400);
            ImGui::Text("Колонок: "); ImGui::SameLine();
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "%d", model.GetColCount(activeTable));
        } else {
            ImGui::TextDisabled("Нет активных данных для анализа");
        }
        ImGui::Separator();

        // 4. НИЗ: Кнопки операций (Фикс положение)
        ImGui::Spacing();
        
        // Группа ФАЙЛЫ
        ImGui::BeginGroup();
            if (ImGui::Button(" ИМПОРТ ", ImVec2(90, 30))) {
                NFD::UniquePathU8 p; nfdu8filteritem_t f = {"CSV", "csv"};
                if (NFD::OpenDialog(p, &f, 1) == NFD_OKAY) {
                    std::string name = p.get();
                    name = name.substr(name.find_last_of("/\\") + 1);
                    name = name.substr(0, name.find_last_of("."));
                    model.ImportCSV(name, p.get()); activeTable = name;
                }
            }
            ImGui::SameLine();
            if (ImGui::Button(" ЭКСПОРТ ", ImVec2(90, 30)) && !activeTable.empty()) {
                NFD::UniquePathU8 p; nfdu8filteritem_t f = {"CSV", "csv"};
                if (NFD::SaveDialog(p, &f, 1, nullptr, (activeTable + ".csv").c_str()) == NFD_OKAY)
                    model.RunQuery("COPY \"" + activeTable + "\" TO '" + std::string(p.get()) + "' (HEADER, DELIMITER ',')");
            }
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.2f, 0.2f, 1.0f));
            if (ImGui::Button(" УДАЛИТЬ ", ImVec2(90, 30)) && !activeTable.empty()) {
                if(model.DeleteTable(activeTable)) activeTable = "";
            }
            ImGui::PopStyleColor();
        ImGui::EndGroup();

        ImGui::SameLine(330);

        // Группа UNION (Кнопка над Списком)
        ImGui::BeginGroup();
            if (ImGui::Button(" UNION ", ImVec2(120, 30)) && !activeTable.empty()) {
                if (selU < tables.size() && tables[selU] != activeTable) {
                    std::string resN = activeTable + "_u_" + tables[selU];
                    if (model.UnionTables(activeTable, tables[selU], resN)) activeTable = resN;
                }
            }
            ImGui::SetNextItemWidth(120);
            if (ImGui::BeginCombo("##u", tables.empty() ? "..." : tables[selU].c_str())) {
                for (int i = 0; i < tables.size(); i++) {
                    if (tables[i] == activeTable) continue;
                    if (ImGui::Selectable(tables[i].c_str(), selU == i)) selU = i;
                }
                ImGui::EndCombo();
            }
        ImGui::EndGroup();

        ImGui::SameLine();

        // Группа JOIN (Кнопка над Списком)
        ImGui::BeginGroup();
            if (ImGui::Button(" JOIN ", ImVec2(120, 30))) { /* logic later */ }
            ImGui::SetNextItemWidth(120);
            ImGui::Combo("##j", &selJ, "Ключ\0\0");
        ImGui::EndGroup();
    }
};
