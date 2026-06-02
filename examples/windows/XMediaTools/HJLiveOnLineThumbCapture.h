#pragma once

#include <cstdint>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <string>

#include "HJMacros.h"

NS_HJ_BEGIN

inline uint64_t buildThumbUrlHash(const std::string& i_url)
{
    uint64_t hash = 1469598103934665603ULL;
    for (size_t i = 0; i < i_url.size(); ++i) {
        hash ^= static_cast<unsigned char>(i_url[i]);
        hash *= 1099511628211ULL;
    }
    return hash;
}

inline std::string buildThumbHashDirName(const std::string& i_url)
{
    std::ostringstream stream;
    stream << std::hex << buildThumbUrlHash(i_url);
    return stream.str();
}

inline std::string buildThumbOutputDir(const std::string& i_root_dir,
                                       const std::string& i_level,
                                       const std::string& i_url)
{
    const std::filesystem::path output_dir =
        std::filesystem::path(i_root_dir) / i_level / buildThumbHashDirName(i_url);
    return output_dir.generic_string();
}

inline std::string buildThumbFilePath(const std::string& i_root_dir,
                                      const std::string& i_level,
                                      const std::string& i_url,
                                      int i_index)
{
    std::ostringstream file_name;
    file_name << "thumb_" << std::setfill('0') << std::setw(2) << i_index << ".jpg";
    const std::filesystem::path file_path =
        std::filesystem::path(buildThumbOutputDir(i_root_dir, i_level, i_url)) / file_name.str();
    return file_path.generic_string();
}

struct HJThumbCaptureSchedule {
    int m_saved_count{ 0 };
    int m_target_count{ 5 };
    int64_t m_interval_ms{ 2000 };
    int64_t m_last_saved_pts_ms{ 0 };
    bool m_has_last_saved_pts{ false };

    bool isCompleted() const
    {
        return m_saved_count >= m_target_count;
    }

    bool shouldSaveFrame(int64_t i_pts_ms) const
    {
        if (isCompleted()) {
            return false;
        }
        if (!m_has_last_saved_pts) {
            return true;
        }
        return i_pts_ms - m_last_saved_pts_ms >= m_interval_ms;
    }

    void markSaved(int64_t i_pts_ms)
    {
        m_last_saved_pts_ms = i_pts_ms;
        m_has_last_saved_pts = true;
        ++m_saved_count;
    }

    void reset()
    {
        m_saved_count = 0;
        m_last_saved_pts_ms = 0;
        m_has_last_saved_pts = false;
    }
};

NS_HJ_END
