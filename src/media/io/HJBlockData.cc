//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJBlockData.h"
#include <cstring>
#include <algorithm>

NS_HJ_BEGIN
//***********************************************************************************//

// CRC32 查表（IEEE 802.3 标准多项式 0xEDB88320）
static const uint32_t kCRC32Table[256] = {
    0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
    0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988, 0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
    0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
    0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5,
    0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172, 0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
    0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
    0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F,
    0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924, 0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,
    0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
    0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
    0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E, 0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457,
    0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
    0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB,
    0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0, 0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
    0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
    0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD,
    0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A, 0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683,
    0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
    0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7,
    0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC, 0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
    0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
    0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79,
    0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236, 0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F,
    0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
    0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713,
    0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38, 0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
    0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
    0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
    0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2, 0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB,
    0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
    0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD706B3, 0x54DE5729, 0x23D967BF,
    0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94, 0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
};

//***********************************************************************************//
// 确保 BlockBuffer 已分配（懒加载）
//***********************************************************************************//
void HJBlockData::ensureBuffer() {
    if (!m_buffer) {
        m_buffer = HJCreateu<BlockBuffer>();
    }
}

//***********************************************************************************//
// 构造函数
//***********************************************************************************//
HJBlockData::HJBlockData(size_t global_offset, size_t valid_size)
    : m_global_offset(global_offset)
    , m_valid_size(valid_size > kBlockSize ? kBlockSize : valid_size)
    , m_buffer{}  // 懒加载，初始为空
    , m_written_count(0) {
    // m_buffer 初始为空，首次写入时才分配
    if (valid_size > kBlockSize) {
        // 由于是 const 成员，只能在初始化列表中赋值
        // 这里仅为了逻辑上的防御性编程，虽然初始化列表中已经处理了强转
        // LOG_WARN("HJBlockData valid_size %zu exceeds max capacity, truncated to %zu", valid_size, kBlockSize);
    }
}

//***********************************************************************************//
// 移动构造函数
//***********************************************************************************//
HJBlockData::HJBlockData(HJBlockData&& other) noexcept
    : m_global_offset(other.m_global_offset)
    , m_valid_size(other.m_valid_size)
    , m_buffer(std::move(other.m_buffer))
    , m_written_count(other.m_written_count) {
    // 源对象置为有效但空状态
    other.m_written_count = 0;
}

//***********************************************************************************//
// 移动赋值运算符
//***********************************************************************************//
HJBlockData& HJBlockData::operator=(HJBlockData&& other) noexcept {
    if (this != &other) {
        // 由于 m_global_offset 和 m_valid_size 是 const，无法重新赋值
        // 这里只移动非 const 成员，并要求调用者保证移动的语义正确性（通常用于容器重排）
        // 严格来说 const 成员的存在使得默认的移动赋值受限，但这里我们假设
        // 调用者清楚这种限制，主要用于 std::vector 等容器的 resize/emplace 操作
        
        //HJ_AUTO_LOCK(m_mutex);
        m_buffer = std::move(other.m_buffer);
        m_written_count = other.m_written_count;
        
        // 源对象置为有效但空状态
        other.m_written_count = 0;
    }
    return *this;
}

//***********************************************************************************//
// 核心写入接口
//***********************************************************************************//
size_t HJBlockData::write(size_t offset, const uint8_t* src, size_t length) {
    // 安全校验：空指针检查
    if (src == nullptr) {
        return 0;
    }
    
    // 安全校验：偏移越界检查（以有效大小为界）
    if (offset >= m_valid_size) {
        return 0;
    }
    
    // 安全校验：长度为 0
    if (length == 0) {
        return 0;
    }
    
    // 计算实际可写入的字节数（自动截断超出 valid_size 部分）
    size_t actual_length = std::min(length, m_valid_size - offset);
    
    //HJ_AUTO_LOCK(m_mutex);
    
    // 懒加载：确保 buffer 已分配
    ensureBuffer();
    
    // 使用 memcpy 进行高效数据拷贝
    std::memcpy(m_buffer->m_data.data() + offset, src, actual_length);
    
    // 更新写入状态位图和计数
    for (size_t i = 0; i < actual_length; ++i) {
        size_t bit_index = offset + i;
        if (!m_buffer->m_write_status.test(bit_index)) {
            m_buffer->m_write_status.set(bit_index);
            ++m_written_count;
        }
    }
    
    return actual_length;
}

//***********************************************************************************//
// 快速完整性校验 - O(1)
//***********************************************************************************//
bool HJBlockData::isComplete() const {
    //HJ_AUTO_LOCK(m_mutex);
    // 比较已写入字节数与有效大小
    return m_written_count == m_valid_size;
}

//***********************************************************************************//
// 精准完整性校验 - 返回未写入偏移列表
//***********************************************************************************//
std::vector<size_t> HJBlockData::getUnwrittenOffsets() const {
    //HJ_AUTO_LOCK(m_mutex);
    
    std::vector<size_t> unwritten_offsets;
    
    // 如果已完整，直接返回空列表
    if (m_written_count == m_valid_size) {
        return unwritten_offsets;
    }
    
    // 如果 buffer 未分配，所有位置都是未写入的
    if (!m_buffer) {
        unwritten_offsets.reserve(m_valid_size);
        for (size_t i = 0; i < m_valid_size; ++i) {
            unwritten_offsets.push_back(i);
        }
        return unwritten_offsets;
    }
    
    // 预分配空间以提高效率
    unwritten_offsets.reserve(m_valid_size - m_written_count);
    
    // 遍历位图，收集未写入位（仅检查有效范围）
    for (size_t i = 0; i < m_valid_size; ++i) {
        if (!m_buffer->m_write_status.test(i)) {
            unwritten_offsets.push_back(i);
        }
    }
    
    return unwritten_offsets;
}

