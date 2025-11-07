#pragma once

#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include "HJUtilitys.h"
#include "HJSQLiteWrapper.h"

NS_HJ_BEGIN

// Enum for file status, mapping to INTEGER in the database
enum class FileStatus {
    Pending = 0,
    Complete = 1,
    Error = 2
};

// Struct for retrieving main table entries
struct SubTableInfo {
    int id;
    int type;
    std::string name;
    int64_t storage_limit_bytes;
    int file_count;
    int64_t total_storage_used_bytes;
};

// Struct for representing a record in a sub-table
struct FileRecord {
    int id;
    std::string remote_url;
    std::string local_url;
    FileStatus status;
    int64_t file_size_bytes;
    std::string rid; // Using string for flexibility, to match TEXT DB type
    int64_t creation_time;
    int64_t modification_time;
};

/**
 * @class HJAssetDBManager
 * @brief Manages the business logic for a main registry table and multiple sub-tables.
 */
class HJAssetDBManager {
public:
    explicit HJAssetDBManager(std::shared_ptr<HJSQLiteWrapper> db_wrapper);
    ~HJAssetDBManager();

    // Creates the main registry table if it doesn't exist.
    bool initialize(std::string& error_msg);

    // --- Main Table Operations ---

    // Adds a new sub-table entry to the registry and creates the corresponding sub-table.
    bool addSubTable(int type, const std::string& name, int64_t storage_limit_mb, std::string& error_msg);

    // Removes a sub-table entry from the registry and drops the sub-table.
    bool removeSubTable(const std::string& name, std::string& error_msg);

    // Retrieves information for all registered sub-tables.
    bool getAllSubTables(std::vector<SubTableInfo>& tables, std::string& error_msg);

    // --- Sub-Table Operations ---

    // Adds a file record to a specified sub-table and updates the main registry.
    bool addFileToSubTable(const std::string& sub_table_name, const FileRecord& record, std::string& error_msg);

    // Removes a file record from a sub-table based on its remote_url and updates the main registry.
    bool removeFileFromSubTable(const std::string& sub_table_name, const std::string& remote_url, std::string& error_msg);

    // Updates the status and modification time of a file record.
    bool updateFileStatus(const std::string& sub_table_name, const std::string& remote_url, FileStatus new_status, int64_t modification_time, std::string& error_msg);

    // Retrieves all file records from a sub-table that match a given WHERE clause.
    bool getFilesFromSubTable(const std::string& sub_table_name, const HJSQLiteWrapper::WhereConditions& where, std::vector<FileRecord>& records, std::string& error_msg);

    // Retrieves a single file record by its unique remote_url.
    bool getFileByRemoteURL(const std::string& sub_table_name, const std::string& remote_url, FileRecord& record, std::string& error_msg);

private:
    std::shared_ptr<HJSQLiteWrapper> db_;
    
    // Helper to parse query results into a FileRecord. Returns true on success.
    bool parseFileRecord(const std::vector<std::string>& row, FileRecord& record);

    // Helper to parse query results into a SubTableInfo. Returns true on success.
    bool parseSubTableInfo(const std::vector<std::string>& row, SubTableInfo& info);
};

NS_HJ_END
