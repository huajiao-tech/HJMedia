#include <gtest/gtest.h>

#include "HJBlockData.h"
#include "HJDataSource.h"
#include "HJFileUtil.h"
#include "HJLocalFileManager.h"
#include "HJDataSourceKit.h"
#include "HJMediaUtils.h"
#include "HJUtilitys.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#define HJ_TEST_HAVE_DATA_SOURCE 1
#include "HJAVIOContext.h"
#include <thread>
#include <random>

NS_HJ_USING;

namespace {
struct FileContext {
    std::string remote_path;
    std::string remote_url;
    std::string rid;
    std::string local_url;
};

std::string makeTempDirPath(const std::string& prefix) {
    std::error_code ec;
    std::filesystem::path temp_dir = std::filesystem::temp_directory_path(ec);
    if (ec || temp_dir.empty()) {
        temp_dir = std::filesystem::path("hjmedia_tmp");
    }
    static std::atomic<uint32_t> dir_index{0};
    const uint32_t index = ++dir_index;
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    std::string dir_name = prefix + "_" + std::to_string(stamp) + "_" + std::to_string(index);
    return (temp_dir / dir_name).string();
}

std::vector<uint8_t> makePatternData(size_t size, uint8_t seed) {
    std::vector<uint8_t> data(size, 0);
    for (size_t i = 0; i < size; ++i) {
        data[i] = static_cast<uint8_t>(seed + static_cast<uint8_t>(i));
    }
    return data;
}

bool writeBinaryFile(const std::string& path, const std::vector<uint8_t>& data) {
    if (path.empty()) {
        return false;
    }
    std::ofstream ofs(path, std::ios::binary | std::ios::trunc);
    if (!ofs.is_open()) {
        return false;
    }
    if (!data.empty()) {
        ofs.write(reinterpret_cast<const char*>(data.data()),
                  static_cast<std::streamsize>(data.size()));
    }
    return ofs.good();
}

FileContext makeFileContext(const std::string& work_dir, const std::string& filename) {
    FileContext ctx;
    ctx.remote_path = HJUtilitys::concatenatePath(work_dir, filename); //(std::filesystem::path(work_dir) / filename).string();
    std::string normalized = HJUtilitys::convertBackslashesToForward(ctx.remote_path);
    ctx.remote_url = "file:" + normalized;
    ctx.rid = HJMediaUtils::makeUrlRid(ctx.remote_url);
    ctx.local_url = HJMediaUtils::makeLocalUrl(work_dir, ctx.remote_url, ctx.rid);
    return ctx;
}

HJUrl::Ptr makeOpenUrl(const FileContext& ctx, const std::string& local_dir) {
    auto url = std::make_shared<HJUrl>(ctx.remote_url, HJ_XIO_READ);
    (*url)["rid"] = ctx.rid;
    (*url)["local_dir"] = local_dir;
    return url;
}

struct Segment {
    size_t offset;
    size_t size;
};

std::vector<Segment> makeSegments(size_t total_size, size_t min_size, size_t max_size, uint32_t seed) {
    std::vector<Segment> segments;
    if (total_size == 0) {
        return segments;
    }
    min_size = (std::max)(static_cast<size_t>(1), min_size);
    max_size = (std::max)(min_size, max_size);

    std::mt19937 rng(seed);
    std::uniform_int_distribution<size_t> dist(min_size, max_size);

    size_t offset = 0;
    while (offset < total_size) {
        size_t remaining = total_size - offset;
        size_t chunk = dist(rng);
        if (chunk > remaining) {
            chunk = remaining;
        }
        segments.push_back({offset, chunk});
        offset += chunk;
    }
    return segments;
}

bool readExact(HJDataSource& source, uint8_t* buffer, size_t size) {
    if (!buffer || size == 0) {
        return true;
    }
    size_t total = 0;
    while (total < size) {
        int read_bytes = source.read(buffer + total, size - total);
        if (read_bytes <= 0) {
            return false;
        }
        total += static_cast<size_t>(read_bytes);
    }
    return true;
}
} // namespace

