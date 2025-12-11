//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR: 
//CREATE TIME: 
//***********************************************************************************//
#pragma once
#include "HJUtilitys.h"
#include "HJMediaStorageUtils.h"
#include "HJXIOFile.h"
#include "HJBlockFile.h"
#include "HJDownloader.h"
#include "HJNetFetch.h"
#include "HJExecutor.h"
#include "HJFileUtil.h"
#include "HJTime.h"
#include <optional>
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <condition_variable>

NS_HJ_BEGIN
class HJMediaStorageManager;
//***********************************************************************************//
class HJCacheFile : public HJXIOBase, public HJNetFetchDelegate
{
public:
	HJ_DECLARE_PUWTR(HJCacheFile);

    HJCacheFile();
    virtual ~HJCacheFile();

	virtual int open(HJUrl::Ptr murl) override;
	virtual void close() override;
	virtual int read(void* buffer, size_t cnt) override;
	virtual int write(const void* buffer, size_t cnt) override;
	virtual int seek(int64_t offset, int whence = std::ios::cur) override;
	virtual int flush() override;
	virtual int64_t tell() override;
	virtual int64_t size() override;
    virtual bool eof() override;

    void setStorageManager(const std::shared_ptr<HJMediaStorageManager>& manager) {
        m_storageManager = manager;
    }
private:
    void getOptions(const HJUrl::Ptr& murl);
	int ensureLocalFile();
    int ensureFetcher();
    void schedulePrefetch();
    void prefetchHeadLocked(size_t bytes);
    void prefetchTailLocked();
	void requestBlock(int blockIndex, bool highPriority = false);
    void enqueueBlockLocked(int blockIndex, bool highPriority);
	void scheduleFetchLoop();
	void fetchLoop();
	int fetchBlock(int blockIndex);
	bool writeToLocal(int64_t offset, const HJBuffer::Ptr& data);
    void handleDownloadedChunk(int64_t offset, const uint8_t* data, size_t length);
    void markBlockCompleteLocked(int blockIndex);
    void persistDBInfoLocked();
    void updateFileStatusLocked();
    bool isBlockReadyLocked(int blockIndex) const;
    int blockCount() const;
    int blockIndexByOffset(int64_t offset) const;
    int64_t blockStartOffset(int blockIndex) const;
    size_t blockLength(int blockIndex) const;
    //
    HJNetFetch::Ptr createFetch();
    int fetch(std::tuple<size_t, size_t, size_t> range);

	virtual int onNetFetchNotify(HJNotification::Ptr ntfy) override;
private:
    std::string 				m_rid{""};
    std::string 				m_cacheDir{""};
    int 						m_preCacheSize{0};
    std::string                 m_category{""};
    float                       m_tailPercentage{0.05f};
	std::optional<HJDBFileInfo> m_dbinfo;
	//
	HJFileStatus				m_fileStatus{HJFileStatus::NONE};
	std::string					m_remoteUrl{""};
	std::string					m_localUrl{""};
	HJNetFetch::Ptr 			m_fetch{nullptr};
	HJXIOFile::Utr              m_localFile{nullptr};
    size_t                      m_blockSize{HJ_FILE_BLOCK_SIZE_DEFAULT};
    int64_t                     m_fileLength{0};
    size_t						m_pos = 0;
	//
	HJExecutor::Ptr 			m_executor{nullptr};
    std::weak_ptr<HJMediaStorageManager> m_storageManager;
    std::mutex                  m_mutex;
    std::mutex                  m_fileMutex;
    std::condition_variable     m_blockCv;
    std::deque<int>             m_blockQueue;
    std::unordered_set<int>     m_blockPending;
    std::unordered_map<int, size_t> m_blockProgress;
    bool                        m_fetching{false};
    bool                        m_stop{false};
};

NS_HJ_END
