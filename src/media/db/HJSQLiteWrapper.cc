//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include <stdexcept>
#include <sstream>
#include "HJSQLiteWrapper.h"
#include "HJFLog.h"

NS_HJ_BEGIN
/////////////////////////////////////////////////////////////////////////////
HJSQLiteWrapper::HJSQLiteWrapper(const std::string& db_path) : db_path_(db_path), db_(nullptr) {
    if (sqlite3_open(db_path.c_str(), &db_) != SQLITE_OK) {
        std::string error = "Failed to open database: " + std::string(sqlite3_errmsg(db_));
        throw std::runtime_error(error);
    }
}

HJSQLiteWrapper::~HJSQLiteWrapper() {
    if (db_) {
        sqlite3_close(db_);
    }
}

// The static callback and QueryResult struct have been removed for a more direct,
// efficient, and safer query implementation using sqlite3_prepare_v2 and sqlite3_step.

bool HJSQLiteWrapper::execute(const std::string& sql, std::string& error_msg) {
    char* err = nullptr;
    int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &err);
    if (rc != SQLITE_OK) {
        if (err) {
            error_msg = err;
            sqlite3_free(err);
        } else {
            error_msg = sqlite3_errmsg(db_); // Use sqlite3_errmsg for more details
        }
        return false;
    }
    return true;
}

bool HJSQLiteWrapper::query(const std::string& sql, std::vector<std::vector<std::string>>& results, std::string& error_msg) {
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        error_msg = sqlite3_errmsg(db_);
        // No need to finalize if prepare failed
        return false;
    }

    results.clear();
    int col_count = sqlite3_column_count(stmt);

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        std::vector<std::string> row;
        row.reserve(col_count);
        for (int i = 0; i < col_count; ++i) {
            const char* col_text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, i));
            row.push_back(col_text ? col_text : "NULL");
        }
        results.push_back(row);
    }

    bool success = (rc == SQLITE_DONE);
    if (!success) {
        error_msg = sqlite3_errmsg(db_);
    }

    sqlite3_finalize(stmt);
    return success;
}

bool HJSQLiteWrapper::createTable(const std::string& table_name, const std::vector<std::pair<std::string, std::string>>& columns, std::string& error_msg) {
    std::stringstream ss;
    ss << "CREATE TABLE IF NOT EXISTS " << table_name << " (";
    for (size_t i = 0; i < columns.size(); ++i) {
        ss << columns[i].first << " " << columns[i].second;
        if (i < columns.size() - 1) ss << ", ";
    }
    ss << ");";
    return execute(ss.str(), error_msg);
}

bool HJSQLiteWrapper::insert(const std::string& table, const std::vector<std::string>& columns, const std::vector<std::string>& values, std::string& error_msg) {
    if (columns.size() != values.size() || columns.empty()) {
        error_msg = "Columns and values size mismatch or columns are empty";
        return false;
    }

    std::stringstream sql;
    sql << "INSERT INTO " << table << " (";
    for (size_t i = 0; i < columns.size(); ++i) {
        sql << columns[i] << (i < columns.size() - 1 ? ", " : "");
    }
    sql << ") VALUES (";
    for (size_t i = 0; i < values.size(); ++i) {
        sql << "?" << (i < values.size() - 1 ? ", " : "");
    }
    sql << ");";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql.str().c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        error_msg = sqlite3_errmsg(db_);
        return false;
    }

    for (size_t i = 0; i < values.size(); ++i) {
        rc = sqlite3_bind_text(stmt, i + 1, values[i].c_str(), -1, SQLITE_STATIC);
        if (rc != SQLITE_OK) {
            error_msg = sqlite3_errmsg(db_);
            sqlite3_finalize(stmt);
            return false;
        }
    }

    rc = sqlite3_step(stmt);
    bool success = (rc == SQLITE_DONE);
    if (!success) {
        error_msg = sqlite3_errmsg(db_);
    }

    sqlite3_finalize(stmt);
    return success;
}

// Helper function to build WHERE clause and bind values
static void build_where_clause(const HJSQLiteWrapper::WhereConditions& where, std::stringstream& sql, std::vector<std::string>& values) {
    if (!where.empty()) {
        sql << " WHERE ";
        for (size_t i = 0; i < where.size(); ++i) {
            sql << where[i].first << " = ?";
            values.push_back(where[i].second);
            if (i < where.size() - 1) {
                sql << " AND ";
            }
        }
    }
}

