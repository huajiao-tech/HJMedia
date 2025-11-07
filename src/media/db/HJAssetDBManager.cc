#include "HJAssetDBManager.h"
#include "HJSQLiteWrapper.h"
#include <sstream>
#include <stdexcept>

NS_HJ_BEGIN

// Helper to safely convert string to long long, with a fallback value
static int64_t safe_stoll(const std::string& s, int64_t fallback = 0) {
    try {
        return std::stoll(s);
    } catch (const std::invalid_argument&) {
        return fallback;
    } catch (const std::out_of_range&) {
        return fallback;
    }
}

// Helper to safely convert string to int, with a fallback value
static int safe_stoi(const std::string& s, int fallback = 0) {
    try {
        return std::stoi(s);
    } catch (const std::invalid_argument&) {
        return fallback;
    } catch (const std::out_of_range&) {
        return fallback;
    }
}


HJAssetDBManager::HJAssetDBManager(std::shared_ptr<HJSQLiteWrapper> db_wrapper) : db_(std::move(db_wrapper)) {
}

HJAssetDBManager::~HJAssetDBManager() {
}

bool HJAssetDBManager::initialize(std::string& error_msg) {
    std::vector<std::pair<std::string, std::string>> columns = {
        {"id", "INTEGER PRIMARY KEY AUTOINCREMENT"},
        {"sub_table_type", "INTEGER NOT NULL"},
        {"sub_table_name", "TEXT NOT NULL UNIQUE"},
        {"storage_limit_bytes", "INTEGER DEFAULT 0"},
        {"file_count", "INTEGER DEFAULT 0"},
        {"total_storage_used_bytes", "INTEGER DEFAULT 0"}
    };
    return db_->createTable("sub_table_registry", columns, error_msg);
}

bool HJAssetDBManager::addSubTable(int type, const std::string& name, int64_t storage_limit_mb, std::string& error_msg) {
    if (!db_->beginTransaction(error_msg)) {
        return false;
    }

    // 1. Add to registry
    bool success = false;
    int64_t limit_bytes = storage_limit_mb * 1024 * 1024;
    std::vector<std::string> reg_cols = {"sub_table_type", "sub_table_name", "storage_limit_bytes"};
    std::vector<std::string> reg_vals = {std::to_string(type), name, std::to_string(limit_bytes)};
    
    if (db_->insert("sub_table_registry", reg_cols, reg_vals, error_msg)) {
        // 2. Create the sub-table
        std::vector<std::pair<std::string, std::string>> sub_cols = {
            {"id", "INTEGER PRIMARY KEY AUTOINCREMENT"},
            {"remote_url", "TEXT UNIQUE"},
            {"local_url", "TEXT"},
            {"file_status", "INTEGER NOT NULL"},
            {"file_size_bytes", "INTEGER DEFAULT 0"},
            {"rid", "TEXT"},
            {"creation_time", "INTEGER NOT NULL"},
            {"modification_time", "INTEGER NOT NULL"}
        };
        if (db_->createTable("'" + name + "'", sub_cols, error_msg)) {
            success = true;
        }
    }

    if (success) {
        db_->commitTransaction(error_msg);
    } else {
        db_->rollbackTransaction(error_msg);
    }
    return success;
}

bool HJAssetDBManager::removeSubTable(const std::string& name, std::string& error_msg) {
    if (!db_->beginTransaction(error_msg)) {
        return false;
    }

    bool success = false;
    if (db_->dropTable("'" + name + "'", error_msg)) {
        HJSQLiteWrapper::WhereConditions where = {{"sub_table_name", name}};
        if (db_->remove("sub_table_registry", where, error_msg)) {
            success = true;
        }
    }

    if (success) {
        db_->commitTransaction(error_msg);
    } else {
        db_->rollbackTransaction(error_msg);
    }
    return success;
}

bool HJAssetDBManager::getAllSubTables(std::vector<SubTableInfo>& tables, std::string& error_msg) {
    tables.clear();
    std::vector<std::vector<std::string>> results;
    if (!db_->select("sub_table_registry", {}, {}, results, error_msg)) {
        return false;
    }

    for (const auto& row : results) {
        SubTableInfo info;
        if (parseSubTableInfo(row, info)) {
            tables.push_back(info);
        }
    }
    return true;
}

