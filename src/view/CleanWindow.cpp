#include "CleanWindow.hpp"
#include <string>
#include <vector>

void CleanWindow::Render(DataModel& model, AppState& state) {
    float windowHeight = ImGui::GetContentRegionAvail().y;
    auto& db_con = model.GetConnection();

    // --- ВЕРХ (70%): Таблица-превью с подсветкой активной колонки ---
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
                            
                            // ПОДСВЕТКА: Если имя колонки совпадает с выбранной в Header (Combo 2)
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
        ImGui::SetCursorPos(ImVec2(ImGui::GetWindowWidth()/2 - 100, ImGui::GetWindowHeight()/2));
        ImGui::TextDisabled("Выберите таблицу и колонку в Header");
    }
    ImGui::EndChild();

    ImGui::Separator();

    // --- ПАНЕЛЬ МЕТАДАННЫХ (Как в окне ИМПОРТ) ---
    ImGui::Separator();
    if (!state.activeTable.empty()) {
    // Получаем данные напрямую из модели
    long long rows = model.GetRowCount(state.activeTable);
    int cols = model.GetColCount(state.activeTable);

    ImGui::Text("Таблица: "); ImGui::SameLine();
    ImGui::TextColored(ImVec4(0, 1, 1, 1), "%s", state.activeTable.c_str()); 
    
    ImGui::SameLine(300); // Отступ для строк
    ImGui::Text("Строк: "); ImGui::SameLine();
    ImGui::TextColored(ImVec4(0, 1, 0, 1), "%lld", rows); 
    
    ImGui::SameLine(500); // Отступ для колонок
    ImGui::Text("Колонок: "); ImGui::SameLine();
    ImGui::TextColored(ImVec4(0, 1, 0, 1), "%d", cols);
    } 
    else {
    ImGui::TextDisabled("Выберите таблицу для отображения метаданных");
    }
    ImGui::Separator();
    
    // Далее идет ImGui::BeginChild("ToolsArea");


    // --- НИЗ (30%): Панель инструментов «Кнопка + Список» ---
    ImGui::BeginChild("ToolsArea");
    if (!state.activeColumn.empty()) {
        ImGui::Columns(3, "CleanColumns", false);

        // 1. СТРУКТУРА
        ImGui::TextColored(ImVec4(0, 1, 1, 1), "СТРУКТУРА");
        if (ImGui::Button("УДАЛИТЬ NULL", ImVec2(-1, 30))) {
            model.RemoveNulls(state.activeTable, state.activeColumn);
        }
        if (ImGui::Button("УДАЛИТЬ КОЛОНКУ", ImVec2(-1, 30))) {
            if (model.DropColumn(state.activeTable, state.activeColumn)) {
        state.refreshColumnList(db_con); // Обновляем список в Header
        state.activeColumn = "";         // Сбрасываем фокус
            }
        }

        ImGui::NextColumn();

        // 2. ПАРСИНГ / АНОМАЛИИ
        ImGui::TextColored(ImVec4(0, 1, 1, 1), "ПАРСИНГ И КАЧЕСТВО");
        if (ImGui::Button("ИЗВЛЕЧЬ ЧИСЛО", ImVec2(-1, 30))) {
            model.ExtractNumbers(state.activeTable, state.activeColumn);
        }
        static int outlierIdx = 0; // 0: 5%, 1: 10%, 2: 25%
        const char* outlierLabels[] = { "Отсечь 5%", "Отсечь 10%", "Отсечь 25%" };
        double outlierValues[] = { 0.05, 0.10, 0.25 };

        if (ImGui::Button("УДАЛИТЬ ВЫБРОСЫ", ImVec2(-1, 30))) {
        model.RemoveOutliers(state.activeTable, state.activeColumn, outlierValues[outlierIdx]);
        }
        ImGui::SetNextItemWidth(-1);
        ImGui::Combo("##outlier_p", &outlierIdx, outlierLabels, IM_ARRAYSIZE(outlierLabels));

        ImGui::NextColumn();

        // 3. ЗАПОЛНЕНИЕ (Кнопка + Параметры)
        ImGui::TextColored(ImVec4(0, 1, 1, 1), "ЗАПОЛНЕНИЕ");
        
        static int fillModeIdx = 0; // MEDIAN, CONSTANT, MODA, SMART
        static char constVal[64] = "0";
        static int selColB = 0;

        ImGui::SetNextItemWidth(-1);
        ImGui::Combo("##fill_mode", &fillModeIdx, "MEDIAN\0CONSTANT\0MODA\0SMART\0");

        // Динамический ввод параметров
        if (fillModeIdx == 1) { // CONSTANT
            ImGui::SetNextItemWidth(-1);
            ImGui::InputText("##c_input", constVal, IM_ARRAYSIZE(constVal));
        }
        if (fillModeIdx == 3) { // SMART
            ImGui::SetNextItemWidth(-1);
            if (ImGui::BeginCombo("##s_col", state.columnList.empty() ? "-" : state.columnList[selColB].c_str())) {
                for (int i = 0; i < (int)state.columnList.size(); i++) {
                    if (state.columnList[i] == state.activeColumn) continue; // Не сравнивать с собой
                    if (ImGui::Selectable(state.columnList[i].c_str(), selColB == i)) selColB = i;
                }
                ImGui::EndCombo();
            }
        }

        if (ImGui::Button(" ВЫПОЛНИТЬ ", ImVec2(-1, 35))) {
            FillMode mode = static_cast<FillMode>(fillModeIdx);
            std::string parameter = "";
            
            if (mode == FillMode::CONSTANT) parameter = std::string(constVal);
            if (mode == FillMode::SMART && !state.columnList.empty()) parameter = state.columnList[selColB];

            model.CleanData(state.activeTable, state.activeColumn, mode, parameter);
        }

        ImGui::Columns(1);
    } else {
        ImGui::SetCursorPosX(20);
        ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), "ВНИМАНИЕ: Выберите колонку в Header для работы инструментов.");
    }
    ImGui::EndChild();
}
