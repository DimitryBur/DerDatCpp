#pragma once
#include <string>
#include <vector>


// Предварительное объявление DuckDB, чтобы не инклудить тяжелую библиотеку в каждый файл
namespace duckdb {
    class Connection;
}

class AppState {
public:
    // Перечисления для навигации (Footer)
    enum class Window {
        IMPORT,
        CLEAN,
        MODELING,
        PLOT
    };

    AppState();

    // --- ГЛОБАЛЬНЫЙ МАКЕТ (Shell) ---
    Window currentWindow;      // Текущее активное окно (Footer)
    std::string activeTable;   // Combo 1: Активный файл (без .csv)
    std::string activeColumn;  // Combo 2: Активная колонка (фокус для кнопок и статистики)

    // --- ДАННЫЕ ДЛЯ UI (Списки) ---
    std::vector<std::string> tableList;   // Все таблицы в DuckDB
    std::vector<std::string> columnList;  // Все колонки активной таблицы

    // --- ЛОГИКА СИНХРОНИЗАЦИИ ---
    
    // Обновить список всех таблиц из БД
    void refreshTableList(duckdb::Connection& con);
    
    // Обновить список колонок для текущей activeTable
    void refreshColumnList(duckdb::Connection& con);
    
    // Сменить таблицу и сразу обновить её колонки
    void setActiveTable(const std::string& tableName, duckdb::Connection& con);

    // Сменить колонку (для фокуса в Инспекторе)
    void setActiveColumn(const std::string& columnName);
};
