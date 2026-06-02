//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJUtilitys.h"
#include <array>
#include <bitset>
#include <mutex>
#include <vector>
#include <cstdint>

NS_HJ_BEGIN
//***********************************************************************************//

/**
 * HJBlockData - 高性能、线程安全的二进制数据块管理类
 * 
 * 核心特性：
 * - 固定 32KB 块大小（编译期常量）
 * - 使用 std::bitset 进行位级写入状态追踪
 * - 支持块内任意偏移写入、分段写入、覆盖写入
 * - 提供 O(1) 快速完整性校验和精准校验接口
 * - 支持块内偏移与全局偏移双向转换
 * - CRC32 数据校验支持
 * - 全程线程安全（std::mutex 互斥锁）
 * - 禁用拷贝语义，启用移动语义
 * 
 * 使用场景：
 * - 大文件分块下载/缓存管理
 * - 分段数据完整性校验
 * - 流式数据块状态追踪
 */
class HJBlockData : public HJObject
{
public:
    //==========================================================================
    // 类型定义与常量
    //==========================================================================
    HJ_DECLARE_PUWTR(HJBlockData);
    
    /** 固定块大小：32KB */
    static constexpr size_t kBlockSize = 32 * 1024;

    enum class BlockStatus : uint8_t {
        HOLE = 0,   //缺数据
        CACHED,     //已缓存
        LOCKED,     //锁定
        ALONE       //独占
    };

    typedef struct BlockBuffer {
        /** 数据缓冲区：使用 std::array 保证连续内存和随机访问效率 */
        std::array<uint8_t, kBlockSize> m_data{};
        
        /** 写入状态位图：使用 std::bitset 实现位级追踪，初始全 0 */
        std::bitset<kBlockSize> m_write_status;

        bool isAll() const {
            return m_write_status.all();
        }
    } BlockBuffer;
    //==========================================================================
    // 构造与析构
    //==========================================================================
    
    /**
     * @brief 构造函数
     * @param global_offset 本块在外部大数据块中的全局起始偏移（永久绑定）
     * @param valid_size 当前块的有效逻辑大小，必须 <= kBlockSize，默认 32KB
     * 
     * 初始化：
     * - 数据缓冲区全 0
     * - 写入状态位图全 0（表示未写入）
     * - 已写入计数为 0
     * - 有效大小被记录，后续操作均受此限制
     */
    explicit HJBlockData(size_t global_offset, size_t valid_size = kBlockSize);
    
    /**
     * @brief 析构函数
     */
    ~HJBlockData() = default;

    // 禁用拷贝构造和拷贝赋值
    HJBlockData(const HJBlockData&) = delete;
    HJBlockData& operator=(const HJBlockData&) = delete;

    /**
     * @brief 移动构造函数
     * @param other 源对象（移动后置为有效但空状态）
     */
    HJBlockData(HJBlockData&& other) noexcept;

    /**
     * @brief 移动赋值运算符
     * @param other 源对象（移动后置为有效但空状态）
     * @return 当前对象引用
     */
    HJBlockData& operator=(HJBlockData&& other) noexcept;

    //==========================================================================
    // 核心写入接口
    //==========================================================================
    
    /**
     * @brief 写入数据到块内指定偏移位置
     * 
     * 支持：
     * - 块内任意偏移写入
     * - 分多次写入（不同区间）
     * - 覆盖写入（同一区间重复写入）
     * 
     * 安全校验：
     * - src 指针判空
     * - 偏移越界检查（以 valid_size 为界）
     * - 长度合法性校验（自动截断超出 valid_size 部分）
     * 
     * @param offset 块内偏移量（0 ~ valid_size-1）
     * @param src 源数据指针（不能为空）
     * @param length 期望写入的字节数
     * @return 实际写入的字节数，校验失败返回 0
     * 
     * @note 内部使用 memcpy 保证拷贝效率
     * @note 线程安全
     */
    size_t write(size_t offset, const uint8_t* src, size_t length);

    //==========================================================================
    // 完整性校验接口
    //==========================================================================
    
    /**
     * @brief 快速完整性校验
     * 
     * 通过比较已写入字节计数与有效大小实现 O(1) 效率校验。
     * 
     * @return true 表示所有有效字节均已写入，false 表示存在未写入区域
     * 
     * @note 线程安全
     */
    bool isComplete() const;

    /**
     * @brief 精准完整性校验
     * 
     * 遍历 std::bitset，获取所有未写入字节的块内偏移列表。
     * 仅检查 [0, valid_size) 范围。
     * 用于调试排障，定位缺失数据区域。
     * 
     * @return 未写入字节的块内偏移列表（空列表表示已完整）
     * 
     * @note 时间复杂度 O(valid_size)
     * @note 线程安全
     */
    std::vector<size_t> getUnwrittenOffsets() const;

    //==========================================================================
    // 偏移换算接口
    //==========================================================================
    
    /**
     * @brief 块内偏移转全局偏移
     * 
     * @param local_offset 块内偏移（0 ~ valid_size-1）
     * @return 对应的全局偏移，非法偏移返回 SIZE_MAX
     * 
     * @note 线程安全
     */
    size_t toGlobalOffset(size_t local_offset) const;

