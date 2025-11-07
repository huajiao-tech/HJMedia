#include "SQLiteWrapper.h"
#include <iostream>
#include <vector>

int main() {
    try {
        // 创建SQLiteWrapper实例，打开/创建数据库文件
        SQLiteWrapper db("test.db");

        // 创建一个表
        std::vector<std::pair<std::string, std::string>> columns = {
            {"id", "INTEGER PRIMARY KEY"},
            {"name", "TEXT NOT NULL"},
            {"age", "INTEGER"}
        };
        std::string error_msg;
        if (!db.createTable("users", columns, error_msg)) {
            std::cerr << "Error creating table: " << error_msg << std::endl;
            return 1;
        }
        std::cout << "Table 'users' created successfully." << std::endl;

        // 插入数据
        std::vector<std::string> insert_columns = {"name", "age"};
        std::vector<std::string> insert_values = {"Alice", "30"};
        if (!db.insert("users", insert_columns, insert_values, error_msg)) {
            std::cerr << "Error inserting data: " << error_msg << std::endl;
            return 1;
        }
        std::cout << "Data inserted successfully." << std::endl;

        // 查询数据
        std::vector<std::vector<std::string>> results;
        if (!db.select("users", {"id", "name", "age"}, "", results, error_msg)) {
            std::cerr << "Error querying data: " << error_msg << std::endl;
            return 1;
        }
        std::cout << "Query results:" << std::endl;
        for (const auto& row : results) {
            for (const auto& col : row) {
                std::cout << col << " ";
            }
            std::cout << std::endl;
        }

        // 更新数据
        std::vector<std::pair<std::string, std::string>> update_values = {{"age", "31"}};
        if (!db.update("users", update_values, "name='Alice'", error_msg)) {
            std::cerr << "Error updating data: " << error_msg << std::endl;
            return 1;
        }
        std::cout << "Data updated successfully." << std::endl;

        // 删除数据
        if (!db.remove("users", "name='Alice'", error_msg)) {
            std::cerr << "Error deleting data: " << error_msg << std::endl;
            return 1;
        }
        std::cout << "Data deleted successfully." << std::endl;

    } catch (const std::runtime_error& e) {
        std::cerr << "Runtime error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}