bool HJAssetDBManager::addFileToSubTable(const std::string& sub_table_name, const FileRecord& record, std::string& error_msg) {
    if (!db_->beginTransaction(error_msg)) {
        return false;
    }

    bool success = false;
    std::vector<std::string> cols = {"remote_url", "local_url", "file_status", "file_size_bytes", "rid", "creation_time", "modification_time"};
    std::vector<std::string> vals = {
        record.remote_url,
        record.local_url,
        std::to_string(static_cast<int>(record.status)),
        std::to_string(record.file_size_bytes),
        record.rid,
        std::to_string(record.creation_time),
        std::to_string(record.modification_time)
    };

    if (db_->insert("'" + sub_table_name + "'", cols, vals, error_msg)) {
        if (db_->updateRegistryStats(sub_table_name, 1, record.file_size_bytes, error_msg)) {
            success = true;
        }
    }

    if (success) {
        db_->commitTransaction(error_msg);
    } else {
        db_->rollbackTransaction(error_msg);
    }
    return success;
}

bool HJAssetDBManager::removeFileFromSubTable(const std::string& sub_table_name, const std::string& remote_url, std::string& error_msg) {
    if (!db_->beginTransaction(error_msg)) {
        return false;
    }

    // 1. Get the record to find its size
    FileRecord record;
    if (!getFileByRemoteURL(sub_table_name, remote_url, record, error_msg)) {
        db_->rollbackTransaction(error_msg);
        error_msg = "File not found: " + remote_url + ". " + error_msg;
        return false;
    }

    // 2. Delete the file record
    bool success = false;
    HJSQLiteWrapper::WhereConditions where = {{"remote_url", remote_url}};
    if (db_->remove("'" + sub_table_name + "'", where, error_msg)) {
        // 3. Update the registry
        if (db_->updateRegistryStats(sub_table_name, -1, -record.file_size_bytes, error_msg)) {
            success = true;
        }
    }

    if (success) {
        db_->commitTransaction(error_msg);
    } else {
        db_->rollbackTransaction(error_msg);
    }
    return success;
}

bool HJAssetDBManager::updateFileStatus(const std::string& sub_table_name, const std::string& remote_url, FileStatus new_status, int64_t modification_time, std::string& error_msg) {
    std::vector<std::pair<std::string, std::string>> set_values = {
        {"file_status", std::to_string(static_cast<int>(new_status))},
        {"modification_time", std::to_string(modification_time)}
    };
    HJSQLiteWrapper::WhereConditions where = {{"remote_url", remote_url}};
    return db_->update("'" + sub_table_name + "'", set_values, where, error_msg);
}

bool HJAssetDBManager::getFilesFromSubTable(const std::string& sub_table_name, const HJSQLiteWrapper::WhereConditions& where, std::vector<FileRecord>& records, std::string& error_msg) {
    records.clear();
    std::vector<std::vector<std::string>> results;
    if (!db_->select("'" + sub_table_name + "'", {}, where, results, error_msg)) {
        return false;
    }

    for (const auto& row : results) {
        FileRecord record;
        if (parseFileRecord(row, record)) {
            records.push_back(record);
        }
    }
    return true;
}

bool HJAssetDBManager::getFileByRemoteURL(const std::string& sub_table_name, const std::string& remote_url, FileRecord& record, std::string& error_msg) {
    std::vector<FileRecord> records;
    HJSQLiteWrapper::WhereConditions where = {{"remote_url", remote_url}};
    if (!getFilesFromSubTable(sub_table_name, where, records, error_msg) || records.empty()) {
        if (records.empty()) {
            error_msg = "Record not found.";
        }
        return false;
    }
    record = records[0];
    return true;
}

bool HJAssetDBManager::parseFileRecord(const std::vector<std::string>& row, FileRecord& record) {
    if (row.size() != 8) return false; // id, remote_url, local_url, status, size, rid, creation, modification
    record.id = safe_stoi(row[0]);
    record.remote_url = row[1];
    record.local_url = row[2];
    record.status = static_cast<FileStatus>(safe_stoi(row[3]));
    record.file_size_bytes = safe_stoll(row[4]);
    record.rid = row[5];
    record.creation_time = safe_stoll(row[6]);
    record.modification_time = safe_stoll(row[7]);
    return true;
}

bool HJAssetDBManager::parseSubTableInfo(const std::vector<std::string>& row, SubTableInfo& info) {
    if (row.size() != 6) return false; // id, type, name, limit, count, used
    info.id = safe_stoi(row[0]);
    info.type = safe_stoi(row[1]);
    info.name = row[2];
    info.storage_limit_bytes = safe_stoll(row[3]);
    info.file_count = safe_stoi(row[4]);
    info.total_storage_used_bytes = safe_stoll(row[5]);
    return true;
}

NS_HJ_END

