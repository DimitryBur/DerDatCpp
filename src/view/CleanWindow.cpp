#include "CleanWindow.hpp"
#include <string>
#include <vector>

void CleanWindow::Render(DataModel& model, AppState& state) {
    float windowHeight = ImGui::GetContentRegionAvail().y;
    auto& db_con = model.GetConnection();
    
    // БУФЕРЫ (Должны быть массивами, а не одиночными символами!)
    static char renameBuf[128] = "";
    static char constVal[128] = "0";
    static int fillModeIdx = 0;
    static int selColB = 0;
    static int outlierIdx = 0;

    // --- ВЕРХ (70%): Таблица-превью ---
    ImGui::BeginChild("CleanPreview", ImVec2(0, windowHeight * 0.7f), true);
    if (!state.activeTable.empty()) {
        auto res = model.RunQuery("SELECT * FROM \"" + state.activeTable + "\" LIMIT 50");
        if (res && !res->HasError()) {
            int colsCount = (int)res->ColumnCount();
            if (ImGui::BeginTable("CleanTable", colsCount, ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY | ImGuiTableFlags_ScrollX | ImGuiTableFlags_RowBg)) {
                for (int i = 0; i < colsCount; i++) 
                    ImGui::TableSetupColumn(res->ColumnName(i).c_str());
                ImGui::TableHeadersRow();
                while (auto chunk = res->Fetch()) {
                    for (size_t r = 0; r < chunk->size(); r++) {
                        ImGui::TableNextRow();
                        for (int c = 0; c < colsCount; c++) {
                            ImGui::TableSetColumnIndex(c);
                            if (res->ColumnName(c) == state.activeColumn)
                                ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, ImGui::GetColorU32(ImVec4(0.15f, 0.35f, 0.55f, 1.0f)));
                            ImGui::TextUnformatted(chunk->GetValue(c, r).ToString().c_str());
                        }
                    }
                }
                ImGui::EndTable();
            }
        }
    } else {
        ImGui::TextDisabled("Выберите таблицу в Header");
    }
    ImGui::EndChild();

    // --- ПАНЕЛЬ МЕТАДАННЫХ ---
    ImGui::Separator();
    if (!state.activeTable.empty()) {
        ImGui::Text("Таблица: %s", state.activeTable.c_str()); ImGui::SameLine(300);
        ImGui::Text("Строк: %lld", model.GetRowCount(state.activeTable)); ImGui::SameLine(500);
        ImGui::Text("Колонок: %d", model.GetColCount(state.activeTable));
    }
    ImGui::Separator();

    // --- НИЗ (30%): Инструменты ---
    ImGui::BeginChild("ToolsArea", ImVec2(0, 0), false);
    if (!state.activeColumn.empty()) {
        ImGui::Columns(3, "CleanCols", false);

        // КОЛОНКА 1: СТРУКТУРА
        ImGui::TextColored(ImVec4(0, 1, 1, 1), "СТРУКТУРА");
        if (ImGui::Button("УДАЛИТЬ NULL", ImVec2(-1, 30))) model.RemoveNulls(state.activeTable, state.activeColumn);
        if (ImGui::Button("УДАЛИТЬ КОЛОНКУ", ImVec2(-1, 30))) {
            if (model.DropColumn(state.activeTable, state.activeColumn)) {
                state.refreshColumnList(db_con);
                state.activeColumn = "";
            }
        }

        ImGui::NextColumn();

        // КОЛОНКА 2: ТРАНСФОРМАЦИЯ
        ImGui::TextColored(ImVec4(0, 1, 1, 1), "ПЕРЕИМЕНОВАТЬ");
        ImGui::SetNextItemWidth(-1);
        ImGui::InputTextWithHint("##ren", "Новое имя...", renameBuf, 128);
        if (ImGui::Button("ОК", ImVec2(-1, 30)) && strlen(renameBuf) > 0) {
            if (model.RenameColumn(state.activeTable, state.activeColumn, renameBuf)) {
                state.refreshColumnList(db_con);
                state.activeColumn = renameBuf;
                renameBuf[0] = '\0';
            }
        }
        ImGui::TextColored(ImVec4(0, 1, 1, 1), "ИЗВЛЕЧЬ ЧИСЛО");
        if (ImGui::Button("DOUBLE", ImVec2(ImGui::GetContentRegionAvail().x * 0.5f - 5, 30))) {
            if (model.ExtractNumbers(state.activeTable, state.activeColumn)) {
                state.refreshColumnList(db_con);
                state.activeColumn += "_num";
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("INT", ImVec2(-1, 30))) {
            if (model.ExtractAsInt(state.activeTable, state.activeColumn)) {
                state.refreshColumnList(db_con);
                state.activeColumn += "_int";
            }
        }

        ImGui::NextColumn();

        // КОЛОНКA 3: КАЧЕСТВО
        ImGui::TextColored(ImVec4(0, 1, 1, 1), "ВЫБРОСЫ И NULL");
        const char* labels[] = { "Отсечь 5%", "Отсечь 10%", "Отсечь 25%" };
        double vals[] = { 0.05, 0.10, 0.25 };
        ImGui::SetNextItemWidth(-1);
        ImGui::Combo("##out", &outlierIdx, labels, 3);
        if (ImGui::Button("УДАЛИТЬ ВЫБРОСЫ", ImVec2(-1, 30))) model.RemoveOutliers(state.activeTable, state.activeColumn, vals[outlierIdx]);

        ImGui::Separator();
        ImGui::SetNextItemWidth(-1);
        ImGui::Combo("##fmode", &fillModeIdx, "MEDIAN\0CONSTANT\0MODA\0SMART\0");
        if (fillModeIdx == 1) { ImGui::SetNextItemWidth(-1); ImGui::InputText("##cv", constVal, 128); }
        if (fillModeIdx == 3) {
            ImGui::SetNextItemWidth(-1);
            if (ImGui::BeginCombo("##scol", state.columnList.empty() ? "-" : (selColB < state.columnList.size() ? state.columnList[selColB].c_str() : "..."))) {
                for (int i = 0; i < (int)state.columnList.size(); i++) 
                    if (ImGui::Selectable(state.columnList[i].c_str(), selColB == i)) selColB = i;
                ImGui::EndCombo();
            }
        }
        if (ImGui::Button("ЗАПОЛНИТЬ NULL", ImVec2(-1, 35))) {
            std::string p = (fillModeIdx == 1) ? constVal : (fillModeIdx == 3 && !state.columnList.empty() ? state.columnList[selColB] : "");
            model.CleanData(state.activeTable, state.activeColumn, static_cast<FillMode>(fillModeIdx), p);
        }

        ImGui::Columns(1);
    } else {
        ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), "Выберите колонку в Header");
    }
    ImGui::EndChild();
}
