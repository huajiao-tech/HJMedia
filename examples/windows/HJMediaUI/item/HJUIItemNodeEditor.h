#pragma once

#include "HJPrerequisites.h"
#include "HJUIBaseItem.h"
#include <glad/gl.h> //#define GLAD_GL_IMPLEMENTATION  only implementation once;
#include "HJNodePipelineEditor.h"
#include "HJRteGraphSetupInfo.h"
#include <mutex>
#include <unordered_map>
#include <cstdint>

class HJFaceDetectWrapper;

NS_HJ_BEGIN

class HJNodePipelineEditor;
class HJRenderCvt;
class VisualDataBase;

class HJUIItemNodeEditor : public HJUIBaseItem
{
public:
	    
	HJ_DEFINE_CREATE(HJUIItemNodeEditor);
    
	HJUIItemNodeEditor();
	virtual ~HJUIItemNodeEditor();

	virtual int run() override;
    
private:
	void priDone();
    void priApplyNodeParams(const NodeData& node);
	void installBridgeCallback(const NodeData& node, const std::shared_ptr<VisualDataBase>& visualData);
	std::shared_ptr<HJFaceDetectWrapper> getOrCreateFaceDetect(const std::string& insName);
	std::shared_ptr<HJNodePipelineEditor> m_nodePipelineEditor = nullptr;
	std::shared_ptr<HJRenderCvt> m_renderCvt = nullptr;
    std::string nodeRoleToString(NodeRole role) const;
    HJRteJsonLinkInfo makeLinkInfo(const LinkData& link) const;
	char m_exportPath[260] = "pipeline.json";
	char m_importPath[260] = "pipeline.json";
	std::vector<char> m_exportJsonPreview;
	bool m_relinkOnDelete = false;

	//static std::string s_imageSeq; 
	//static std::string s_pngGiftSeq;
	static int s_uiWidth;
	static int s_uiHeight;

	void priTest();
	void enqueueLinkRenderEvent(const std::string& linkId, const std::string& fromInsName, const std::string& toInsName, bool i_bOnlyCopy);
	void flushLinkRenderEvents();
	void enqueueFrameStatEvent(int64_t timeMs, double fps, double renderAvgMs);
	void flushFrameStatEvents();
	void pushStatSample(double x, double frameMs, double fps);
	void drawStatPlot();

	int m_idx = 0;

	struct FaceDetectState
	{
		std::shared_ptr<HJFaceDetectWrapper> detect = nullptr;
		uint64_t generation = 0;
	};
	mutable std::mutex m_faceDetectMutex;
	std::unordered_map<std::string, FaceDetectState> m_faceDetectStates;
	mutable std::mutex m_faceDetectLifeMutex;
	std::unordered_map<std::string, uint64_t> m_faceDetectGenerationByIns;

	struct LinkRenderEvent
	{
		std::string linkId;
		std::string fromInsName;
		std::string toInsName;
		bool bOnlyCopy = false;
	};
	mutable std::mutex m_linkRenderMutex;
	std::vector<LinkRenderEvent> m_pendingLinkRenderEvents;
	mutable std::mutex m_frameStatMutex;
	struct FrameStatEvent
	{
		int64_t timeMs = 0;
		double fps = 0.0;
		double renderAvgMs = 0.0;
	};
	std::vector<FrameStatEvent> m_pendingFrameStatEvents;
	double m_latestRenderAvgMs = 0.0;
	double m_latestRenderFps = 0.0;

	bool m_showStatsPlot = false;
	size_t m_statMaxSamples = 300;
	std::vector<double> m_statXs;
	std::vector<double> m_statFrameMs;
	std::vector<double> m_statFps;
	mutable std::mutex m_faceDetectStatMutex;
	std::unordered_map<std::string, double> m_faceDetectElapsedMsByIns;
	std::unordered_map<std::string, std::vector<double>> m_statFaceDetectSeriesByIns;
};

NS_HJ_END



