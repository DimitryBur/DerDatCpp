#include "ImportWindow.hpp"
#include <nfd.hpp>

void ImportWindow::Render(DataModel& model, AppState& state) {
    static int selU = 0; // Индекс таблицы Б для UNION
    static int selJ = 0; // Индекс таблицы Б для JOIN
    static int keyA = 0; // Ключ таблицы А
    static int keyB = 0; // Ключ таблицы Б
    static int keyIdxA = 0; // Индекс ключа Таблицы А
    static int keyIdxB = 0; // Индекс ключа Таблицы Б
    static int joinMode = 0; // 0 - LEFT, 1 - INNER
    

    float totalHeight = ImGui::GetContentRegionAvail().y;
    auto& db_con = model.GetConnection();
    const auto& tables = state.tableList;

    // 1. ВЕРХ: Список загруженных таблиц (Фикс 100px)
    ImGui::BeginChild("FilesList", ImVec2(0, 100), true);
    if (ImGui::BeginTable("TGrid", 1, ImGuiTableFlags_Borders)) {
        for (const auto& t : tables) {
            ImGui::TableNextRow(); ImGui::TableSetColumnIndex(0);
            if (ImGui::Selectable(t.c_str(), state.activeTable == t)) {
                state.setActiveTable(t, db_con);
            }
        }
        ImGui::EndTable();
    }
    ImGui::EndChild();

    // 2. ЦЕНТР: Превью активной таблицы
    ImGui::BeginChild("PreviewArea", ImVec2(0, totalHeight - 250), true);
    if (!state.activeTable.empty()) {
        auto res = model.RunQuery("SELECT * FROM \"" + state.activeTable + "\" LIMIT 30");
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

    // 3. НИЗ: Кнопки операций (согласно Паспорту)
    ImGui::Separator();
    
    // --- Блок IMPORT / EXPORT / DELETE ---
    ImGui::BeginGroup();
    if (ImGui::Button(" ИМПОРТ ", ImVec2(90, 30))) {
        NFD::UniquePathU8 p; nfdu8filteritem_t f = {"CSV", "csv"};
        if (NFD::OpenDialog(p, &f, 1) == NFD_OKAY) {
            std::string path = p.get();
            std::string name = path.substr(path.find_last_of("/\\") + 1);
            name = name.substr(0, name.find_last_of("."));
            if (model.ImportCSV(name, path)) {
                state.refreshTableList(db_con);
                state.setActiveTable(name, db_con);
            }
        }
    }
    ImGui::SameLine();

    if (ImGui::Button(" ЭКСПОРТ ", ImVec2(90, 30)) && !state.activeTable.empty()) {
        NFD::UniquePathU8 p; nfdu8filteritem_t f = {"CSV", "csv"};
        if (NFD::SaveDialog(p, &f, 1, nullptr, (state.activeTable + ".csv").c_str()) == NFD_OKAY)
            model.RunQuery("COPY \"" + state.activeTable + "\" TO '" + std::string(p.get()) + "' (HEADER, DELIMITER ',')");
    }
    ImGui::SameLine();

    // КНОПКА УДАЛИТЬ (Красная)
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.24f, 0.24f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.3f, 0.3f, 1.0f));
    
    if (ImGui::Button(" УДАЛИТЬ ", ImVec2(90, 30)) && !state.activeTable.empty()) {
        if (model.DeleteTable(state.activeTable)) {
            state.refreshTableList(db_con); // Обновляем список для всех окон
            state.activeTable = "";         // Сбрасываем выбор
            state.columnList.clear();
        }
    }
    ImGui::PopStyleColor(2);
    ImGui::EndGroup();

    ImGui::SameLine();

    // Блок UNION (Кнопка + Список)
    ImGui::BeginGroup();
    
    if (ImGui::Button(" UNION ", ImVec2(100, 30)) && !state.activeTable.empty() && !tables.empty()) {
        std::string resN = state.activeTable + "_u";
        if (model.UnionTables(state.activeTable, tables[selU], resN)) {
            state.refreshTableList(db_con);
            state.setActiveTable(resN, db_con);
        }
    }
    ImGui::SetNextItemWidth(100);

    if (ImGui::BeginCombo("##u_list", tables.empty() ? "-" : tables[selU].c_str())) {
        for (int i = 0; i < tables.size(); i++) {
            if (ImGui::Selectable(tables[i].c_str(), selU == i)) selU = i;
        }
        ImGui::EndCombo();
    }
    ImGui::EndGroup();

    ImGui::SameLine();

    // Блок JOIN (Кнопка + 3 Списка: Таблица Б, Ключ А, Ключ Б)   

    // --- Группа JOIN (Кнопка + 3 Списка) ---
    ImGui::BeginGroup();
    // 1. Заголовки над списками
    ImGui::Text(" Таблица Б"); ImGui::SameLine(135); 
    ImGui::Text(" Ключ А");   ImGui::SameLine(270); 
    ImGui::Text(" Ключ Б");
    
    // 2. Выбор Таблицы Б
    ImGui::SetNextItemWidth(125);
    if (ImGui::BeginCombo("##j_tbl", tables.empty() ? "-" : tables[selJ].c_str())) {
        for (int i = 0; i < (int)tables.size(); i++) {
            if (ImGui::Selectable(tables[i].c_str(), selJ == i)) selJ = i;
        }
        ImGui::EndCombo();
    }
    ImGui::SameLine();

    // 3. Выбор Ключа А
    ImGui::SetNextItemWidth(125);
    if (ImGui::BeginCombo("##j_keyA", state.columnList.empty() ? "-" : state.columnList[keyIdxA].c_str())) {
        for (int i = 0; i < (int)state.columnList.size(); i++) {
            if (ImGui::Selectable(state.columnList[i].c_str(), keyIdxA == i)) keyIdxA = i;
        }
        ImGui::EndCombo();
    }
    ImGui::SameLine();

    // 4. Выбор Ключа Б
    auto colsB = (!tables.empty()) ? model.GetColumns(tables[selJ]) : std::vector<std::string>();
    ImGui::SetNextItemWidth(125);
    if (ImGui::BeginCombo("##j_keyB", colsB.empty() ? "-" : colsB[keyIdxB].c_str())) {
        for (int i = 0; i < (int)colsB.size(); i++) {
            if (ImGui::Selectable(colsB[i].c_str(), keyIdxB == i)) keyIdxB = i;
        }
        ImGui::EndCombo();
    }
    ImGui::SameLine();

    // 5. Кнопка выполнения (самая правая)
    ImGui::BeginGroup();
    bool canJoin = !state.activeTable.empty() && !tables.empty() && !colsB.empty();
    
    if (!canJoin) ImGui::BeginDisabled();
    
    if (ImGui::Button(" JOIN ", ImVec2(100, 30))) {
        std::string resN = state.activeTable + "_j";
        bool isLeft = (joinMode == 0);
        if (model.JoinTables(state.activeTable, tables[selJ], state.columnList[keyIdxA], colsB[keyIdxB], resN, isLeft)) {
            state.refreshTableList(db_con);
            state.setActiveTable(resN, db_con);
        }
    }
    
    ImGui::SetNextItemWidth(100);
    ImGui::Combo("##j_mode", &joinMode, "LEFT\0INNER\0");
    
    if (!canJoin) ImGui::EndDisabled();
    ImGui::EndGroup();
ImGui::EndGroup();
 


}
