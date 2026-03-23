#pragma once
#include <duckdb.hpp>
#include <string>
#include <vector>
#include <memory>

class DataModel {
public:
    DataModel();
    bool ImportCSV(const std::string& tableName, const std::string& filePath);
    bool DeleteTable(const std::string& tableName);
    bool UnionTables(const std::string& tableA, const std::string& tableB, const std::string& resultName);

    std::vector<std::string> GetTableNames();
    std::unique_ptr<duckdb::QueryResult> RunQuery(const std::string& sql);
    std::vector<std::string> GetColumns(const std::string& table);
    long long GetRowCount(const std::string& tableName);
    int GetColCount(const std::string& tableName);

private:
    duckdb::DuckDB db;
    duckdb::Connection con;
};
