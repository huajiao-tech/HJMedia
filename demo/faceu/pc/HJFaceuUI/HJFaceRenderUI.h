#pragma once

#include "HJPrerequisites.h"
#include "HJComUtils.h"
#include <glad/gl.h>

#define TWO_HANDLE 0

NS_HJ_BEGIN

class HJTimerThreadPool;
class HJFaceRenderUI : public HJBaseObject
{
public:

	HJ_DEFINE_CREATE(HJFaceRenderUI);

	HJFaceRenderUI();
	virtual ~HJFaceRenderUI();

	int run();

private:
	static void priHJFaceuCallback(const char* i_uniqueKey, int i_type);
	void priDrawOneAndAlpha(GLuint i_textureId, int i_width, int i_height);

	char m_urls[3][512] = { 0 };
	bool m_bDraw = false;
	std::shared_ptr<HJTimerThreadPool> m_timerThread = nullptr;

	static std::string m_imgSeqUrl;
	static std::string m_faceuUrl0;
	static std::string m_faceuUrl1;
	static std::string m_faceuUrl2;
	bool m_bContainFace = true;
	bool m_bDebugPoint = false;
	bool m_bDrawImgSeq = false;
	bool m_bCreateTexture = false;
	GLuint m_textureFaceuId = 0;
	GLuint m_textureFaceuId1 = 0;
	GLuint m_textureImgSeqId = 0;
	std::vector<std::string> m_imgSeqPaths;
	void *m_handle = nullptr;
#if TWO_HANDLE
	void* m_handle1 = nullptr;
#endif
};
NS_HJ_END



