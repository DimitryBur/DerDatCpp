#include "AppState.hpp"
#include <algorithm>
#include <duckdb.hpp>

// Конструктор инициализирует начальное состояние
AppState::AppState() 
    : activeTable(""), activeColumn(""), currentWindow(Window::IMPORT) {
}

// Метод обновления списка таблиц из базы DuckDB
void AppState::refreshTableList(duckdb::Connection& con) {
    tableList.clear();
    try {
        auto result = con.Query("PRAGMA show_tables");
        if (!result->HasError()) {
            for (size_t i = 0; i < result->RowCount(); i++) {
                tableList.push_back(result->GetValue(0, i).ToString());
            }
        }
    } catch (...) {
        // Ошибка доступа к БД
    }

    // Если активная таблица удалена или пуста, сбрасываем выбор
    if (std::find(tableList.begin(), tableList.end(), activeTable) == tableList.end()) {
        activeTable = tableList.empty() ? "" : tableList[0];
        refreshColumnList(con); // Обновляем колонки для новой активной таблицы
    }
}

// Метод обновления списка колонок для выбранной таблицы
void AppState::refreshColumnList(duckdb::Connection& con) {
    columnList.clear();
    if (activeTable.empty()) {
        activeColumn = "";
        return;
    }

    try {
        // Получаем структуру таблицы
        auto result = con.Query("PRAGMA table_info('" + activeTable + "')");
        if (!result->HasError()) {
            for (size_t i = 0; i < result->RowCount(); i++) {
                // В PRAGMA table_info имя колонки идет вторым столбцом (индекс 1)
                columnList.push_back(result->GetValue(1, i).ToString());
            }
        }
    } catch (...) {
        activeColumn = "";
    }

    // Проверяем, осталась ли текущая активная колонка в новом списке
    if (std::find(columnList.begin(), columnList.end(), activeColumn) == columnList.end()) {
        activeColumn = columnList.empty() ? "" : columnList[0];
    }
}

// Удобный метод для смены активной таблицы (вызывается из Header Combo)
void AppState::setActiveTable(const std::string& tableName, duckdb::Connection& con) {
    if (activeTable != tableName) {
        activeTable = tableName;
        refreshColumnList(con); // Сразу перестраиваем список колонок
    }
}
