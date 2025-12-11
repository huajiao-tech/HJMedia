//***********************************************************************************//
//HJMediaKit FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include <fstream>
#include <string>
#include "HJUtilitys.h"
NS_HJ_BEGIN
//***********************************************************************************//
class HJWavWriter : public HJObject {
public:
    HJ_DECLARE_PUWTR(HJWavWriter);
    HJWavWriter(const std::string& filename, int sampleRate = 48000, int channels = 2, int bitsPerSample = 16);
    ~HJWavWriter();

    bool write(const uint8_t* data, size_t size);

private:
    void writeHeader();
    void updateHeader();

private:
    std::string m_filename;
    int m_sampleRate;
    int m_channels;
    int m_bitsPerSample;
    std::ofstream m_file;
    size_t m_dataSize;
};

NS_HJ_END
