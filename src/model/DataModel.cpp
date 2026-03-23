#include "DataModel.hpp"

DataModel::DataModel() : db(nullptr), con(db) {}

bool DataModel::ImportCSV(const std::string& tableName, const std::string& filePath) {
    std::string query = "CREATE OR REPLACE TABLE '" + tableName + "' AS SELECT * FROM read_csv_auto('" + filePath + "')";
    auto result = con.Query(query);
    return !result->HasError();
}

bool DataModel::DeleteTable(const std::string& tableName) {
    if (tableName.empty()) return false;  
    std::string query = "DROP TABLE IF EXISTS \"" + tableName + "\"" ;
    auto res = con.Query(query);  
    con.Query("CHECKPOINT");     
    return !res->HasError();
}

// Добавь в DataModel.cpp
bool DataModel::UnionTables(const std::string& tableA, const std::string& tableB, const std::string& resultName) {
    if (tableA.empty() || tableB.empty()) return false;
    std::string sql = "CREATE OR REPLACE TABLE \"" + resultName + "\" AS "
                      "SELECT * FROM \"" + tableA + "\" UNION ALL BY NAME "
                      "SELECT * FROM \"" + tableB + "\"";
    
    auto res = con.Query(sql);
    con.Query("CHECKPOINT");
    return !res->HasError();
}

long long DataModel::GetRowCount(const std::string& tableName) {
    if (tableName.empty()) return 0;
    auto res = con.Query("SELECT COUNT(*) FROM \"" + tableName + "\"");
    return res->GetValue(0, 0).GetValue<int64_t>();
}

int DataModel::GetColCount(const std::string& tableName) {
    if (tableName.empty()) return 0;
    auto res = con.Query("SELECT count(*) FROM information_schema.columns WHERE table_name = '" + tableName + "'");
    return res->GetValue(0, 0).GetValue<int32_t>();
}





std::vector<std::string> DataModel::GetTableNames() {
    std::vector<std::string> tables;
    auto res = con.Query("SELECT table_name FROM information_schema.tables WHERE table_schema = 'main'");
    for (duckdb::idx_t i = 0; i < res->RowCount(); i++) {
        tables.push_back(res->GetValue(0, i).ToString());
    }
    return tables;
}

std::unique_ptr<duckdb::QueryResult> DataModel::RunQuery(const std::string& sql) {
    return con.Query(sql);
}

std::vector<std::string> DataModel::GetColumns(const std::string& table) {
    std::vector<std::string> cols;
    auto res = con.Query("PRAGMA table_info('" + table + "')");
    for (duckdb::idx_t i = 0; i < res->RowCount(); i++) {
        cols.push_back(res->GetValue(1, i).ToString());
    }
    return cols;
}
