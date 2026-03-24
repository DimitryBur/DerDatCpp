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


ColumnStats DataModel::GetStats(const std::string& table, const std::string& col) {
    ColumnStats stats;
    if (table.empty() || col.empty()) return stats;

    // 1. Сначала считаем общие данные (строки, пропуски, уникальные)
    std::string sql_base = "SELECT "
        "count(*)::TEXT, "
        "(count(*) - count(\"" + col + "\"))::TEXT, "
        "approx_count_distinct(\"" + col + "\")::TEXT "
        "FROM \"" + table + "\"";

    auto res_base = con.Query(sql_base);
    if (!res_base->HasError() && res_base->RowCount() > 0) {
        stats.count  = res_base->GetValue(0, 0).ToString();
        stats.nulls  = res_base->GetValue(1, 0).ToString();
        stats.unique = res_base->GetValue(2, 0).ToString();
    }

    // 2. Считаем математику отдельно, чтобы ошибка в типах не обнулила весь отчет
    std::string sql_math = "SELECT "
        "min(\"" + col + "\")::TEXT, "
        "max(\"" + col + "\")::TEXT, "
        "round(avg(TRY_CAST(\"" + col + "\" AS DOUBLE)), 2)::TEXT, "
        "median(TRY_CAST(\"" + col + "\" AS DOUBLE))::TEXT "
        "FROM \"" + table + "\"";

    auto res_math = con.Query(sql_math);
    if (!res_math->HasError() && res_math->RowCount() > 0) {
        // Проверяем на NULL (если DuckDB вернул пустую ячейку)
        auto vMin = res_math->GetValue(0, 0);
        stats.min = vMin.IsNull() ? "-" : vMin.ToString();
        
        auto vMax = res_math->GetValue(1, 0);
        stats.max = vMax.IsNull() ? "-" : vMax.ToString();

        auto vAvg = res_math->GetValue(2, 0);
        stats.avg = vAvg.IsNull() ? "0.00" : vAvg.ToString();

        auto vMed = res_math->GetValue(3, 0);
        stats.median = vMed.IsNull() ? "0.00" : vMed.ToString();
    }

    return stats;
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

bool DataModel::JoinTables(const std::string& tableA, const std::string& tableB, 
                           const std::string& keyA, const std::string& keyB, 
                           const std::string& resultName, bool isLeftJoin) {
    if (tableA.empty() || tableB.empty() || keyA.empty() || keyB.empty()) return false;
    
    std::string joinType = isLeftJoin ? "LEFT JOIN" : "INNER JOIN";
    
    // SQL запрос с выбором типа джойна
    std::string sql = "CREATE OR REPLACE TABLE \"" + resultName + "\" AS "
                      "SELECT A.*, B.* EXCLUDE (\"" + keyB + "\") "
                      "FROM \"" + tableA + "\" AS A " +
                      joinType + " \"" + tableB + "\" AS B "
                      "ON A.\"" + keyA + "\" = B.\"" + keyB + "\"";
    
    auto res = con.Query(sql);
    con.Query("CHECKPOINT");
    return !res->HasError();
    }

    bool DataModel::SmartFill(const std::string& table, const std::string& colA, const std::string& colB) {
    if (table.empty() || colA.empty()) return false;

    // 1. Сначала заполняем из колонки Б, где это возможно
    std::string sql1 = "UPDATE \"" + table + "\" SET \"" + colA + "\" = \"" + colB + 
                       "\" WHERE \"" + colA + "\" IS NULL AND \"" + colB + "\" IS NOT NULL";
    con.Query(sql1);

    // 2. Линейная интерполяция для оставшихся NULL (DuckDB Window Functions)
    // Мы берем последнее известное значение ДО и первое известное ПОСЛЕ, затем делим пополам
    std::string sql2 = 
        "UPDATE \"" + table + "\" SET \"" + colA + "\" = sub.interpolated "
        "FROM (SELECT rowid, "
        "( (NULLIF(lag(\"" + colA + "\" IGNORE NULLS) OVER (ORDER BY rowid), 0) + "
        "   NULLIF(lead(\"" + colA + "\" IGNORE NULLS) OVER (ORDER BY rowid), 0)) / 2 ) as interpolated "
        "FROM \"" + table + "\") AS sub "
        "WHERE \"" + table + "\".rowid = sub.rowid AND \"" + table + "\".\"" + colA + "\" IS NULL";
    
    auto res = con.Query(sql2);
    return !res->HasError();
    }
    bool DataModel::ExtractNumbers(const std::string& table, const std::string& col) {
    std::string sql = "UPDATE \"" + table + "\" SET \"" + col + 
                      "\" = regexp_extract(\"" + col + "\", '([0-9]+[.,]?[0-9]*)')";
    return !con.Query(sql)->HasError();
}

bool DataModel::CleanData(const std::string& table, const std::string& col, 
                          FillMode mode, const std::string& param) {
    if (table.empty() || col.empty()) return false;
    std::string sql = "";

    switch (mode) {
        case FillMode::MEDIAN:
            sql = "UPDATE \"" + table + "\" SET \"" + col + "\" = (SELECT median(TRY_CAST(\"" + col + 
                  "\" AS DOUBLE)) FROM \"" + table + "\") WHERE \"" + col + "\" IS NULL";
            break;

        case FillMode::CONSTANT:
            sql = "UPDATE \"" + table + "\" SET \"" + col + "\" = '" + param + 
                  "' WHERE \"" + col + "\" IS NULL";
            break;

        case FillMode::MODA:
            sql = "UPDATE \"" + table + "\" SET \"" + col + "\" = (SELECT \"" + col + 
                  "\" FROM \"" + table + "\" GROUP BY 1 ORDER BY count(*) DESC LIMIT 1) WHERE \"" + col + "\" IS NULL";
            break;

        case FillMode::SMART:
            // 1. Сверка с другой колонкой (параметр param здесь — имя колонки Б)
            if (!param.empty()) {
                con.Query("UPDATE \"" + table + "\" SET \"" + col + "\" = \"" + param + 
                          "\" WHERE \"" + col + "\" IS NULL AND \"" + param + "\" IS NOT NULL");
            }
            // 2. Линейная интерполяция по соседям
            sql = "UPDATE \"" + table + "\" SET \"" + col + "\" = ((lag(\"" + col + 
                  "\" IGNORE NULLS) OVER(ORDER BY rowid) + lead(\"" + col + 
                  "\" IGNORE NULLS) OVER(ORDER BY rowid)) / 2) WHERE \"" + col + "\" IS NULL";
            break;
            }

            bool success = RunCommand(sql);
            con.Query("CHECKPOINT"); // Сбрасываем изменения на диск/в память
            return success;
            }
            bool DataModel::RemoveNulls(const std::string& table, const std::string& col) {
            std::string sql = "DELETE FROM \"" + table + "\" WHERE \"" + col + "\" IS NULL";
            return RunCommand(sql);
            }

            bool DataModel::DropColumn(const std::string& table, const std::string& col) {
            std::string sql = "ALTER TABLE \"" + table + "\" DROP COLUMN \"" + col + "\"";
            bool ok = RunCommand(sql);
            con.Query("CHECKPOINT");
            return ok;
            }
            bool DataModel::RunCommand(const std::string& sql) {
            if (sql.empty()) return false;
            auto res = con.Query(sql);
            return !res->HasError();
            }

            // 2. Реализация RemoveOutliers
            bool DataModel::RemoveOutliers(const std::string& table, const std::string& col, double percentile) {
            if (table.empty() || col.empty()) return false;
    
            // percentile приходит как 0.05, 0.10 или 0.25
            std::string lowP = std::to_string(percentile);          // Нижний порог (напр. 0.05)
            std::string highP = std::to_string(1.0 - percentile);   // Верхний порог (напр. 0.95)
    
            // Удаляем всё, что ВНЕ этого диапазона
            std::string sql = "DELETE FROM \"" + table + "\" WHERE "
                      "TRY_CAST(\"" + col + "\" AS DOUBLE) < (SELECT quantile_cont(TRY_CAST(\"" + col + "\" AS DOUBLE), " + lowP + ") FROM \"" + table + "\") OR "
                      "TRY_CAST(\"" + col + "\" AS DOUBLE) > (SELECT quantile_cont(TRY_CAST(\"" + col + "\" AS DOUBLE), " + highP + ") FROM \"" + table + "\")";
    
            bool ok = RunCommand(sql);
            con.Query("CHECKPOINT");
            return ok;
            }