class HJDataSourceTest : public testing::Test {
protected:
    void SetUp() override {
        m_work_dir = makeTempDirPath("hj_datasource");
        ASSERT_FALSE(m_work_dir.empty());
        std::filesystem::create_directories(m_work_dir);

        HJCacheOptions cache_opts;
        cache_opts.emplace_back(m_work_dir, 10);
        auto params = HJCreates<HJParams>();
        (*params)["medias_dir"] = m_work_dir;
        (*params)["cache_opts"] = cache_opts;

        auto kit = HJDataSourceKit::getInstance();
        ASSERT_TRUE(kit != nullptr);
        ASSERT_EQ(kit->init(params, nullptr), HJ_OK);
        m_file_manager = kit->getFileManager();
        ASSERT_TRUE(m_file_manager != nullptr);
    }

    void TearDown() override {
        auto kit = HJDataSourceKit::getInstance();
        if (kit) {
            kit->done();
        }
        HJDataSourceKit::destoryInstance();

        std::error_code ec;
        if (!m_work_dir.empty()) {
            std::filesystem::remove_all(m_work_dir, ec);
        }
    }

    HJDBFileInfo makeDbInfo(const FileContext& ctx, int64_t total_length) {
        HJDBFileInfo info(ctx.rid, ctx.remote_url, ctx.local_url, total_length,
                          static_cast<int>(HJBlockData::kBlockSize));
        info.ensure();
        return info;
    }

    void updateFileInfo(const HJDBFileInfo& info) {
        ASSERT_EQ(m_file_manager->updateFileInfo(info), HJ_OK);
    }

    std::string m_work_dir;
    HJLocalFileManager::Ptr m_file_manager;
};

#if HJ_TEST_HAVE_DATA_SOURCE
TEST_F(HJDataSourceTest, OpenRejectsNullOrEmptyUrl) {
    HJDataSource source;
    EXPECT_EQ(source.open(nullptr), HJErrInvalidParams);

    auto empty_url = std::make_shared<HJUrl>("", HJ_XIO_READ);
    EXPECT_EQ(source.open(empty_url), HJErrInvalidParams);
}

TEST_F(HJDataSourceTest, ReadFromCachedUsesLocalFile) {
    const size_t data_size = 4096;
    auto ctx = makeFileContext(m_work_dir, "cached_source.bin");
    auto remote_data = makePatternData(data_size, 0x10);
    auto cached_data = makePatternData(data_size, 0x80);

    ASSERT_TRUE(writeBinaryFile(ctx.remote_path, remote_data));
    ASSERT_TRUE(writeBinaryFile(ctx.local_url, cached_data));

    HJDBFileInfo info = makeDbInfo(ctx, static_cast<int64_t>(data_size));
    info.setBlockStatusAndUpdateSize(0, true);
    if(info.isComplete()) {
        info.status = static_cast<int>(HJFileStatus::COMPLETED);
    }
    HJDBSpanInfo span(0, 1, ctx.local_url);
    info.spans.clear();
    info.spans.push_back(span);
    updateFileInfo(info);

    auto open_url = makeOpenUrl(ctx, m_work_dir);
    HJDataSource source;
    ASSERT_EQ(source.open(open_url), HJ_OK);
    EXPECT_EQ(source.size(), static_cast<int64_t>(data_size));

    EXPECT_EQ(source.read(nullptr, data_size), 0);
    std::vector<uint8_t> buffer(data_size, 0);
    EXPECT_EQ(source.read(buffer.data(), 0), 0);

    int bytes_read = source.read(buffer.data(), buffer.size());
    EXPECT_EQ(bytes_read, static_cast<int>(buffer.size()));
    EXPECT_EQ(buffer, cached_data);
    EXPECT_EQ(source.tell(), static_cast<int64_t>(data_size));
    EXPECT_TRUE(source.eof());
    source.close();
}

