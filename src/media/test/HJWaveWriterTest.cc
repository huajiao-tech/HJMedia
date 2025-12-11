#include <gtest/gtest.h>
#include <fstream>
#include <vector>
#include "../HJWavWriter.h"

using namespace HJ;

class HJWavWriterTest : public testing::Test {
protected:
    std::string testFile = "test_audio.wav";

    void TearDown() override {
        remove(testFile.c_str());
    }
};

TEST_F(HJWavWriterTest, WriteWaveFile) {
    int sampleRate = 44100;
    int channels = 2;
    int bitsPerSample = 16;
    
    {
        HJWavWriter writer(testFile, sampleRate, channels, bitsPerSample);
        std::vector<uint8_t> dummyData(1024, 0x00); // 1KB of silence
        bool res = writer.write(dummyData.data(), dummyData.size());
        EXPECT_TRUE(res);
    } // Destructor called here, header updated

    // Verify file content
    std::ifstream file(testFile, std::ios::binary);
    ASSERT_TRUE(file.is_open());

    // Check RIFF header
    char buffer[4];
    file.read(buffer, 4);
    EXPECT_EQ(std::string(buffer, 4), "RIFF");

    uint32_t fileSize;
    file.read(reinterpret_cast<char*>(&fileSize), 4);
    // File size should be 44 (header) + 1024 (data) - 8 = 1060
    EXPECT_EQ(fileSize, 1060);

    file.read(buffer, 4);
    EXPECT_EQ(std::string(buffer, 4), "WAVE");
    
    // Check fmt chunk
    file.read(buffer, 4);
    EXPECT_EQ(std::string(buffer, 4), "fmt ");

    // Skip to data chunk
    file.seekg(40, std::ios::beg);
    uint32_t dataSize;
    file.read(reinterpret_cast<char*>(&dataSize), 4);
    EXPECT_EQ(dataSize, 1024);

    file.close();
}
