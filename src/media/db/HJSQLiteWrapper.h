//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJUtilitys.h"

extern "C" {
#include "sqlite3.h"
}

NS_HJ_BEGIN
/////////////////////////////////////////////////////////////////////////////
class HJSQLiteWrapper {
public:
    // Type alias for structured WHERE clauses
    using WhereConditions = std::vector<std::pair<std::string, std::string>>;

    // Constructor: Open database connection
    // If the database file does not exist, it will be created automatically.
    HJSQLiteWrapper(const std::string& db_path);
    
    // Destructor: Close database connection
    ~HJSQLiteWrapper();

    // --- Transaction Management ---
    bool beginTransaction(std::string& error_msg);
    bool commitTransaction(std::string& error_msg);
    bool rollbackTransaction(std::string& error_msg);
    
    // --- Raw Execution (use with caution) ---
    // Execute a raw SQL statement (for complex operations not covered by other methods)
    bool execute(const std::string& sql, std::string& error_msg);
    
    // Execute a raw query and return results
    bool query(const std::string& sql, std::vector<std::vector<std::string>>& results, std::string& error_msg);
    
    // --- Table Operations ---
    // Create a table
    bool createTable(const std::string& table_name, const std::vector<std::pair<std::string, std::string>>& columns, std::string& error_msg);
    bool dropTable(const std::string& table_name, std::string& error_msg);

    // --- Data Manipulation (DML) ---
    // Insert into a table
    bool insert(const std::string& table, const std::vector<std::string>& columns, const std::vector<std::string>& values, std::string& error_msg);
    
    // Update records in a table using structured WHERE conditions
    bool update(const std::string& table, const std::vector<std::pair<std::string, std::string>>& set_values, const WhereConditions& where, std::string& error_msg);
    
    // Delete from a table using structured WHERE conditions
    bool remove(const std::string& table, const WhereConditions& where, std::string& error_msg);
    
    // Select from a table using structured WHERE conditions
    bool select(const std::string& table, const std::vector<std::string>& columns, const WhereConditions& where, std::vector<std::vector<std::string>>& results, std::string& error_msg);

    // --- Specialized Operations ---
    bool updateRegistryStats(const std::string& sub_table_name, int file_count_delta, int64_t size_delta, std::string& error_msg);

private:
    sqlite3* db_;
    std::string db_path_;
};

/////////////////////////////////////////////////////////////////////////////
/**
* 
*/

NS_HJ_END