//***********************************************************************************//
// 块内偏移转全局偏移
//***********************************************************************************//
size_t HJBlockData::toGlobalOffset(size_t local_offset) const {
    //HJ_AUTO_LOCK(m_mutex);
    
    // 边界校验（必须在有效范围内）
    if (local_offset >= m_valid_size) {
        return SIZE_MAX;
    }
    
    return m_global_offset + local_offset;
}

//***********************************************************************************//
// 全局偏移转块内偏移
//***********************************************************************************//
size_t HJBlockData::toLocalOffset(size_t global_offset) const {
    //HJ_AUTO_LOCK(m_mutex);
    
    // 归属校验（必须在有效范围内）
    if (global_offset < m_global_offset || 
        global_offset >= m_global_offset + m_valid_size) {
        return SIZE_MAX;
    }
    
    return global_offset - m_global_offset;
}

//***********************************************************************************//
// 检查全局偏移是否属于本块
//***********************************************************************************//
bool HJBlockData::containsGlobalOffset(size_t global_offset) const {
    //HJ_AUTO_LOCK(m_mutex);
    return global_offset >= m_global_offset && 
           global_offset < m_global_offset + m_valid_size;
}

//***********************************************************************************//
// 获取数据缓冲区只读指针
//***********************************************************************************//
const uint8_t* HJBlockData::data() const {
    //HJ_AUTO_LOCK(m_mutex);
    if (!m_buffer) {
        return nullptr;
    }
    return m_buffer->m_data.data();
}

//***********************************************************************************//
// 获取数据缓冲区可写指针
//***********************************************************************************//
uint8_t* HJBlockData::mutableData() {
    //HJ_AUTO_LOCK(m_mutex);
    if (!m_buffer) {
        return nullptr;
    }
    return m_buffer->m_data.data();
}

//***********************************************************************************//
// 检查 BlockBuffer 是否已分配
//***********************************************************************************//
bool HJBlockData::hasBuffer() const {
    return m_buffer != nullptr;
}

//***********************************************************************************//
// 清理 BlockBuffer 释放内存
//***********************************************************************************//
void HJBlockData::clearBuffer() {
    //HJ_AUTO_LOCK(m_mutex);
    m_buffer.reset();
    m_written_count = 0;
}

//***********************************************************************************//
// 重置数据块
//***********************************************************************************//
void HJBlockData::reset() {
    //HJ_AUTO_LOCK(m_mutex);
    
    // 如果 buffer 已分配，重置其内容
    if (m_buffer) {
        // 清空数据缓冲区
        m_buffer->m_data.fill(0);
        
        // 重置写入状态位图
        m_buffer->m_write_status.reset();
    }
    
    // 重置已写入计数
    m_written_count = 0;
}

//***********************************************************************************//
// 获取块物理容量
//***********************************************************************************//
size_t HJBlockData::getBlockCapacity() const {
    return kBlockSize;
}

//***********************************************************************************//
// 获取逻辑有效大小
//***********************************************************************************//
size_t HJBlockData::getValidSize() const {
    return m_valid_size;
}

//***********************************************************************************//
// 获取全局起始偏移
//***********************************************************************************//
size_t HJBlockData::getGlobalStartOffset() const {
    //HJ_AUTO_LOCK(m_mutex);
    return m_global_offset;
}

//***********************************************************************************//
// 计算全局结束偏移
//***********************************************************************************//
size_t HJBlockData::getGlobalEndOffset() const {
    //HJ_AUTO_LOCK(m_mutex);
    return m_global_offset + m_valid_size;
}

//***********************************************************************************//
// 获取已写入字节数
//***********************************************************************************//
size_t HJBlockData::getWrittenCount() const {
    //HJ_AUTO_LOCK(m_mutex);
    return m_written_count;
}

//***********************************************************************************//
// CRC32 校验
//***********************************************************************************//
uint32_t HJBlockData::calculateCRC32() const {
    //HJ_AUTO_LOCK(m_mutex);
    
    uint32_t crc = 0xFFFFFFFF;
    
    // 如果 buffer 未分配，返回空数据的 CRC
    if (!m_buffer) {
        // 空数据的 CRC32
        for (size_t i = 0; i < m_valid_size; ++i) {
            crc = kCRC32Table[(crc ^ 0) & 0xFF] ^ (crc >> 8);
        }
        return crc ^ 0xFFFFFFFF;
    }
    
    // 仅计算有效范围的数据
    for (size_t i = 0; i < m_valid_size; ++i) {
        uint8_t byte = m_buffer->m_data[i];
        crc = kCRC32Table[(crc ^ byte) & 0xFF] ^ (crc >> 8);
    }
    
    return crc ^ 0xFFFFFFFF;
}

NS_HJ_END
