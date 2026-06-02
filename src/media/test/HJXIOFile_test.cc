#include <gtest/gtest.h>
#include "HJXIOFile.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>

NS_HJ_USING;

namespace {
std::string makeTempFilePath(const std::string& prefix) {
    std::error_code ec;
    std::filesystem::path temp_dir = std::filesystem::temp_directory_path(ec);
    if (ec) {
        return "";
    }
    static std::atomic<uint32_t> file_index{0};
    const uint32_t index = ++file_index;
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    std::string filename = prefix + "_" + std::to_string(stamp) + "_" + std::to_string(index) + ".bin";
    return (temp_dir / filename).string();
}

struct ScopedFile {
    explicit ScopedFile(const std::string& path_in) : path(path_in) {}
    ~ScopedFile() {
        if (path.empty()) {
            return;
        }
        std::error_code ec;
        std::filesystem::remove(path, ec);
    }
    std::string path;
};

bool writeFile(const std::string& path, const std::string& data) {
    if (path.empty()) {
        return false;
    }
    std::ofstream ofs(path, std::ios::binary | std::ios::trunc);
    if (!ofs.is_open()) {
        return false;
    }
    ofs.write(data.data(), static_cast<std::streamsize>(data.size()));
    return ofs.good();
}

bool readFile(const std::string& path, size_t size, std::string* out) {
    if (!out || path.empty()) {
        return false;
    }
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs.is_open()) {
        return false;
    }
    out->assign(size, '\0');
    ifs.read(&(*out)[0], static_cast<std::streamsize>(size));
    return static_cast<size_t>(ifs.gcount()) == size;
}
} // namespace

TEST(HJXIOFileTest, WriteModeTruncatesFile) {
    const std::string path = makeTempFilePath("hjxio_write_truncate");
    ASSERT_FALSE(path.empty());
    ScopedFile guard(path);
    ASSERT_TRUE(writeFile(path, "0123456789"));

    HJXIOFile::Ptr file = std::make_shared<HJXIOFile>();
    HJUrl::Ptr url = HJUrl::makeUrl(path, HJ_XIO_WRITE);
    ASSERT_EQ(file->open(url), HJ_OK);

    const std::string data = "abc";
    ASSERT_EQ(file->write(data.data(), data.size()), static_cast<int>(data.size()));
    file->close();

    std::error_code ec;
    const auto size = std::filesystem::file_size(path, ec);
    ASSERT_FALSE(ec);
    EXPECT_EQ(static_cast<size_t>(size), data.size());

    std::string out;
    ASSERT_TRUE(readFile(path, data.size(), &out));
    EXPECT_EQ(out, data);
}

TEST(HJXIOFileTest, SeekSyncsReadWritePointers) {
    const std::string path = makeTempFilePath("hjxio_seek_sync");
    ASSERT_FALSE(path.empty());
    ScopedFile guard(path);
    ASSERT_TRUE(writeFile(path, "0123456789"));

    HJXIOFile::Ptr file = std::make_shared<HJXIOFile>();
    HJUrl::Ptr url = HJUrl::makeUrl(path, HJ_XIO_RW);
    ASSERT_EQ(file->open(url), HJ_OK);

    ASSERT_EQ(file->seek(5, std::ios::beg), HJ_OK);
    const char replace = 'X';
    ASSERT_EQ(file->write(&replace, 1), 1);
    file->close();

    std::string out;
    ASSERT_TRUE(readFile(path, 10, &out));
    EXPECT_EQ(out, "01234X6789");
}

TEST(HJXIOFileTest, TellUsesLastIO) {
    const std::string path = makeTempFilePath("hjxio_tell_last");
    ASSERT_FALSE(path.empty());
    ScopedFile guard(path);
    ASSERT_TRUE(writeFile(path, "abcdef"));

    HJXIOFile::Ptr file = std::make_shared<HJXIOFile>();
    HJUrl::Ptr url = HJUrl::makeUrl(path, HJ_XIO_RW);
    ASSERT_EQ(file->open(url), HJ_OK);

    const char replace = 'Z';
    ASSERT_EQ(file->write(&replace, 1), 1);
    EXPECT_EQ(file->tell(), 1);

    ASSERT_EQ(file->seek(0, std::ios::beg), HJ_OK);
    char buffer[2] = {0};
    ASSERT_EQ(file->read(buffer, sizeof(buffer)), 2);
    EXPECT_EQ(file->tell(), 2);
    EXPECT_EQ(buffer[0], 'Z');
    EXPECT_EQ(buffer[1], 'b');
    file->close();
}