TEST_F(HJDataSourceTest, ReadFromLockedCachesToLocalFile) {
    const size_t data_size = 2048;
    auto ctx = makeFileContext(m_work_dir, "locked_source.bin");
    auto remote_data = makePatternData(data_size, 0x21);

    ASSERT_TRUE(writeBinaryFile(ctx.remote_path, remote_data));
    HJFileUtil::removeFile(ctx.local_url);

    HJDBFileInfo info = makeDbInfo(ctx, static_cast<int64_t>(data_size));
    HJDBSpanInfo span(ctx.local_url);
    span.block_start_idx = 0;
    span.block_cnt = 0;
    info.spans.clear();
    info.spans.push_back(span);
    updateFileInfo(info);

    auto open_url = makeOpenUrl(ctx, m_work_dir);
    HJDataSource source;
    ASSERT_EQ(source.open(open_url), HJ_OK);

    std::vector<uint8_t> buffer(data_size, 0);
    int bytes_read = source.read(buffer.data(), buffer.size());
    EXPECT_EQ(bytes_read, static_cast<int>(buffer.size()));
    EXPECT_EQ(buffer, remote_data);
    source.close();

    EXPECT_TRUE(HJFileUtil::compareFile(ctx.remote_path, ctx.local_url));

    auto blob = m_file_manager->getShareBlob(ctx.remote_url, m_work_dir, ctx.rid);
    ASSERT_TRUE(blob != nullptr);
    EXPECT_EQ(blob->getBlockStatus(0), HJBlockData::BlockStatus::CACHED);
}

TEST_F(HJDataSourceTest, ReadFromAloneSkipsCaching) {
    const size_t data_size = 1024;
    auto ctx = makeFileContext(m_work_dir, "alone_source.bin");
    auto remote_data = makePatternData(data_size, 0x33);

    ASSERT_TRUE(writeBinaryFile(ctx.remote_path, remote_data));
    HJFileUtil::removeFile(ctx.local_url);

    HJDBFileInfo info = makeDbInfo(ctx, static_cast<int64_t>(data_size));
    HJDBSpanInfo span(ctx.local_url);
    span.block_start_idx = 0;
    span.block_cnt = 0;
    info.spans.clear();
    info.spans.push_back(span);
    updateFileInfo(info);

    auto blob = m_file_manager->getShareBlob(ctx.remote_url, m_work_dir, ctx.rid);
    ASSERT_TRUE(blob != nullptr);
    auto locked_block = blob->getBlock(0);
    ASSERT_TRUE(locked_block != nullptr);
    ASSERT_EQ(locked_block->getStatus(), HJBlockData::BlockStatus::LOCKED);

    auto open_url = makeOpenUrl(ctx, m_work_dir);
    HJDataSource source;
    ASSERT_EQ(source.open(open_url), HJ_OK);

    std::vector<uint8_t> buffer(data_size, 0);
    int bytes_read = source.read(buffer.data(), buffer.size());
    EXPECT_EQ(bytes_read, static_cast<int>(buffer.size()));
    EXPECT_EQ(buffer, remote_data);
    source.close();

    std::error_code ec;
    uint64_t local_size = 0;
    if (std::filesystem::exists(ctx.local_url, ec) && !ec) {
        local_size = static_cast<uint64_t>(std::filesystem::file_size(ctx.local_url, ec));
    }
    EXPECT_EQ(local_size, 0u);
    EXPECT_EQ(blob->getBlockStatus(0), HJBlockData::BlockStatus::LOCKED);

    blob->releaseBlock(0);
}

