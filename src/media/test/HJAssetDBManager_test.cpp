#include "media/db/HJAssetDBManager.h"
#include "media/db/HJSQLiteWrapper.h"
#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include <cstdio> // For remove()

// Use a simple test runner
void run_test(void (*test_func)(), const std::string& test_name) {
    std::cout << "--- Running test: " << test_name << " ---" << std::endl;
    try {
        test_func();
        std::cout << "[ PASS ] " << test_name << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[ FAIL ] " << test_name << " failed with exception: " << e.what() << std::endl;
    }
    std::cout << std::endl;
}

// Global objects for tests
const char* DB_PATH = "asset_manager_test.db";
std::shared_ptr<HJ::HJSQLiteWrapper> wrapper;
std::shared_ptr<HJ::HJAssetDBManager> manager;
std::string error_msg;

void setup() {
    // Clean up previous test database file
    remove(DB_PATH);
    wrapper = std::make_shared<HJ::HJSQLiteWrapper>(DB_PATH);
    manager = std::make_shared<HJ::HJAssetDBManager>(wrapper);
    bool success = manager->initialize(error_msg);
    assert(success && "Manager initialization failed");
}

void teardown() {
    wrapper.reset();
    manager.reset();
    remove(DB_PATH);
}

// Test Case 1: Sub-table management
void test_sub_table_management() {
    // 1. Add a new sub-table
    std::cout << "Step 1: Adding sub-table 'videos'..." << std::endl;
    bool success = manager->addSubTable(1, "videos", 1024, error_msg);
    assert(success && "addSubTable failed");

    // 2. Get all sub-tables and verify
    std::cout << "Step 2: Verifying sub-table creation..." << std::endl;
    std::vector<HJ::SubTableInfo> tables;
    success = manager->getAllSubTables(tables, error_msg);
    assert(success && "getAllSubTables failed");
    assert(tables.size() == 1 && "Should be exactly one sub-table");
    assert(tables[0].name == "videos" && "Sub-table name mismatch");
    assert(tables[0].type == 1 && "Sub-table type mismatch");
    assert(tables[0].storage_limit_bytes == 1024 * 1024 * 1024 && "Storage limit mismatch");

    // 3. Try to add a duplicate table, should fail
    std::cout << "Step 3: Verifying duplicate sub-table prevention..." << std::endl;
    success = manager->addSubTable(1, "videos", 1024, error_msg);
    assert(!success && "Should not be able to add a duplicate sub-table");

    // 4. Remove the sub-table
    std::cout << "Step 4: Removing sub-table 'videos'..." << std::endl;
    success = manager->removeSubTable("videos", error_msg);
    assert(success && "removeSubTable failed");

    // 5. Verify removal
    std::cout << "Step 5: Verifying sub-table removal..." << std::endl;
    success = manager->getAllSubTables(tables, error_msg);
    assert(success && "getAllSubTables failed after removal");
    assert(tables.empty() && "Sub-tables should be empty after removal");
}

// Test Case 2: File record management and data integrity
void test_file_record_management() {
    const std::string TABLE_NAME = "audios";

    // 1. Add a sub-table for this test
    std::cout << "Step 1: Adding sub-table 'audios' for file tests..." << std::endl;
    bool success = manager->addSubTable(2, TABLE_NAME, 512, error_msg);
    assert(success && "Failed to create sub-table for file tests");

    // 2. Add a file record
    std::cout << "Step 2: Adding a file record..." << std::endl;
    HJ::FileRecord record1;
    record1.remote_url = "http://example.com/audio1.mp3";
    record1.local_url = "/data/audio1.mp3";
    record1.status = HJ::FileStatus::Complete;
    record1.file_size_bytes = 1000;
    record1.rid = "rid_audio_1";
    record1.creation_time = 1234567890;
    record1.modification_time = 1234567890;
    success = manager->addFileToSubTable(TABLE_NAME, record1, error_msg);
    assert(success && "addFileToSubTable failed for record1");

    // 3. Verify registry stats after addition
    std::cout << "Step 3: Verifying registry stats after addition..." << std::endl;
    std::vector<HJ::SubTableInfo> tables;
    manager->getAllSubTables(tables, error_msg);
    assert(tables.size() == 1 && "Should be one sub-table");
    assert(tables[0].file_count == 1 && "File count should be 1");
    assert(tables[0].total_storage_used_bytes == 1000 && "Storage used should be 1000");

    // 4. Retrieve the record and verify its content
    std::cout << "Step 4: Retrieving and verifying the file record..." << std::endl;
    HJ::FileRecord fetched_record;
    success = manager->getFileByRemoteURL(TABLE_NAME, record1.remote_url, fetched_record, error_msg);
    assert(success && "getFileByRemoteURL failed");
    assert(fetched_record.local_url == record1.local_url && "local_url mismatch");
    assert(fetched_record.rid == record1.rid && "rid mismatch");

    // 5. Update the record's status
    std::cout << "Step 5: Updating file status..." << std::endl;
    success = manager->updateFileStatus(TABLE_NAME, record1.remote_url, HJ::FileStatus::Pending, 9876543210, error_msg);
    assert(success && "updateFileStatus failed");

    // 6. Verify the update
    std::cout << "Step 6: Verifying status update..." << std::endl;
    manager->getFileByRemoteURL(TABLE_NAME, record1.remote_url, fetched_record, error_msg);
    assert(fetched_record.status == HJ::FileStatus::Pending && "Status should be Pending");
    assert(fetched_record.modification_time == 9876543210 && "Modification time should be updated");

    // 7. Remove the record
    std::cout << "Step 7: Removing the file record..." << std::endl;
    success = manager->removeFileFromSubTable(TABLE_NAME, record1.remote_url, error_msg);
    assert(success && "removeFileFromSubTable failed");

    // 8. Verify registry stats after removal
    std::cout << "Step 8: Verifying registry stats after removal..." << std::endl;
    manager->getAllSubTables(tables, error_msg);
    assert(tables.size() == 1 && "Should still be one sub-table");
    assert(tables[0].file_count == 0 && "File count should be 0");
    assert(tables[0].total_storage_used_bytes == 0 && "Storage used should be 0");

    // 9. Verify the record is gone
    std::cout << "Step 9: Verifying the record is gone..." << std::endl;
    success = manager->getFileByRemoteURL(TABLE_NAME, record1.remote_url, fetched_record, error_msg);
    assert(!success && "Record should not be found after deletion");
}


int main() {
    std::cout << "Starting HJAssetDBManager tests..." << std::endl;
    
    setup();
    run_test(test_sub_table_management, "Sub-Table Management");
    teardown();

    setup();
    run_test(test_file_record_management, "File Record Management & Data Integrity");
    teardown();

    std::cout << "All tests completed." << std::endl;

    return 0;
}
