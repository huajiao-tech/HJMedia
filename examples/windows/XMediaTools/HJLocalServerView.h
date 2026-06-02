//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJUtils.h"
#include "HJMedias.h"
#include "HJCores.h"
#include "HJGuis.h"

NS_HJ_BEGIN
//***********************************************************************************//
struct HJDBCategoryInfo;

typedef struct HJUIMediaItemInfo
{
	std::string url{};
	std::string rid{};
	int64_t valid_size{};
	int64_t total_length{};
	// int    progress{};
	int 	result{HJ_OK};
} HJUIMediaItemInfo;

class HJLocalServerView : public HJView
{
public:
	HJ_DECLARE_PUWTR(HJLocalServerView);
	explicit HJLocalServerView();
	virtual ~HJLocalServerView();

	virtual int init(const std::string info) override;
	virtual int draw(const HJRectf& rect) override;
	virtual void done();

	int openLocalIOKit();
	void closeLocalIOKit();
protected:
	virtual int drawStatusBar(const HJRectf& rect);
	virtual int drawDBInfo(const HJRectf& rect);
	virtual int drawMediaList(const HJRectf& rect);

	void onClickDownload(const std::string& tips, const std::string& url);
    void onClickPlay(const std::string& tips, const std::string& url);

	int downloadMedia(const std::string& url);
	int cancelMedia(const std::string& url);
    int playMedia(const std::string& url);
    int stopMedia(const std::string& url);

	std::optional<HJUIMediaItemInfo> getMediaItemInfo(const std::string& url) {
		HJ_AUTOU_LOCK(m_mutex);
		auto it = m_mediaItemInfos.find(url);
		if (it != m_mediaItemInfos.end()) {
			return it->second;
		}
		return std::nullopt;
	}
	void updateMediaItemInfo(const std::string& url, const std::string& rid, int64_t valid_size, int64_t total_length, int progress = -1, int result = HJ_OK) {
		HJ_AUTOU_LOCK(m_mutex);
		auto it = m_mediaItemInfos.find(url);
		if (it != m_mediaItemInfos.end()) {
			auto&info = it->second;
			info.url = url;
			info.rid = rid;
			if(valid_size > 0) {
				info.valid_size = valid_size;
			}
			info.total_length = total_length;
			info.result = result;
			return;
		}
        m_mediaItemInfos[url] = HJUIMediaItemInfo{url, rid, valid_size, total_length, result};

		return;
	}

	int procMediaNotify(const HJNotification::Ptr& ntf);

private:
	HJImageButton::Ptr		m_btnClose{};
	bool					m_showDBInfoWindow{false};
	std::shared_ptr<std::vector<HJDBCategoryInfo>> m_categroyInfos{};
	std::unordered_map<std::string, HJUIMediaItemInfo> m_mediaItemInfos{};
	std::mutex				m_mutex{};

	std::vector<std::string>				m_mediaUrls{};
	std::vector<HJComplexImageButton::Ptr>	m_downloadButtons{};
	std::vector<HJComplexImageButton::Ptr>	m_playButtons{};
};
NS_HJ_END