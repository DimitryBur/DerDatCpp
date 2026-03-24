#pragma once
#include <string>
#include <vector>
#include <duckdb.hpp> // Проверь, что путь к DuckDB прописан в CMake/Include

class AppState {
public:
    AppState();

    // Состояние выбора
    std::string activeTable;   // Без .csv
    std::string activeColumn;  // Текущий фокус для статистики

    // Списки для UI (Combo)
    std::vector<std::string> tableList;
    std::vector<std::string> columnList;

    // Навигация
    enum class Window { IMPORT, CLEAN, MODELING, PLOT };
    Window currentWindow;

    // Методы обновления
    void refreshTableList(duckdb::Connection& con);
    void refreshColumnList(duckdb::Connection& con);
    void setActiveTable(const std::string& tableName, duckdb::Connection& con);
};
