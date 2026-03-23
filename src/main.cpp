#include <iostream>
#include <duckdb.hpp>

int main() {
    try {
        duckdb::DuckDB db(nullptr); // База в памяти
        duckdb::Connection con(db);

        con.Query("CREATE TABLE test (id INTEGER, name VARCHAR)");
        con.Query("INSERT INTO test VALUES (1, 'DuckDB + ImGui Work!')");

        auto result = con.Query("SELECT name FROM test WHERE id = 1");
        std::cout << result->GetValue(0, 0).ToString() << std::endl;
        
        std::cout << "\n[SUCCESS] DuckDB is ready!" << std::endl;
    } catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
