//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJUtils.h"
#include "HJLog.h"
#include "HJNotify.h"
#include "HJMediaUtils.h"

NS_HJ_BEGIN
//***********************************************************************************//
class HJImage : public HJObject
{
public:
	using Ptr = std::shared_ptr<HJImage>;
	explicit HJImage();
	virtual ~HJImage();

	virtual int init(const std::string& filename);
	virtual void done();
	virtual uint64_t bind();
	virtual void unbind();

	uint8_t* getData() {
		return m_data;
	}
	const int getWidth() {
		return m_width;
	}
	const int getHeight() {
		return m_height;
	}
protected:
	uint8_t*	m_data = NULL;
	int			m_width = 0;
	int			m_height = 0;
	int			m_nrChannels = 0;
	uint64_t	m_texID = 0;
};

class HJGuiImageManager : public HJObject
{
public:
	HJ_INSTANCE_DECL(HJGuiImageManager);

	HJImage::Ptr load(const std::string& filename);
private:
	std::unordered_map<std::string, HJImage::Ptr> m_images;
};

NS_HJ_END