TEST_F(HJDataSourceTest, SeekUpdatesPositionAndReadsCachedData) {
    const size_t data_size = 8192;
    const size_t start_pos = 256;
    const size_t read_len = 128;

    auto ctx = makeFileContext(m_work_dir, "seek_source.bin");
    auto cached_data = makePatternData(data_size, 0x44);
    ASSERT_TRUE(writeBinaryFile(ctx.remote_path, cached_data));
    ASSERT_TRUE(writeBinaryFile(ctx.local_url, cached_data));

    HJDBFileInfo info = makeDbInfo(ctx, static_cast<int64_t>(data_size));
    info.setBlockStatusAndUpdateSize(0, true);
    HJDBSpanInfo span(0, 1, ctx.local_url);
    info.spans.clear();
    info.spans.push_back(span);
    updateFileInfo(info);

    auto open_url = makeOpenUrl(ctx, m_work_dir);
    HJDataSource source;
    ASSERT_EQ(source.open(open_url), HJ_OK);
    EXPECT_EQ(source.size(), static_cast<int64_t>(data_size));

    EXPECT_EQ(source.seek(static_cast<int64_t>(start_pos), std::ios::beg), 0);
    EXPECT_EQ(source.tell(), static_cast<int64_t>(start_pos));

    std::vector<uint8_t> buffer(read_len, 0);
    int bytes_read = source.read(buffer.data(), buffer.size());
    EXPECT_EQ(bytes_read, static_cast<int>(buffer.size()));
    for (size_t i = 0; i < read_len; ++i) {
        EXPECT_EQ(buffer[i], cached_data[start_pos + i]);
    }

    EXPECT_EQ(source.seek(-1, std::ios::beg), -1);
    EXPECT_EQ(source.tell(), static_cast<int64_t>(start_pos + read_len));
    EXPECT_FALSE(source.eof());
    source.close();
}

TEST_F(HJDataSourceTest, ReadAllLocalFile) {
    const size_t data_size = HJBlockData::kBlockSize * 2 + 257;
    auto ctx = makeFileContext(m_work_dir, "all_source.bin");
    auto data = makePatternData(data_size, 0x52);

    ASSERT_TRUE(writeBinaryFile(ctx.remote_path, data));
    HJFileUtil::removeFile(ctx.local_url);

    auto open_url = makeOpenUrl(ctx, m_work_dir);
    HJDataSource source;
    ASSERT_EQ(source.open(open_url), HJ_OK);
    EXPECT_EQ(source.size(), static_cast<int64_t>(data_size));

    std::vector<uint8_t> buffer(data_size, 0);
    ASSERT_TRUE(readExact(source, buffer.data(), buffer.size()));
    EXPECT_EQ(buffer, data);

    EXPECT_EQ(source.tell(), static_cast<int64_t>(data_size));
    EXPECT_TRUE(source.eof());
    source.close();
}

TEST_F(HJDataSourceTest, ReadSegmentsSequentiallyReconstructsFile) {
    const size_t data_size = HJBlockData::kBlockSize * 3 + 123;
    auto ctx = makeFileContext(m_work_dir, "segs_in_order.bin");
    auto data = makePatternData(data_size, 0x60);

    ASSERT_TRUE(writeBinaryFile(ctx.remote_path, data));
    HJFileUtil::removeFile(ctx.local_url);

    auto segments = makeSegments(data_size, 512, HJBlockData::kBlockSize + 2048, 0x1234);

    auto open_url = makeOpenUrl(ctx, m_work_dir);
    HJDataSource source;
    ASSERT_EQ(source.open(open_url), HJ_OK);

    std::vector<uint8_t> combined;
    combined.reserve(data_size);
    for (const auto& seg : segments) {
        ASSERT_EQ(seg.offset, combined.size());
        std::vector<uint8_t> buffer(seg.size);
        ASSERT_TRUE(readExact(source, buffer.data(), buffer.size()));
        combined.insert(combined.end(), buffer.begin(), buffer.end());
    }

    EXPECT_EQ(combined.size(), data_size);
    EXPECT_EQ(combined, data);
    source.close();
}

