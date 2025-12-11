#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "src/media/sei/HJPackerManager.h"
#include "src/media/sei/HJXVibeTypes.h"

using namespace HJ;

namespace {

HJVibeData::Ptr makeVibeData(const std::string& text) {
    auto vibe = std::make_shared<HJVibeData>();
    auto payload = std::make_shared<HJUserData>(std::vector<uint8_t>(text.begin(), text.end()));
    vibe->append(payload);
    return vibe;
}

std::string toString(const HJVibeData::Ptr& data) {
    if (!data || data->size() == 0) {
        return {};
    }
    const auto& userData = data->getData().front()->getData();
    return std::string(userData.begin(), userData.end());
}

class RecordingPackerBase : public HJBasePacker {
public:
    RecordingPackerBase(const HJParams::Ptr& params,
                        std::string id,
                        char token)
        : HJBasePacker(params),
          m_id(std::move(id)),
          m_token(token) {}

    HJVibeData::Ptr pack(const HJVibeData::Ptr& data) override {
        s_packCalls.push_back(m_id);
        return mutate(data, true);
    }

    HJVibeData::Ptr unpack(const HJVibeData::Ptr& data) override {
        s_unpackCalls.push_back(m_id);
        return mutate(data, false);
    }

    static void reset() {
        s_packCalls.clear();
        s_unpackCalls.clear();
    }

    static const std::vector<std::string>& packCalls() {
        return s_packCalls;
    }
    static const std::vector<std::string>& unpackCalls() {
        return s_unpackCalls;
    }

protected:
    HJVibeData::Ptr mutate(const HJVibeData::Ptr& data, bool append) const {
        if (!data || data->size() == 0) {
            return {};
        }
        auto userData = data->getData().front();
        auto bytes = userData->getData();
        if (append) {
            bytes.push_back(static_cast<uint8_t>(m_token));
        } else {
            if (bytes.empty() || bytes.back() != static_cast<uint8_t>(m_token)) {
                return {};
            }
            bytes.pop_back();
        }
        userData->setData(bytes);
        return data;
    }

private:
    std::string m_id;
    char m_token;

    static std::vector<std::string> s_packCalls;
    static std::vector<std::string> s_unpackCalls;
};

std::vector<std::string> RecordingPackerBase::s_packCalls;
std::vector<std::string> RecordingPackerBase::s_unpackCalls;

class AlphaPacker : public RecordingPackerBase {
public:
    explicit AlphaPacker(const HJParams::Ptr& params)
        : RecordingPackerBase(params, "Alpha", 'A') {}
};

class BetaPacker : public RecordingPackerBase {
public:
    explicit BetaPacker(const HJParams::Ptr& params)
        : RecordingPackerBase(params, "Beta", 'B') {}
};

class CustomTokenPacker : public RecordingPackerBase {
public:
    CustomTokenPacker(const HJParams::Ptr& params, char token)
        : RecordingPackerBase(params, std::string("Custom-") + token, token) {}
};

template <typename Packer, typename... Args>
struct PackerDescriptor {
    using first_type = Packer;
    using second_type = std::tuple<Args...>;

    explicit PackerDescriptor(Args&&... args)
        : second(std::forward<Args>(args)...) {}

    second_type second;
};

template <typename Packer, typename... Args>
PackerDescriptor<Packer, Args...> MakeDescriptor(Args&&... args) {
    return PackerDescriptor<Packer, Args...>(std::forward<Args>(args)...);
}

}  // namespace

TEST(HJPackerManagerTest, PackAndUnpackPipelineMaintainsInitialPayload) {
    RecordingPackerBase::reset();
    HJPackerManager manager;
    manager.registerPackers<AlphaPacker, BetaPacker>();

    auto payload = makeVibeData("payload");
    auto packed = manager.pack(payload);
    ASSERT_TRUE(packed);
    EXPECT_EQ("payloadAB", toString(packed));

    auto unpacked = manager.unpack(packed);
    ASSERT_TRUE(unpacked);
    EXPECT_EQ("payload", toString(unpacked));

    const std::vector<std::string> expectedPack{"Alpha", "Beta"};
    const std::vector<std::string> expectedUnpack{"Beta", "Alpha"};
    EXPECT_EQ(expectedPack, RecordingPackerBase::packCalls());
    EXPECT_EQ(expectedUnpack, RecordingPackerBase::unpackCalls());
}

TEST(HJPackerManagerTest, PackReturnsNullWhenInputIsNull) {
    RecordingPackerBase::reset();
    HJPackerManager manager;
    manager.registerPackers<AlphaPacker>();

    auto result = manager.pack(nullptr);
    EXPECT_FALSE(result);
    EXPECT_TRUE(RecordingPackerBase::packCalls().empty());
}

TEST(HJPackerManagerTest, RegisterPackersWithArgumentsCreatesCustomTokens) {
    RecordingPackerBase::reset();
    HJPackerManager manager;
    manager.registerPackers(MakeDescriptor<CustomTokenPacker>('Z'));

    auto payload = makeVibeData("data");
    auto packed = manager.pack(payload);
    ASSERT_TRUE(packed);
    EXPECT_EQ("dataZ", toString(packed));

    auto unpacked = manager.unpack(packed);
    ASSERT_TRUE(unpacked);
    EXPECT_EQ("data", toString(unpacked));

    const std::vector<std::string> expected{"Custom-Z"};
    EXPECT_EQ(expected, RecordingPackerBase::packCalls());
    EXPECT_EQ(expected, RecordingPackerBase::unpackCalls());
}

TEST(HJPackerManagerTest, WriteAndReadHeaderRoundTrip) {
    HJXVibHeader header;
    header.version = 3;
    header.compress = HJCompressType::COMPRESS_LZ4;
    header.data_type = HJXVibDataType::VIBDATA_JSON;
    header.data_len = 123456;
    header.seg_id = 2;
    header.seg_count = 4;
    header.seq_id = 42;

    std::vector<uint8_t> buffer;
    EXPECT_TRUE(HJPackerManager::writeHeader(header, buffer));
    ASSERT_EQ(10u, buffer.size());

    HJXVibHeader parsed;
    ASSERT_TRUE(HJPackerManager::readHeader(buffer.data(), buffer.size(), parsed));
    EXPECT_EQ(header.version, parsed.version);
    EXPECT_EQ(header.compress, parsed.compress);
    EXPECT_EQ(header.data_type, parsed.data_type);
    EXPECT_EQ(header.data_len, parsed.data_len);
    EXPECT_EQ(header.seg_id, parsed.seg_id);
    EXPECT_EQ(header.seg_count, parsed.seg_count);
    EXPECT_EQ(header.seq_id, parsed.seq_id);
}
