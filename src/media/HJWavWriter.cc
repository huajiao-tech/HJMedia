//***********************************************************************************//
//HJMediaKit FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJWavWriter.h"
#include <iostream>

NS_HJ_BEGIN

HJWavWriter::HJWavWriter(const std::string& filename, int sampleRate, int channels, int bitsPerSample)
    : m_filename(filename), m_sampleRate(sampleRate), m_channels(channels), m_bitsPerSample(bitsPerSample), m_dataSize(0) {
    m_file.open(m_filename, std::ios::binary);
    if (m_file.is_open()) {
        writeHeader();
    }
}

HJWavWriter::~HJWavWriter() {
    if (m_file.is_open()) {
        updateHeader();
        m_file.close();
    }
}

bool HJWavWriter::write(const uint8_t* data, size_t size) {
    if (!m_file.is_open() || !data || size == 0) {
        return false;
    }
    m_file.write(reinterpret_cast<const char*>(data), size);
    m_dataSize += size;
    return true;
}

void HJWavWriter::writeHeader() {
    // RIFF header
    m_file.write("RIFF", 4);
    uint32_t fileSize = 0; // Placeholder
    m_file.write(reinterpret_cast<const char*>(&fileSize), 4);
    m_file.write("WAVE", 4);

    // fmt chunk
    m_file.write("fmt ", 4);
    uint32_t fmtChunkSize = 16;
    m_file.write(reinterpret_cast<const char*>(&fmtChunkSize), 4);
    uint16_t audioFormat = 1; // PCM
    m_file.write(reinterpret_cast<const char*>(&audioFormat), 2);
    uint16_t numChannels = static_cast<uint16_t>(m_channels);
    m_file.write(reinterpret_cast<const char*>(&numChannels), 2);
    uint32_t sampleRate = static_cast<uint32_t>(m_sampleRate);
    m_file.write(reinterpret_cast<const char*>(&sampleRate), 4);
    uint32_t byteRate = m_sampleRate * m_channels * m_bitsPerSample / 8;
    m_file.write(reinterpret_cast<const char*>(&byteRate), 4);
    uint16_t blockAlign = m_channels * m_bitsPerSample / 8;
    m_file.write(reinterpret_cast<const char*>(&blockAlign), 2);
    uint16_t bitsPerSample = static_cast<uint16_t>(m_bitsPerSample);
    m_file.write(reinterpret_cast<const char*>(&bitsPerSample), 2);

    // data chunk
    m_file.write("data", 4);
    uint32_t dataSize = 0; // Placeholder
    m_file.write(reinterpret_cast<const char*>(&dataSize), 4);
}

void HJWavWriter::updateHeader() {
    if (!m_file.is_open()) return;

    // Update file size (RIFF chunk size)
    // File size - 8 bytes (RIFF + size)
    uint32_t fileSize = static_cast<uint32_t>(m_dataSize + 44 - 8);
    m_file.seekp(4, std::ios::beg);
    m_file.write(reinterpret_cast<const char*>(&fileSize), 4);

    // Update data size
    uint32_t dataSize = static_cast<uint32_t>(m_dataSize);
    m_file.seekp(40, std::ios::beg);
    m_file.write(reinterpret_cast<const char*>(&dataSize), 4);
}

NS_HJ_END
