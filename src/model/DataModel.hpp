#pragma once
#include <duckdb.hpp>
#include <string>
#include <vector>
#include <memory>

// Структура для статистики (вынесена из метода для доступности)
struct ColumnStats {
    std::string count = "0";
    std::string nulls = "0";
    std::string unique = "0";
    std::string min = "-";
    std::string max = "-";
    std::string avg = "-";
    std::string median = "-";
};
enum class FillMode { MEDIAN, CONSTANT, MODA, SMART };

class DataModel {
private:
    duckdb::DuckDB db;          // Сам движок БД
    duckdb::Connection con;     // Активное соединение

public:
    DataModel();
    
    // Геттер для получения ссылки на соединение (нужен для AppState)
    duckdb::Connection& GetConnection() { return con; }

    // Твои существующие методы
    bool ImportCSV(const std::string& tableName, const std::string& filePath);
    bool DeleteTable(const std::string& tableName);
    bool UnionTables(const std::string& tableA, const std::string& tableB, const std::string& resultName);
    
    long long GetRowCount(const std::string& tableName);
    int GetColCount(const std::string& tableName);
    bool JoinTables(const std::string& tableA, const std::string& tableB, 
                           const std::string& keyA, const std::string& keyB, 
                           const std::string& resultName, bool isLeftJoin);

    
    ColumnStats GetStats(const std::string& table, const std::string& col);
    bool SmartFill(const std::string& table, const std::string& colA, const std::string& colB);
    bool CleanData(const std::string& table, const std::string& col, 
                   FillMode mode, const std::string& param = "");
    bool ExtractNumbers(const std::string& table, const std::string& col);
    bool RemoveOutliers(const std::string& table, const std::string& col, double percentile = 0.95);
    bool RunCommand(const std::string& sql); // Удобная обертка для UPDATE/DELETE
    // В секцию public класса DataModel
    bool RemoveNulls(const std::string& table, const std::string& col);
    bool DropColumn(const std::string& table, const std::string& col);


    std::vector<std::string> GetTableNames();
    std::vector<std::string> GetColumns(const std::string& table);    
    std::unique_ptr<duckdb::QueryResult> RunQuery(const std::string& sql);
    
    
};
