//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include <string>

// Lightweight wrapper for BLOB bytes. Kept minimal to avoid heavy includes.
struct HJBlob {
    std::string bytes; // raw byte buffer
};
