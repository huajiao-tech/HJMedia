//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR: 
//CREATE TIME: 
//***********************************************************************************//
#include "HJUUIDTools.h"
#include "HJFLog.h"
#include "HJException.h"

#include "openssl/sha.h"
#include <openssl/rand.h> 

NS_HJ_BEGIN
const HJUUID HJUUIDTools::HJ_MAIN_UUID = { {0x6e, 0x53, 0x7f, 0x4f, 0xbf, 0xbc, 0x49, 0x65, 0x99, 0x79, 0x09, 0x58, 0x83, 0x8d, 0xda, 0xc2} }; //6e537f4fbfbc496599790958838ddac2
const HJUUID HJUUIDTools::HJ_DERIVE_UUID0 = { { 0x6e, 0x23, 0x7f, 0x1c, 0x2d, 0xca, 0xd0, 0x82, 0x67, 0x52, 0xab, 0x4, 0x7a, 0xec, 0x60, 0xa0} }; //6e237f1c2dcad0826752ab047aec60a0
//***********************************************************************************//
std::optional<HJUUID> HJUUIDTools::generate_uuid() 
{
    HJUUID uuid;
    if (RAND_bytes(uuid.data.data(), uuid.data.size()) != 1) {
        // HJ_EXCEPT(HJException::ERR_INVALID_CALL, "Error generating random bytes for UUID.");
        // std::fill(uuid.data.begin(), uuid.data.end(), 0); // Return a zero UUID on error
        HJFLoge("Error generating random bytes for UUID.");
        return {};
    }

    // Set UUID version (e.g., version 4 for random) and variant
    // Version 4: 0100 in the most significant 4 bits of the 7th byte
    uuid.data[6] = (uuid.data[6] & 0x0F) | 0x40;
    // Variant 1 (Leach-Salz): 10xx in the most significant 2 bits of the 9th byte
    uuid.data[8] = (uuid.data[8] & 0x3F) | 0x80;

    return uuid;
}
HJUUID HJUUIDTools::derive_uuid(const HJUUID& parent, std::optional<int64_t> number)
{
    // 1. Concatenate parent and the number
    std::vector<uint8_t> buffer;
    buffer.insert(buffer.end(), parent.data.begin(), parent.data.end());

    // Append the number in a fixed byte order (e.g., big-endian)
    if (number.has_value()) {
        for (int i = 7; i >= 0; --i) {
            buffer.push_back((*number >> (i * 8)) & 0xFF);
        }
    }

    // 2. Hash the concatenated data using SHA-256
    std::array<uint8_t, SHA256_DIGEST_LENGTH> hash;
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, buffer.data(), buffer.size());
    SHA256_Final(hash.data(), &sha256);

    // 3. Truncate the hash to 128 bits (16 bytes) to get UUID1
    HJUUID uuid1{};
    std::copy(hash.begin(), hash.begin() + 16, uuid1.data.begin());

    return uuid1;
}

bool HJUUIDTools::verify_uuid(const HJUUID& child, const HJUUID& parent, std::optional<int64_t> number) {
    HJUUID expected = derive_uuid(parent, number);
    return expected.data == child.data;
}

HJUUID HJUUIDTools::derive_uuid(const HJUUID& parent, const std::vector<uint8_t>& data)
{
    std::vector<uint8_t> buffer;
    buffer.insert(buffer.end(), parent.data.begin(), parent.data.end());
    buffer.insert(buffer.end(), data.begin(), data.end());

        // 2. Hash the concatenated data using SHA-256
    std::array<uint8_t, SHA256_DIGEST_LENGTH> hash;
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, buffer.data(), buffer.size());
    SHA256_Final(hash.data(), &sha256);

    // 3. Truncate the hash to 128 bits (16 bytes) to get UUID1
    HJUUID uuid1{};
    std::copy(hash.begin(), hash.begin() + 16, uuid1.data.begin());

    return uuid1;
}

bool HJUUIDTools::verify_uuid(const HJUUID& child, const HJUUID& parent, const std::vector<uint8_t>& data)
{
    HJUUID expected = derive_uuid(parent, data);
    return expected.data == child.data;
}

HJUUID HJUUIDTools::derive_uuid(const std::string& parent, std::optional<int64_t> number)
{
    HJUUID parent_uuid{};
    parent_uuid.fromString(parent);
    return derive_uuid(parent_uuid, number);
}

bool HJUUIDTools::verify_uuid(const std::string& child, const std::string& parent, std::optional<int64_t> number)
{
    HJUUID child_uuid{};
    child_uuid.fromString(child);
    HJUUID parent_uuid{};
    parent_uuid.fromString(parent);
    return verify_uuid(child_uuid, parent_uuid, number);
}

HJUUID HJUUIDTools::derive_uuid_from_main(std::optional<int64_t> number)
{
    return derive_uuid(HJUUIDTools::HJ_MAIN_UUID, number);
}

bool HJUUIDTools::verify_uuid_from_main(const std::string& child, std::optional<int64_t> number)
{
    HJUUID child_uuid{};
    child_uuid.fromString(child);
    return verify_uuid(child_uuid, HJUUIDTools::HJ_MAIN_UUID, number);
}

NS_HJ_END