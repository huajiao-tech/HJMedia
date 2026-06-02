//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once

#include <string>
#include <algorithm>
#include <cstdint>
#include <limits>
#include "sqlite3.h"
#include "sqlite_orm/sqlite_orm.h"
#include "HJDBBlockStatus.h"
#include "HJMediaDBUtils.h"

namespace sqlite_orm {

// ============================================================================
// std::vector<HJDBSpanInfo> serialization
// Binary format: [4 bytes count][span1][span2]...
// Each span: [4 bytes block_start_idx][4 bytes block_cnt][4 bytes url_len][url bytes]
// ============================================================================

template <>
struct type_printer<std::vector<HJDBSpanInfo>> {
    static std::string print() { return "BLOB"; }
};

template <>
struct statement_binder<std::vector<HJDBSpanInfo>> {
    int bind(sqlite3_stmt* stmt, int index, const std::vector<HJDBSpanInfo>& value) {
        if (value.empty()) {
            // Bind empty blob instead of NULL to avoid constraint violations
            return sqlite3_bind_blob(stmt, index, "", 0, SQLITE_STATIC);
        }
        const size_t kMaxTotalSize = static_cast<size_t>(std::numeric_limits<int>::max());
        // Header: start(4) + count(4) + url_len(4)
        const size_t kSpanHeaderSize = sizeof(int32_t) * 2 + sizeof(uint32_t);

        if (value.size() > static_cast<size_t>(std::numeric_limits<uint32_t>::max())) {
            return SQLITE_TOOBIG;
        }
        // Calculate total size with overflow checks
        size_t total_size = sizeof(uint32_t); // count
        for (const auto& span : value) {
            if (span.block_start_idx < std::numeric_limits<int32_t>::min() ||
                span.block_start_idx > std::numeric_limits<int32_t>::max()) {
                return SQLITE_RANGE;
            }
            if (span.block_cnt < std::numeric_limits<int32_t>::min() ||
                span.block_cnt > std::numeric_limits<int32_t>::max()) {
                return SQLITE_RANGE;
            }
            if (span.local_url.size() > static_cast<size_t>(std::numeric_limits<uint32_t>::max())) {
                return SQLITE_TOOBIG;
            }
            const size_t span_size = kSpanHeaderSize + span.local_url.size();
            if (span_size > kMaxTotalSize - total_size) {
                return SQLITE_TOOBIG;
            }
            total_size += span_size;
        }
        if (total_size > kMaxTotalSize) {
            return SQLITE_TOOBIG;
        }
        std::vector<uint8_t> buffer(total_size);
        uint8_t* ptr = buffer.data();
        auto write_bytes = [&ptr](const void* src, size_t length) {
            const auto* src_bytes = static_cast<const uint8_t*>(src);
            std::copy_n(src_bytes, length, ptr);
            ptr += length;
        };

        // Write count
        uint32_t count = static_cast<uint32_t>(value.size());
        write_bytes(&count, sizeof(uint32_t));

        // Write each span
        for (const auto& span : value) {
            int32_t block_start_idx = span.block_start_idx;
            int32_t block_cnt = span.block_cnt;
            uint32_t url_len = static_cast<uint32_t>(span.local_url.size());

            write_bytes(&block_start_idx, sizeof(int32_t));
            write_bytes(&block_cnt, sizeof(int32_t));
            write_bytes(&url_len, sizeof(uint32_t));
            if (url_len > 0) {
                write_bytes(span.local_url.data(), url_len);
            }
        }
        return sqlite3_bind_blob(stmt, index, buffer.data(), static_cast<int>(buffer.size()), SQLITE_TRANSIENT);
    }
};

template <>
struct row_extractor<std::vector<HJDBSpanInfo>> {
    std::vector<HJDBSpanInfo> extract(sqlite3_stmt* stmt, int columnIndex) const {
        std::vector<HJDBSpanInfo> result;
        const void* data = sqlite3_column_blob(stmt, columnIndex);
        const int size = sqlite3_column_bytes(stmt, columnIndex);
        if (!data || size < static_cast<int>(sizeof(uint32_t))) {
            return result;
        }
        const auto* ptr = static_cast<const uint8_t*>(data);
        const auto* end = ptr + static_cast<size_t>(size);
        auto read_bytes = [&ptr](void* dst, size_t length) {
            auto* dst_bytes = static_cast<uint8_t*>(dst);
            std::copy_n(ptr, length, dst_bytes);
            ptr += length;
        };
        // Header: start(4) + count(4) + url_len(4)
        const size_t kSpanHeaderSize = sizeof(int32_t) * 2 + sizeof(uint32_t);

        uint32_t count = 0;
        read_bytes(&count, sizeof(uint32_t));

        const size_t remaining = static_cast<size_t>(end - ptr);
        const size_t max_count = remaining / kSpanHeaderSize;
        if (static_cast<size_t>(count) > max_count) {
            return {};
        }
        result.reserve(static_cast<size_t>(count));
        for (uint32_t i = 0; i < count; ++i) {
            if (static_cast<size_t>(end - ptr) < kSpanHeaderSize) {
                return {};
            }
            HJDBSpanInfo span;
            read_bytes(&span.block_start_idx, sizeof(int32_t));
            read_bytes(&span.block_cnt, sizeof(int32_t));
            
            uint32_t url_len = 0;
            read_bytes(&url_len, sizeof(uint32_t));
            if (url_len > static_cast<uint32_t>(end - ptr)) {
                return {};
            }
            if (url_len > 0) {
                span.local_url.assign(reinterpret_cast<const char*>(ptr), url_len);
                ptr += url_len;
            }
            result.push_back(std::move(span));
        }
        return result;
    }
};

template <>
struct field_printer<std::vector<HJDBSpanInfo>> {
    std::string operator()(const std::vector<HJDBSpanInfo>& value) const {
        return std::string("<spans:") + std::to_string(value.size()) + ">";
    }
};

template <>
struct type_printer<HJDBBlockStatus> {
    static std::string print() { return "BLOB"; }
};

template <>
struct statement_binder<HJDBBlockStatus> {
    int bind(sqlite3_stmt* stmt, int index, const HJDBBlockStatus& value) {
        // Use empty string instead of nullptr to avoid constraint violations
        const void* data = value.bytes.empty() ? static_cast<const void*>("") : static_cast<const void*>(value.bytes.data());
        const int size = static_cast<int>(value.bytes.size());
        // Use SQLITE_TRANSIENT to let SQLite copy bytes
        return sqlite3_bind_blob(stmt, index, data, size, SQLITE_TRANSIENT);
    }
};

template <>
struct row_extractor<HJDBBlockStatus> {
    HJDBBlockStatus extract(sqlite3_stmt* stmt, int columnIndex) const {
        const void* data = sqlite3_column_blob(stmt, columnIndex);
        const int size = sqlite3_column_bytes(stmt, columnIndex);
        HJDBBlockStatus out;
        if (data && size > 0) {
            const auto* ptr = static_cast<const uint8_t*>(data);
            out.bytes.assign(ptr, ptr + size);
        }
        return out;
    }
};

template <>
struct field_printer<HJDBBlockStatus> {
    std::string operator()(const HJDBBlockStatus& value) const {
        return std::string("<blob:") + std::to_string(value.bytes.size()) + ">";
    }
};

} // namespace sqlite_orm
