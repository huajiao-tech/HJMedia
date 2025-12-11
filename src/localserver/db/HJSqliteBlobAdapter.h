//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once

#include <string>
#include "sqlite3.h"
#include "sqlite_orm/sqlite_orm.h"
#include "HJBlob.h"

namespace sqlite_orm {

template <>
struct type_printer<HJBlob> {
    static std::string print() { return "BLOB"; }
};

template <>
struct statement_binder<HJBlob> {
    int bind(sqlite3_stmt* stmt, int index, const HJBlob& value) {
        const void* data = value.bytes.empty() ? nullptr : static_cast<const void*>(value.bytes.data());
        const int size = static_cast<int>(value.bytes.size());
        // Use SQLITE_TRANSIENT to let SQLite copy bytes
        return sqlite3_bind_blob(stmt, index, data, size, SQLITE_TRANSIENT);
    }
};

template <>
struct row_extractor<HJBlob> {
    HJBlob extract(sqlite3_stmt* stmt, int columnIndex) const {
        const void* data = sqlite3_column_blob(stmt, columnIndex);
        const int size = sqlite3_column_bytes(stmt, columnIndex);
        HJBlob out;
        if (data && size > 0) {
            out.bytes.assign(static_cast<const char*>(data), static_cast<size_t>(size));
        }
        return out;
    }
};

template <>
struct field_printer<HJBlob> {
    std::string operator()(const HJBlob& value) const {
        return std::string("<blob:") + std::to_string(value.bytes.size()) + ">";
    }
};

} // namespace sqlite_orm