bool HJSQLiteWrapper::update(const std::string& table, const std::vector<std::pair<std::string, std::string>>& set_values, const WhereConditions& where, std::string& error_msg) {
    if (set_values.empty()) {
        error_msg = "No values to set for update";
        return false;
    }

    std::stringstream sql;
    sql << "UPDATE " << table << " SET ";

    std::vector<std::string> all_values;
    for (size_t i = 0; i < set_values.size(); ++i) {
        sql << set_values[i].first << " = ?";
        all_values.push_back(set_values[i].second);
        if (i < set_values.size() - 1) {
            sql << ", ";
        }
    }

    build_where_clause(where, sql, all_values);
    sql << ";";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql.str().c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        error_msg = sqlite3_errmsg(db_);
        return false;
    }

    for (size_t i = 0; i < all_values.size(); ++i) {
        sqlite3_bind_text(stmt, i + 1, all_values[i].c_str(), -1, SQLITE_STATIC);
    }

    rc = sqlite3_step(stmt);
    bool success = (rc == SQLITE_DONE);
    if (!success) {
        error_msg = sqlite3_errmsg(db_);
    }

    sqlite3_finalize(stmt);
    return success;
}

bool HJSQLiteWrapper::remove(const std::string& table, const WhereConditions& where, std::string& error_msg) {
    std::stringstream sql;
    sql << "DELETE FROM " << table;

    std::vector<std::string> where_values;
    build_where_clause(where, sql, where_values);
    sql << ";";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql.str().c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        error_msg = sqlite3_errmsg(db_);
        return false;
    }

    for (size_t i = 0; i < where_values.size(); ++i) {
        sqlite3_bind_text(stmt, i + 1, where_values[i].c_str(), -1, SQLITE_STATIC);
    }

    rc = sqlite3_step(stmt);
    bool success = (rc == SQLITE_DONE);
    if (!success) {
        error_msg = sqlite3_errmsg(db_);
    }

    sqlite3_finalize(stmt);
    return success;
}

bool HJSQLiteWrapper::select(const std::string& table, const std::vector<std::string>& columns, const WhereConditions& where, std::vector<std::vector<std::string>>& results, std::string& error_msg) {
    std::stringstream sql;
    sql << "SELECT ";
    if (columns.empty()) {
        sql << "*";
    } else {
        for (size_t i = 0; i < columns.size(); ++i) {
            sql << columns[i] << (i < columns.size() - 1 ? ", " : "");
        }
    }
    sql << " FROM " << table;

    std::vector<std::string> where_values;
    build_where_clause(where, sql, where_values);
    sql << ";";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql.str().c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        error_msg = sqlite3_errmsg(db_);
        return false;
    }

    for (size_t i = 0; i < where_values.size(); ++i) {
        sqlite3_bind_text(stmt, i + 1, where_values[i].c_str(), -1, SQLITE_STATIC);
    }

    results.clear();
    int col_count = sqlite3_column_count(stmt);

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        std::vector<std::string> row;
        row.reserve(col_count);
        for (int i = 0; i < col_count; ++i) {
            const char* col_text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, i));
            row.push_back(col_text ? col_text : "NULL");
        }
        results.push_back(row);
    }

    bool success = (rc == SQLITE_DONE);
    if (!success) {
        error_msg = sqlite3_errmsg(db_);
    }

    sqlite3_finalize(stmt);
    return success;
}

bool HJSQLiteWrapper::beginTransaction(std::string& error_msg) {
    return execute("BEGIN TRANSACTION;", error_msg);
}

bool HJSQLiteWrapper::commitTransaction(std::string& error_msg) {
    return execute("COMMIT;", error_msg);
}

bool HJSQLiteWrapper::rollbackTransaction(std::string& error_msg) {
    return execute("ROLLBACK;", error_msg);
}

bool HJSQLiteWrapper::dropTable(const std::string& table_name, std::string& error_msg) {
    std::string sql = "DROP TABLE IF EXISTS '" + table_name + "';";
    return execute(sql, error_msg);
}

bool HJSQLiteWrapper::updateRegistryStats(const std::string& sub_table_name, int file_count_delta, int64_t size_delta, std::string& error_msg) {
    std::string sql = "UPDATE sub_table_registry SET file_count = file_count + ?, total_storage_used_bytes = total_storage_used_bytes + ? WHERE sub_table_name = ?;";
    
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        error_msg = sqlite3_errmsg(db_);
        return false;
    }

    sqlite3_bind_int(stmt, 1, file_count_delta);
    sqlite3_bind_int64(stmt, 2, size_delta);
    sqlite3_bind_text(stmt, 3, sub_table_name.c_str(), -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    bool success = (rc == SQLITE_DONE);
    if (!success) {
        error_msg = sqlite3_errmsg(db_);
    }

    sqlite3_finalize(stmt);
    return success;
}

NS_HJ_END