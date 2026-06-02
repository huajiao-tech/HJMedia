#pragma once

#include "HJPrerequisites.h"
#include "HJComUtils.h"
#include "HJNotify.h"
#include "HJRenderFaceuNotify.h"

//typedef struct GLFWwindow GLFWwindow;

NS_HJ_BEGIN

class HJOGFBOCtrl;
class HJPBOReadWrapper;
class HJRenderFaceuObject;
class HJMoreFacePointsReal;
class HJSPBuffer;


class HJRenderFaceuBaseMgr 
{
public:
    HJRenderFaceuBaseMgr();
    virtual ~HJRenderFaceuBaseMgr();

    virtual int init(HJFaceuListener i_listener);
	virtual int render(int i_width, int i_height, std::shared_ptr<HJMoreFacePointsReal> i_points, unsigned char*& o_pRGBA);
    virtual int add(const std::string& i_uniqueKey, const std::string &i_url, bool i_bDebugPoints);
    virtual int remove(const std::string& i_uniqueKey);
    virtual int removeAll();
    virtual void done();

    void setDebugPoints(bool i_bDebugPoints)
	{
        m_bDebugPoints = i_bDebugPoints;
	}
    bool getDebugPoints() const
	{
        return m_bDebugPoints;
	}

private:
	HJFaceuListener m_listener = nullptr;
    std::unordered_map<std::string, std::shared_ptr<HJRenderFaceuObject>> m_map;
    std::shared_ptr<HJMoreFacePointsReal> m_catchPoints = nullptr;

    int64_t m_statIdx = 0;
    int64_t m_statSum = 0;
	bool m_bDebugPoints = false;
};

class HJRenderFaceuEnvMgr : public HJRenderFaceuBaseMgr
{
public:
	HJRenderFaceuEnvMgr();
	virtual ~HJRenderFaceuEnvMgr();

	virtual int init(HJFaceuListener i_listener) override;
	virtual int add(const std::string& i_uniqueKey, const std::string& i_url, bool i_bDebugPoints) override;
	virtual int remove(const std::string& i_uniqueKey) override;
	virtual int removeAll() override;
	virtual void done() override;
	virtual int render(int i_width, int i_height, std::shared_ptr<HJMoreFacePointsReal> i_points, unsigned char*& o_pRGBA) override;
private:
	void priTryCreatePBO();
	bool priIsAdjustResolution(int i_width, int i_height);

	int m_width = 0;
	int m_height = 0;
	std::shared_ptr<HJOGFBOCtrl> m_fboCtrl = nullptr;
	std::shared_ptr<HJPBOReadWrapper> m_pbReadWrapper = nullptr;
	std::shared_ptr<HJSPBuffer> m_outBuffer = nullptr;
};

NS_HJ_END