    /**
     * @brief 全局偏移转块内偏移
     * 
     * @param global_offset 全局偏移
     * @return 对应的块内偏移，非法偏移（不属于本块有效范围）返回 SIZE_MAX
     * 
     * @note 线程安全
     */
    size_t toLocalOffset(size_t global_offset) const;

    /**
     * @brief 检查全局偏移是否属于本块范围
     * 
     * @param global_offset 全局偏移
     * @return true 表示在 [m_global_offset, m_global_offset + valid_size) 范围内
     * 
     * @note 线程安全
     */
    bool containsGlobalOffset(size_t global_offset) const;

    //==========================================================================
    // 数据访问接口
    //==========================================================================
    
    /**
     * @brief 获取数据缓冲区只读指针
     * @return 指向内部缓冲区的 const 指针，如果 buffer 未分配则返回 nullptr
     * 
     * @warning 返回后的数据访问需调用者自行保证线程安全
     * @note 此方法不会触发 buffer 的懒加载分配
     */
    const uint8_t* data() const;

    /**
     * @brief 获取数据缓冲区可写指针
     * @return 指向内部缓冲区的可写指针，如果 buffer 未分配则返回 nullptr
     * 
     * @warning 返回后的数据访问需调用者自行保证线程安全
     * @warning 通过此指针直接写入不会更新位图状态，建议使用 write() 接口
     * @note 此方法不会触发 buffer 的懒加载分配
     */
    uint8_t* mutableData();

    //==========================================================================
    // 缓冲区管理接口
    //==========================================================================
    
    /**
     * @brief 检查 BlockBuffer 是否已分配
     * @return true 表示 buffer 已分配，false 表示未分配
     */
    bool hasBuffer() const;

    /**
     * @brief 清理 BlockBuffer 释放内存
     * 
     * 执行：
     * - 释放 BlockBuffer 智能指针
     * - 已写入计数归零
     * 
     * @note 不修改全局起始偏移和有效大小（只读属性）
     * @note 清理后再次写入会重新分配 buffer
     * @note 线程安全
     */
    void clearBuffer();

    //==========================================================================
    // 重置与属性接口
    //==========================================================================
    
    /**
     * @brief 重置数据块
     * 
     * 执行：
     * - 如果 buffer 已分配：数据缓冲区清零、写入状态位图全部置 0
     * - 已写入计数归零
     * 
     * @note 不修改全局起始偏移和有效大小（只读属性）
     * @note 不释放 buffer 内存，仅重置状态
     * @note 线程安全
     */
    void reset();

    /**
     * @brief 获取块的物理容量（32KB）
     * @return kBlockSize
     */
    size_t getBlockCapacity() const;

    /**
     * @brief 获取块的逻辑有效大小
     * @return 构造时指定的 valid_size
     */
    size_t getValidSize() const;

    /**
     * @brief 获取全局起始偏移
     * @return 构造时绑定的全局起始偏移
     */
    size_t getGlobalStartOffset() const;

    /**
     * @brief 计算全局结束偏移
     * @return m_global_offset + valid_size（不含边界）
     */
    size_t getGlobalEndOffset() const;

    /**
     * @brief 获取已写入字节数
     * @return 已成功写入的字节总数
     */
    size_t getWrittenCount() const;

    //==========================================================================
    // CRC32 校验接口
    //==========================================================================
    
    /**
     * @brief 计算整块有效数据的 CRC32 校验值
     * 
     * 使用 IEEE 802.3 标准多项式（0xEDB88320）查表法实现。
     * 用于数据防篡改和写入正确性校验。
     * 
     * @return 32 位 CRC32 校验值
     * 
     * @note 时间复杂度 O(valid_size)
     * @note 线程安全
     */
    uint32_t calculateCRC32() const;

    void setStatus(BlockStatus status) {
        m_status = status;
    }
    BlockStatus getStatus() const {
        return m_status;
    }
    void setLocalUrl(const std::string& url) {
        m_local_url = url;
    }
    const std::string& getLocalUrl() const {
        return m_local_url;
    }
private:
    //==========================================================================
    // 私有方法
    //==========================================================================
    
    /**
     * @brief 确保 BlockBuffer 已分配（懒加载）
     * 
     * 如果 m_buffer 为空，则分配新的 BlockBuffer。
     * 此方法应在需要写入数据前调用。
     */
    void ensureBuffer();

    //==========================================================================
    // 私有成员 - 属性区
    //==========================================================================
    
    /** 只读属性：全局起始偏移，构造时永久绑定 */
    const size_t m_global_offset{0};
    
    /** 只读属性：逻辑有效大小，必须 <= kBlockSize */
    const size_t m_valid_size{0};
    
    /** 
     * 数据缓冲区智能指针：包含数据和写入状态位图
     * 懒加载分配，在首次写入时创建
     * 外部可通过 clearBuffer() 释放
     */
    std::unique_ptr<BlockBuffer> m_buffer{};
    
    /** 已写入字节计数：用于 O(1) 快速完整性校验 */
    size_t          m_written_count{0};
    
    /** 线程安全互斥锁：mutable 修饰允许 const 接口使用 */
    mutable std::mutex m_mutex;

    BlockStatus     m_status{BlockStatus::HOLE};
    std::string     m_local_url{};
};

NS_HJ_END