TEST_F(HJDataSourceTest, ReadSegmentsOutOfOrderReconstructsFile) {
    const size_t data_size = HJBlockData::kBlockSize * 3 + 511;
    auto ctx = makeFileContext(m_work_dir, "segs_out_of_order.bin");
    auto data = makePatternData(data_size, 0x6a);

    ASSERT_TRUE(writeBinaryFile(ctx.remote_path, data));
    HJFileUtil::removeFile(ctx.local_url);

    auto segments = makeSegments(data_size, 256, HJBlockData::kBlockSize + 1024, 0x5678);
    auto shuffled = segments;
    std::mt19937 rng(0x9abc);
    std::shuffle(shuffled.begin(), shuffled.end(), rng);

    auto open_url = makeOpenUrl(ctx, m_work_dir);
    HJDataSource source;
    ASSERT_EQ(source.open(open_url), HJ_OK);

    std::vector<uint8_t> combined(data_size, 0);
    for (const auto& seg : shuffled) {
        ASSERT_EQ(source.seek(static_cast<int64_t>(seg.offset), std::ios::beg), 0);
        std::vector<uint8_t> buffer(seg.size);
        ASSERT_TRUE(readExact(source, buffer.data(), buffer.size()));
        std::copy(buffer.begin(), buffer.end(), combined.begin() + seg.offset);
    }

    EXPECT_EQ(combined, data);
    source.close();
}

TEST_F(HJDataSourceTest, ReadSegmentsWithMultipleInstances) {
    const size_t data_size = HJBlockData::kBlockSize * 4 + 321;
    auto ctx = makeFileContext(m_work_dir, "multi_instance.bin");
    auto data = makePatternData(data_size, 0x71);

    ASSERT_TRUE(writeBinaryFile(ctx.remote_path, data));
    ASSERT_TRUE(writeBinaryFile(ctx.local_url, data));

    HJDBFileInfo info = makeDbInfo(ctx, static_cast<int64_t>(data_size));
    int block_count = static_cast<int>(info.blockCount());
    for (int i = 0; i < block_count; ++i) {
        info.setBlockStatusAndUpdateSize(i, true);
    }
    if (info.isComplete()) {
        info.status = static_cast<int>(HJFileStatus::COMPLETED);
    }
    info.spans.clear();
    info.spans.push_back(HJDBSpanInfo(0, block_count, ctx.local_url));
    updateFileInfo(info);

    const int instance_count = 3;
    std::vector<std::unique_ptr<HJDataSource>> sources;
    sources.reserve(instance_count);
    for (int i = 0; i < instance_count; ++i) {
        auto source = std::make_unique<HJDataSource>();
        auto open_url = makeOpenUrl(ctx, m_work_dir);
        ASSERT_EQ(source->open(open_url), HJ_OK);
        sources.push_back(std::move(source));
    }

    std::vector<uint8_t> combined(data_size, 0);
    size_t offset = 0;
    size_t segment_index = 0;
    while (offset < data_size) {
        size_t segment_size = (std::min)(HJBlockData::kBlockSize, data_size - offset);
        auto& source = sources[segment_index % sources.size()];
        ASSERT_EQ(source->seek(static_cast<int64_t>(offset), std::ios::beg), 0);
        std::vector<uint8_t> buffer(segment_size);
        ASSERT_TRUE(readExact(*source, buffer.data(), buffer.size()));
        std::copy(buffer.begin(), buffer.end(), combined.begin() + offset);
        offset += segment_size;
        ++segment_index;
    }

    for (auto& source : sources) {
        source->close();
    }

    EXPECT_EQ(combined, data);
}

