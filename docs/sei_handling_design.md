# SEI 数据插入与提取方案设计

## 1. 需求分析

在 H.264/H.265 视频流处理中，需要在压缩帧数据中插入或提取 SEI（Supplemental Enhancement Information）数据。
输入数据格式不确定，可能是 **Annex B** 格式（起始码分隔）或 **AVCC/HVCC** 格式（4字节长度前缀）。
接口需要自动检测输入格式，并保持输出格式与输入一致。

### 1.1 核心需求

1. **自动格式检测**：识别输入是 Annex B 还是 Length Prefix。
2. **SEI 插入**：
   * 插入位置：在流结束符（EOS/EOSeq）之前；若无 EOS，则追加在最后。
   * 插入顺序：多个 SEI 按顺序插入。
   * 格式一致性：插入的 SEI NAL 必须封装为与原数据相同的格式（带起始码或带长度前缀）。
3. **SEI 提取**：从数据中提取所有 SEI NAL 数据。

## 2. 格式检测策略

NAL Unit 的封装通常有两种形式：

1. **Annex B**: `[Start Code] [NAL Unit] [Start Code] ...`
   * Start Code 通常为 `00 00 01` 或 `00 00 00 01`。
2. **Length Prefix (AVCC/HVCC)**: `[Length (4 bytes)] [NAL Unit] [Length] ...`
   * Length 为大端序（Big Endian）的 32 位整数。

### 检测算法

读取输入数据的前 4 个字节：

* 如果前 3 字节是 `00 00 01` 或前 4 字节是 `00 00 00 01`，则判定为 **Annex B**。
* 否则，判定为 **Length Prefix**。
  * *注：为了更严谨，可以检查读取出的 Length 是否合理（例如 `Length + 4 <= TotalSize`），但在流式处理或简单判断中，非起始码通常即视为长度前缀。*

## 3. 详细设计

### 3.1 辅助函数

我们需要一个内部枚举来表示格式：

```cpp
enum class NALFormat {
    Unknown,
    AnnexB,
    LengthPrefix
};
```

以及一个检测函数：

```cpp
NALFormat detectFormat(const uint8_t* data, size_t size);
```

### 3.2 插入逻辑 (insertSEIData)

**流程：**

1. **检测格式**。
2. **遍历 NAL Units**：
   * **Annex B**: 使用 `findStartCode` 查找每个 NAL 的边界。
   * **Length Prefix**: 读取 4 字节长度 `L`，当前 NAL 范围为 `[Current+4, Current+4+L]`，下一 NAL 位置为 `Current+4+L`。
3. **定位插入点**：
   * 解析每个 NAL 的 Header（H.264: 1 byte, H.265: 2 bytes）。
   * 检查 NAL Type 是否为 EOS (End of Stream) 或 EOSeq (End of Sequence)。
     * H.264 EOS: 10, EOSeq: 11
     * H.265 EOS: 36, EOSeq: 37
   * 如果遇到 EOS/EOSeq，记录当前位置为插入点，停止寻找。
4. **构建结果**：
   * 拷贝插入点之前的所有数据。
   * **插入 SEI**：
     * 遍历输入的 SEI 列表。
     * 根据检测到的格式，将 SEI 封装（添加起始码或长度前缀）并写入结果。
   * 拷贝插入点之后的所有数据（包括 EOS/EOSeq 及其后的数据）。
   * 如果未找到 EOS/EOSeq，则将 SEI 追加到数据末尾。

### 3.3 提取逻辑 (extractSEIData)

**流程：**

1. **检测格式**。
2. **遍历 NAL Units**（同上）。
3. **识别 SEI**：
   * 解析 NAL Header。
   * 检查 NAL Type 是否为 SEI。
     * H.264 SEI: 6
     * H.265 SEI: 39 (Prefix), 40 (Suffix)
4. **提取**：
   * 如果是 SEI，将该 NAL Unit 的完整数据（含起始码或长度前缀）拷贝到结果列表中。

## 4. 代码实现细节

### 4.1 格式检测实现

```cpp
static bool isAnnexB(const uint8_t* data, size_t size) {
    if (size < 3) return false;
    if (data[0] == 0x00 && data[1] == 0x00 && data[2] == 0x01) return true;
    if (size >= 4 && data[0] == 0x00 && data[1] == 0x00 && data[2] == 0x00 && data[3] == 0x01) return true;
    return false;
}
```

### 4.2 长度前缀遍历

```cpp
size_t offset = 0;
while (offset + 4 < size) {
    uint32_t val = (data[offset] << 24) | (data[offset+1] << 16) | (data[offset+2] << 8) | data[offset+3];
    size_t nalLen = val;

    // 处理 NAL: data + offset + 4, 长度 nalLen

    offset += 4 + nalLen;
}
```

### 4.3 封装 SEI

在插入时，需要将纯净的 SEI payload（或 RBSP/EBSP）封装成对应的格式。

* **Annex B**: 插入 `00 00 00 01` + `NAL Header` + `SEI Data`。
* **Length Prefix**: 计算 `1 + SEI Data Size` (H.264) 或 `2 + SEI Data Size` (H.265)，写入 4 字节大端长度，再写入 Header 和 Data。

**约定**：本方案假定传入的 `seiNals` 为 **EBSP 数据（包含 NAL Header，不包含 Start Code 或 Length Prefix）**。

## 5. 接口定义

```cpp
/**
* @brief 在H264、H265压缩帧数据nals中插入sei的nal数据
* @param nals - H264、H265压缩帧数据
* @param nalsSize - H264、H265压缩帧数据长度
* @param seiNals - sei的nal数据 (应为 EBSP，即含 NAL Header，不含 StartCode/Length)
* @param isH265 - 输入nals数据是否是H265
* @return 带sei的nal数据的H264、H265压缩帧数据
*/
static std::vector<uint8_t> insertSEIData(const uint8_t* nals, size_t nalsSize, const std::vector<HJRawBuffer>& seiNals, bool isH265 = true);

/**
* @brief 从H264、H265压缩帧数据nals中提取sei的nal数据
* @param nals - H264、H265压缩帧数据
* @param nalsSize - H264、H265压缩帧数据长度
* @param isH265 - 输入nals数据是否是H265
* @return sei的nal数据 (返回完整格式，含 StartCode 或 Length)
*/
static std::vector<HJRawBuffer> extractSEIData(const uint8_t* nals, size_t nalsSize, bool isH265 = true);
```
