//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJPackerManager.h"
#include "HJFLog.h"
#include "HJException.h"

NS_HJ_BEGIN
//***********************************************************************************//
static inline void be_write_u32(uint8_t* p, uint32_t v)
{
    p[0] = static_cast<uint8_t>((v >> 24) & 0xFF);
    p[1] = static_cast<uint8_t>((v >> 16) & 0xFF);
    p[2] = static_cast<uint8_t>((v >> 8) & 0xFF);
    p[3] = static_cast<uint8_t>((v) & 0xFF);
}

static inline uint32_t be_read_u32(const uint8_t* p)
{
    return (static_cast<uint32_t>(p[0]) << 24) |
           (static_cast<uint32_t>(p[1]) << 16) |
           (static_cast<uint32_t>(p[2]) << 8)  |
           (static_cast<uint32_t>(p[3]));
}

static inline uint8_t make_flags(HJCompressType c, HJXVibDataType t)
{
    uint8_t compress = static_cast<uint8_t>(c) & 0x03;
    uint8_t dtype = static_cast<uint8_t>(t) & 0x03;
    return static_cast<uint8_t>((0u << 4) | (dtype << 2) | compress);
}

static inline void parse_flags(uint8_t flags, HJCompressType& c, HJXVibDataType& t, uint8_t& reserved)
{
    c = static_cast<HJCompressType>(flags & 0x03);
    t = static_cast<HJXVibDataType>((flags >> 2) & 0x03);
    reserved = static_cast<uint8_t>((flags >> 4) & 0x0F);
}

bool HJPackerManager::writeHeader(const HJXVibHeader& h, std::vector<uint8_t>& out10)
{
    out10.resize(HJXVIB_HEADER_SIZE);
    out10[0] = h.version;
    out10[1] = make_flags(h.compress, h.data_type);
    out10[2] = h.data_offset; // == 10
    be_write_u32(&out10[3], h.data_len);
    out10[7] = h.seg_id;
    out10[8] = h.seg_count;
    out10[9] = h.seq_id;
    return true;
}

bool HJPackerManager::readHeader(const uint8_t* p, size_t n, HJXVibHeader& h)
{
    if (n < HJXVIB_HEADER_SIZE) return false;
    h.version = p[0];
    uint8_t flags = p[1];
    parse_flags(flags, h.compress, h.data_type, h.reserved);
    h.data_offset = p[2];
    h.data_len = be_read_u32(p + 3);
    h.seg_id = p[7];
    h.seg_count = p[8];
    h.seq_id = p[9];
    return true;
}

HJVibeData::Ptr HJPackerManager::pack(const HJVibeData::Ptr& data)
{
    std::shared_lock lock(m_packersMutex);
    
    if(m_packers.empty()) {
        HJFLogw("There are not any packers in the manager, please register them first");
        return {};
    }
    if(!data || data->size() < 1) {
        HJFLogw("Input data for pack is null or empty");
        return {};
    }
    //HJFLogi("Packing data...");
    uint8_t seq_id = data->getInt("seq_id");

    HJCompressType compress = HJCompressType::COMPRESS_NONE;
    HJVibeData::Ptr tmp = data;
    for(auto& packer : m_packers) {
        tmp = packer->pack(tmp);
        if(!tmp) {
            HJFLoge("Packer {} returned null data while packing", packer->getName());
            return {};
        }
    }
    if(tmp->haveValue("compress")) {
        compress = (HJCompressType)tmp->getInt("compress");
    }
    HJXVibDataType dataType = HJXVibDataType::VIBDATA_RAW;
    size_t origin_size = tmp->getSizet("origin_size");

    HJVibeData::Ptr result = HJCreates<HJVibeData>();
    uint8_t seg_count = static_cast<uint8_t>(tmp->size());
    for (size_t i = 0; i < tmp->size(); i++) 
    {
        const auto& src = tmp->get(i);
        //
        HJXVibHeader h;
        h.version = HJXVIB_VERSION;
        h.compress = compress;
        h.data_type = dataType;
        h.reserved = 0;
        h.data_offset = HJXVIB_HEADER_SIZE;
        h.data_len = origin_size;
        h.seg_id = static_cast<uint8_t>(i);
        h.seg_count = seg_count;
        h.seq_id = seq_id;

        //
        std::vector<uint8_t> header;
        writeHeader(h, header);

        HJUserData::Ptr userData = HJCreates<HJUserData>();
        userData->append(header);
        userData->append(src->getData());

        result->append(userData);
    }
    
    return result;
}

HJVibeData::Ptr HJPackerManager::unpack(const HJVibeData::Ptr& data)
{
    std::shared_lock lock(m_packersMutex);
    
    if(m_packers.empty()) {
        HJFLogw("There are not any packers in the manager, please register them first");
        return {};
    }
    if(!data) {
        HJFLogw("Input data for unpack is null");
        return {};
    }
    HJFLogi("Unpacking data...");
    // struct Item { uint8_t seg_id; std::vector<uint8_t> payload; HJXVibHeader h; };
    // std::vector<Item> items; items.reserve(data->size());

    HJVibeData::Ptr result = HJCreates<HJVibeData>();

    HJXVibHeader ref{};
    bool ref_set = false;
    HJVibeData::Ptr tmp = data;
    for (size_t i = 0; i < tmp->size(); i++) {
        const auto& src = data->get(i);
        HJXVibHeader h;
        if (!readHeader(src->data(), src->size(), h))  {
            HJFLoge("invalid header");
            return {};
        }
        if (h.data_offset < HJXVIB_HEADER_SIZE || src->size() < h.data_offset) {
            HJFLoge("invalid data offset");
            return {};
        }
        if (!ref_set) { 
            ref = h; 
            ref_set = true;
            //
            (*result)["origin_size"] = (size_t)h.data_len;
        } else if (h.version != ref.version || h.compress != ref.compress || h.data_type != ref.data_type){
            HJFLoge("invalid header: version{}, compress:{} data_type:{}", h.version, (int)h.compress, (int)h.data_type);
             return {};
        }

        size_t off = h.data_offset;
        std::vector<uint8_t> payload(src->getData().begin() + off, src->getData().end());
        // items.push_back(Item{ h.seg_id, std::move(payload), h });

        HJUserData::Ptr userData = HJCreates<HJUserData>(payload);
        userData->setID(h.seg_id);
        userData->setOriginSize(h.data_len);
        //
        result->append(userData);
    }

    if (ref.seg_count == 0 || ref.seg_count > MAX_SEG_COUNT || result->size() != ref.seg_count) {
        HJFLoge("invalid seg_count:{}", ref.seg_count);
        return {};
    }

    // std::sort(items.begin(), items.end(), [](const Item& a, const Item& b){ return a.seg_id < b.seg_id; });
    // for (size_t i = 0; i < items.size(); ++i) {
    //     if (items[i].seg_id != static_cast<uint8_t>(i)) return {};
    // }    

    for(auto it = m_packers.rbegin(); it != m_packers.rend(); ++it) {
        auto& packer = *it;
        result = packer->unpack(result);
        if(!result) {
            HJFLoge("Packer {} returned null data while unpacking", packer->getName());
            return {};
        }
    }
    return result;
}

NS_HJ_END