TEST_F(HJDataSourceTest, ReadSegmentsWithThreadsDifferentRegions) {
    const size_t data_size = HJBlockData::kBlockSize * 4 + 777;
    auto ctx = makeFileContext(m_work_dir, "multi_thread.bin");
    auto data = makePatternData(data_size, 0x7a);

    ASSERT_TRUE(writeBinaryFile(ctx.remote_path, data));
    ASSERT_TRUE(writeBinaryFile(ctx.local_url, data));

    HJDBFileInfo info = makeDbInfo(ctx, static_cast<int64_t>(data_size));
    int block_count = static_cast<int>(info.blockCount());
    for (int i = 0; i < block_count; ++i) {
        info.setBlockStatusAndUpdateSize(i, true);
    }
    if (info.isComplete()) {
        info.status = static_cast<int>(HJFileStatus::COMPLETED);
    }
    info.spans.clear();
    info.spans.push_back(HJDBSpanInfo(0, block_count, ctx.local_url));
    updateFileInfo(info);

    std::vector<Segment> segments;
    size_t offset = 0;
    while (offset < data_size) {
        size_t segment_size = (std::min)(HJBlockData::kBlockSize, data_size - offset);
        segments.push_back({offset, segment_size});
        offset += segment_size;
    }

    const int thread_count = 4;
    std::vector<std::vector<Segment>> thread_segments(thread_count);
    for (size_t i = 0; i < segments.size(); ++i) {
        thread_segments[i % thread_count].push_back(segments[i]);
    }

    std::vector<uint8_t> combined(data_size, 0);
    std::atomic<int> errors{0};
    std::vector<std::thread> threads;
    threads.reserve(thread_count);
    for (int t = 0; t < thread_count; ++t) {
        threads.emplace_back([&, t]() {
            auto open_url = makeOpenUrl(ctx, m_work_dir);
            HJDataSource source;
            if (source.open(open_url) != HJ_OK) {
                errors.fetch_add(1);
                return;
            }
            for (const auto& seg : thread_segments[t]) {
                if (source.seek(static_cast<int64_t>(seg.offset), std::ios::beg) != 0) {
                    errors.fetch_add(1);
                    break;
                }
                std::vector<uint8_t> buffer(seg.size);
                if (!readExact(source, buffer.data(), buffer.size())) {
                    errors.fetch_add(1);
                    break;
                }
                std::copy(buffer.begin(), buffer.end(), combined.begin() + seg.offset);
            }
            source.close();
        });
    }
    for (auto& th : threads) {
        th.join();
    }

    EXPECT_EQ(errors.load(), 0);
    EXPECT_EQ(combined, data);
}

TEST_F(HJDataSourceTest, ReadPastEOFReturnsRemainingAndSetsEof) {
    const size_t data_size = HJBlockData::kBlockSize + 123;
    auto ctx = makeFileContext(m_work_dir, "read_past_eof.bin");
    auto data = makePatternData(data_size, 0x8f);

    ASSERT_TRUE(writeBinaryFile(ctx.remote_path, data));
    HJFileUtil::removeFile(ctx.local_url);

    auto open_url = makeOpenUrl(ctx, m_work_dir);
    HJDataSource source;
    ASSERT_EQ(source.open(open_url), HJ_OK);

    std::vector<uint8_t> buffer(data_size + 256, 0);
    int bytes_read = source.read(buffer.data(), buffer.size());
    EXPECT_EQ(bytes_read, static_cast<int>(data_size));
    EXPECT_TRUE(std::equal(buffer.begin(), buffer.begin() + data_size, data.begin()));
    EXPECT_EQ(source.tell(), static_cast<int64_t>(data_size));
    EXPECT_TRUE(source.eof());

    int second_read = source.read(buffer.data(), 16);
    EXPECT_EQ(second_read, 0);
    source.close();
}

