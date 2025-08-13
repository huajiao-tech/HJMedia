//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJRotatingFile.h"
#include "spdlog/details/file_helper.h"
#include "spdlog/details/fmt_helper.h"
#include "spdlog/common.h"
#include "HJException.h"

using namespace spdlog;

NS_HJ_BEGIN
//***********************************************************************************//
class HJRotatingFileImpl : public HJRotatingFile
{
public:
	using Ptr = std::shared_ptr<HJRotatingFileImpl>;
	HJRotatingFileImpl(const std::string& filename, size_t maxFileSize = 10 * 1024 * 1024, size_t maxFiles = 2, bool rotate_on_open = false, const file_event_handlers& event_handlers = {});
	virtual ~HJRotatingFileImpl();

	virtual int write(const std::string& msg) override;
    virtual void flush() override;

private:
    filename_t calcFilename(const filename_t& filename, std::size_t index);
    void rotate();
    bool rename_file(const filename_t& src_filename, const filename_t& target_filename);
private:
	details::file_helper	m_file_helper;
	std::size_t				m_current_size{0};
	std::recursive_mutex	m_mutex;
	std::size_t 		    m_block_size{ 0 };
	std::size_t			    m_block_max{ 4 * 1024 };
};

HJRotatingFileImpl::HJRotatingFileImpl(const std::string& filename, size_t maxFileSize, size_t maxFiles, bool rotate_on_open, const file_event_handlers& event_handlers)
	: HJRotatingFile(filename, maxFileSize, maxFiles)
	, m_file_helper(event_handlers)
{
	if (maxFileSize <= 0 || maxFiles > 20000) {
		HJ_EXCEPT(HJException::ERR_INVALIDPARAMS, "create rotating file failed");
	}
	m_file_helper.open(calcFilename(m_filename, 0));
	m_current_size = m_file_helper.size(); // expensive. called only once
	if (rotate_on_open && m_current_size > 0)
	{
		rotate();
		m_current_size = 0;
	}
    m_block_size = m_current_size %  m_block_max;
}

HJRotatingFileImpl::~HJRotatingFileImpl()
{

}

int HJRotatingFileImpl::write(const std::string& msg)
{
    HJ_AUTOU_LOCK(m_mutex);
	m_block_size += msg.size();
    auto new_size = m_current_size + msg.size();
    if (new_size > m_maxFileSize)
    {
        m_file_helper.flush();
        if (m_file_helper.size() > 0)
        {
            rotate();
            new_size = m_block_size = msg.size();
        }
    }
    memory_buf_t formatted;
    details::fmt_helper::append_string_view(msg, formatted);
    details::fmt_helper::append_string_view(spdlog::details::os::default_eol, formatted);
    m_file_helper.write(formatted);
    m_current_size = new_size;
    //
    if (m_block_size > m_block_max)
    {
        m_file_helper.flush();
        m_block_size = m_current_size % m_block_max;
    }

    return 0;
}

void HJRotatingFileImpl::flush() {
    HJ_AUTOU_LOCK(m_mutex);
    m_file_helper.flush();
    m_block_size = 0;
}

filename_t HJRotatingFileImpl::calcFilename(const filename_t& filename, std::size_t index)
{
	if (index == 0u) {
		return filename;
	}

	filename_t basename, ext;
	std::tie(basename, ext) = details::file_helper::split_by_extension(filename);
	return fmt_lib::format(SPDLOG_FILENAME_T("{}.{}{}"), basename, index, ext);
}

// Rotate files:
// log.txt -> log.1.txt
// log.1.txt -> log.2.txt
// log.2.txt -> log.3.txt
// log.3.txt -> delete
void HJRotatingFileImpl::rotate()
{
    using details::os::filename_to_str;
    using details::os::path_exists;

    m_file_helper.close();
    for (auto i = m_maxFiles; i > 0; --i)
    {
        filename_t src = calcFilename(m_filename, i - 1);
        if (!path_exists(src))
        {
            continue;
        }
        filename_t target = calcFilename(m_filename, i);

        if (!rename_file(src, target))
        {
            // if failed try again after a small delay.
            // this is a workaround to a windows issue, where very high rotation
            // rates can cause the rename to fail with permission denied (because of antivirus?).
            details::os::sleep_for_millis(100);
            if (!rename_file(src, target))
            {
                m_file_helper.reopen(true); // truncate the log file anyway to prevent it to grow beyond its limit!
                m_maxFileSize = 0;
                throw_spdlog_ex("rotating_file_sink: failed renaming " + filename_to_str(src) + " to " + filename_to_str(target), errno);
            }
        }
    }
    m_file_helper.reopen(true);
}

// delete the target if exists, and rename the src file  to target
// return true on success, false otherwise.
bool HJRotatingFileImpl::rename_file(const filename_t& src_filename, const filename_t& target_filename)
{
    // try to delete the target file in case it already exists.
    (void)details::os::remove(target_filename);
    return details::os::rename(src_filename, target_filename) == 0;
}

//***********************************************************************************//
HJRotatingFile::HJRotatingFile(const std::string& filename, size_t maxFileSize, size_t maxFiles)
    : m_filename(filename)
    , m_maxFileSize(maxFileSize)
    , m_maxFiles(maxFiles)
{

}
HJRotatingFile::~HJRotatingFile()
{

}

HJRotatingFile::Ptr HJRotatingFile::create(const std::string& filename, size_t maxFileSize, size_t maxFiles)
{
    return std::make_shared<HJRotatingFileImpl>(filename, maxFileSize, maxFiles);
}

//***********************************************************************************//
NS_HJ_END