TEST_F(HJDataSourceTest, SeekFromEndReadsTail) {
    const size_t data_size = HJBlockData::kBlockSize + 77;
    const size_t tail_size = 64;
    auto ctx = makeFileContext(m_work_dir, "seek_from_end.bin");
    auto data = makePatternData(data_size, 0x93);

    ASSERT_TRUE(writeBinaryFile(ctx.remote_path, data));
    HJFileUtil::removeFile(ctx.local_url);

    auto open_url = makeOpenUrl(ctx, m_work_dir);
    HJDataSource source;
    ASSERT_EQ(source.open(open_url), HJ_OK);

    ASSERT_EQ(source.seek(-static_cast<int64_t>(tail_size), std::ios::end), 0);
    std::vector<uint8_t> buffer(tail_size, 0);
    ASSERT_TRUE(readExact(source, buffer.data(), buffer.size()));
    for (size_t i = 0; i < tail_size; ++i) {
        EXPECT_EQ(buffer[i], data[data_size - tail_size + i]);
    }
    source.close();
}

TEST_F(HJDataSourceTest, ReadAcrossCachedAndRemoteBlocksCachesMissing) {
    const size_t data_size = HJBlockData::kBlockSize + 17;
    auto ctx = makeFileContext(m_work_dir, "mixed_cached_remote.bin");
    auto remote_data = makePatternData(data_size, 0x22);
    auto local_block0 = makePatternData(HJBlockData::kBlockSize, 0xb0);

    ASSERT_TRUE(writeBinaryFile(ctx.remote_path, remote_data));
    ASSERT_TRUE(writeBinaryFile(ctx.local_url, local_block0));

    HJDBFileInfo info = makeDbInfo(ctx, static_cast<int64_t>(data_size));
    info.setBlockStatusAndUpdateSize(0, true);
    info.spans.clear();
    info.spans.push_back(HJDBSpanInfo(0, 1, ctx.local_url));
    updateFileInfo(info);

    auto open_url = makeOpenUrl(ctx, m_work_dir);
    HJDataSource source;
    ASSERT_EQ(source.open(open_url), HJ_OK);

    std::vector<uint8_t> buffer(data_size, 0);
    int bytes_read = source.read(buffer.data(), buffer.size());
    EXPECT_EQ(bytes_read, static_cast<int>(data_size));

    std::vector<uint8_t> expected = remote_data;
    std::copy(local_block0.begin(), local_block0.end(), expected.begin());
    EXPECT_EQ(buffer, expected);
    source.close();

    auto blob = m_file_manager->getShareBlob(ctx.remote_url, m_work_dir, ctx.rid);
    ASSERT_TRUE(blob != nullptr);
    EXPECT_EQ(blob->getBlockStatus(1), HJBlockData::BlockStatus::CACHED);
}

TEST_F(HJDataSourceTest, CachedBlockMissingLocalFileReturnsError) {
    const size_t data_size = 1024;
    auto ctx = makeFileContext(m_work_dir, "cached_missing_local.bin");
    auto remote_data = makePatternData(data_size, 0x55);
    auto cached_data = makePatternData(data_size, 0x99);

    ASSERT_TRUE(writeBinaryFile(ctx.remote_path, remote_data));
    ASSERT_TRUE(writeBinaryFile(ctx.local_url, cached_data));

    HJDBFileInfo info = makeDbInfo(ctx, static_cast<int64_t>(data_size));
    info.setBlockStatusAndUpdateSize(0, true);
    HJDBSpanInfo span(0, 1, ctx.local_url);
    info.spans.clear();
    info.spans.push_back(span);
    updateFileInfo(info);

    auto open_url = makeOpenUrl(ctx, m_work_dir);
    HJDataSource source;
    ASSERT_EQ(source.open(open_url), HJ_OK);

    HJFileUtil::removeFile(ctx.local_url);
    ASSERT_FALSE(HJFileUtil::isFileExist(ctx.local_url));

    std::vector<uint8_t> buffer(data_size, 0);
    int bytes_read = source.read(buffer.data(), buffer.size());
    EXPECT_EQ(bytes_read, HJErrIO);
    source.close();
}
#endif
