#include "HJFaceuInfo.h"
#include "HJFLog.h"
#include "HJBaseUtils.h"
#include "HJThreadPool.h"
#include "stb_image.h"
#include "libyuv.h"
#include "HJComEvent.h"
#include "HJOGBaseShader.h"
#include "HJOGCommon.h"
#include "HJTime.h"
#include "HJRteUtils.h"

#if defined(HarmonyOS)
	#include "HJOGRenderWindowBridge.h"
#endif

#define PI 3.1415926
#define IRadianRatio PI / 180.f

NS_HJ_BEGIN

const int HJFaceuConstants::AnchorAlignType_Eyebrows = 0;
const int HJFaceuConstants::AnchorAlignType_Nose     = 1;
const int HJFaceuConstants::AnchorAlignType_Mouth    = 3;
const int HJFaceuConstants::AnchorAlignType_Fixed    = 2;

const int HJFaceuConstants::RotateType_Eyes          = 0;
const int HJFaceuConstants::RotateType_Fixed         = 1;

const int HJFaceuConstants::ScaleType_EyeDistance    = 0;
const int HJFaceuConstants::ScaleType_Fixed          = 1;

static double sGetFaceAngle(HJPointf leftEye, HJPointf rightEye, int faceDetWidth) {
    float x0 = leftEye.x;//faceDetWidth - leftEye.x;
    float y0 = leftEye.y;
    float x1 = rightEye.x;//faceDetWidth - rightEye.x;
    float y1 = rightEye.y;
    double faceAngle = 0.0f;
    if (y0 > y1) { //right
        if (x0 < x1) {
            faceAngle = std::atan((y0 - y1) / (x1 - x0)) * 180.0f / PI;
        }
        else {
            faceAngle = std::atan((x0 - x1) / (y0 - y1)) * 180.0f / PI + 90.0f;
        }
    }
    else if (y0 < y1) { //left
        if (x0 < x1) {
            faceAngle = -std::atan((y1 - y0) / (x1 - x0)) * 180.0f / PI;
        }
        else {
            faceAngle = -std::atan((x0 - x1) / (y1 - y0)) * 180.f / PI - 90.0f;
        }
    }

    return faceAngle;
}
static HJPointf sGetRotatePoint(HJPointf o, HJPointf a, float angle) {
    HJPointf b{ 0.f, 0.f };
    double angleRatio = angle * IRadianRatio;

    b.x = (a.x - o.x) * (float)std::cos(angleRatio) - (a.y - o.y) * (float)std::sin(angleRatio) + o.x;
    b.y = (a.x - o.x) * (float)std::sin(angleRatio) + (a.y - o.y) * (float)std::cos(angleRatio) + o.y;
    return b;
}
static HJPointf sOnTransformPonits(float mid_x, float mid_y, float w, float h, float anchorOffsetX, float anchorOffsetY, float angle, float scaleRatio, float ratio_w, bool isHJCamera) {
    HJPointf o{ mid_x, mid_y };
    float scale = scaleRatio * ratio_w;

    float mx = isHJCamera ? mid_x + (w / 2 + anchorOffsetX) * scale : mid_x - (w / 2 + anchorOffsetX) * scale;
    float my = mid_y - (h / 2 + anchorOffsetY) * scale;
    HJPointf a{ mx, my };

    HJPointf b = sGetRotatePoint(o, a, angle);

    return b;
}

//////////////////////////////////
int HJFaceuTextureInfo::onParseReady()
{
    int i_err = HJ_OK;
    do
    {
#if defined(HarmonyOS)
        if (getStrategy() == HJFaceuStrategy_HarmonyBridge)
        {
            m_bridge = HJOGRenderWindowBridge::Create();
            m_bridge->setInsName(HJFMT("{}_bridge", imageName));
            i_err = m_bridge->init();
            if (i_err < 0)
            {
                i_err = HJErrInvalidResult;
                break;
            }
        }
#endif
    } while (false);
    return i_err;
}

void HJFaceuTextureInfo::priOnDestroy()
{
#if defined(HarmonyOS)
    if (getStrategy() == HJFaceuStrategy_HarmonyBridge)
    {
        if (m_bridge)
        {
            m_bridge->done();
            m_bridge = nullptr;
        }
    }

#endif
    if ((getStrategy() == HJFaceuStrategy_SharedCtxTexture) || (getStrategy() == HJFaceuStrategy_ExclusiveTexture))
    {
        if (m_bCreateTexture)
        {
            HJOGCommon::textureDestroy(m_textureId);
            m_bCreateTexture = false;
        }
    }
}
int HJFaceuTextureInfo::priUpdate(const HJRawImageDataInfo::Ptr& rawInfo)
{
    int i_err = HJ_OK;
    do
    {
		if (!m_bCreateTexture)
		{
			m_textureId = HJOGCommon::textureCreate();
			m_bCreateTexture = true;
		}

		GLenum internalFormat;
		GLenum dataFormat;
		if (rawInfo->m_components == 3)
		{
			internalFormat = GL_RGB;
			dataFormat = GL_RGB;
		}
		else if (rawInfo->m_components == 4)
		{
			internalFormat = GL_RGBA;
			dataFormat = GL_RGBA;
		}
		HJOGCommon::textureUpload(m_textureId, internalFormat, rawInfo->m_width, rawInfo->m_height, dataFormat, rawInfo->m_buffer->getBuf());
        
    } while (false);
    return i_err;
}
HJFaceuTextureInfo::~HJFaceuTextureInfo()
{
    priOnDestroy();
}
int HJFaceuTextureInfo::update()
{
    int i_err = HJ_OK;
    do
    {
        if (getStrategy() == HJFaceuStrategy_HarmonyBridge)
        {
#if defined(HarmonyOS)
            if (m_bridge->IsStateAvaiable())
            {
                m_bridge->update();
            }
#endif           
        }    
        else if (getStrategy() == HJFaceuStrategy_ExclusiveTexture)
        {
            HJRawImageDataInfo::Ptr rawInfo = m_cache.acquire();
            if (!rawInfo)
            {
                i_err = HJ_WOULD_BLOCK;
                break;
            }
            i_err = priUpdate(rawInfo);
            if (i_err < 0)
            {
                break;
            }
            m_cache.recovery(rawInfo);
        }
    } while (false);
    return i_err;
}
bool HJFaceuTextureInfo::priIsDrawReady()
{
    bool bReady = false;
    do
    {
        if (getStrategy() == HJFaceuStrategy_HarmonyBridge)
        {
#if defined(HarmonyOS)
            bReady = m_bridge->IsStateAvaiable();
#endif
        }
        else
        {
            bReady = m_bCreateTexture;
        }
    } while (false);
    return bReady;
}
int HJFaceuTextureInfo::draw(std::vector<HJPointf> i_points, int width, int height)
{
    int i_err = HJ_OK;
    do
    {
        if (m_bDisable)
        {
            break;
        }
        if (!priIsDrawReady())        
        {
            break;
        }

		int mFaceDetWidth = width;
		int mFaceDetHeight = height;
		HJPointf leftEye = i_points[0];
		HJPointf rightEye = i_points[1];
		HJPointf nose = i_points[2];
		//fixme shang zui chun, but not have this point, so use left and right mouse average; is not precise
		HJPointf top_lip = HJPointf{ (i_points[3].x + i_points[4].x) / 2, (i_points[3].y + i_points[4].y) / 2 };

		double constexpr doubleMin = std::numeric_limits<double>::min();

		bool mIsAsGift = false;

		bool mIsHuajiaoCamera = false;
		int faceUWidth = width;
		int faceUHeight = height;

		double curMidX = 0.5;
		double curMidY = 0.5;
		float ratio_w = faceUWidth / (float)mFaceDetWidth;
		float ratio_h = faceUHeight / (float)mFaceDetHeight;
		bool isLandscape = faceUWidth > faceUHeight;

		float angle = (float)sGetFaceAngle(leftEye, rightEye, mFaceDetWidth);
		float eyeDistance = (float)sqrt((float)((rightEye.x - leftEye.x) * (rightEye.x - leftEye.x) + (rightEye.y - leftEye.y) * (rightEye.y - leftEye.y)));
		int radiusType = radius_Type;
		float radius = (HJFaceuConstants::RotateType_Eyes == radiusType) ? angle : (float)mradius;

		int midType = mid_Type;
		int scaleType = scale_Type;
		float scaleRatio = (HJFaceuConstants::ScaleType_EyeDistance == scaleType) ? eyeDistance / scale_ratio : scale_ratio;
		if (mIsHuajiaoCamera) {
			scaleRatio = (HJFaceuConstants::ScaleType_Fixed == scaleType) ? scale_ratio : eyeDistance / scale_ratio;
		}
		float anchorOffsetX = (float)anchor_offset_x;
		float anchorOffsetY = (float)anchor_offset_y;
		float w = (float)asize_offset_x;
		float h = (float)asize_offset_y;
		float fixed_ratio = isLandscape ? faceUHeight / w : faceUWidth / w;                             //portrait -> landscape, because faceU gift design by portrait
		float base_ratio_x = ratio_w;
		float base_ratio_y = ratio_h;
		bool isTransformPonit = true;
		bool isOrthProjection = mIsHuajiaoCamera ? false : true;// mIsOrthProjection;

		switch (midType)
		{
        case HJFaceuConstants::AnchorAlignType_Eyebrows:
			// 2.1 head type
			curMidX = ((rightEye.x + leftEye.x) / 2.0f) * ratio_w;
			curMidY = faceUHeight - ((rightEye.y + leftEye.y) / 2.0f) * ratio_h;
			break;
		case HJFaceuConstants::AnchorAlignType_Nose:
			// 2.2 nose
			curMidX = nose.x * ratio_w;
			curMidY = faceUHeight - nose.y * ratio_h;
			//HJFLogi("radius:{} radiusType:{} angle:{} x1 y1:{} {} x2 y2:{} {}", radius, radiusType, angle, leftEye.x, leftEye.y, rightEye.x, rightEye.y);
			break;
		case HJFaceuConstants::AnchorAlignType_Fixed: // 2.3 fixed type
			if (isLandscape)
			{
				curMidX = land_new_mid_x;
				if (curMidX <= doubleMin) { //old
					curMidX = (w - mid_x) * scaleRatio * fixed_ratio / 2.0f;
					curMidY = (h - mid_y) * scaleRatio * fixed_ratio / 2.0f;
					//portrait -> landscape, map middle point to landscape
					curMidX = faceUWidth * curMidX / faceUHeight;
					curMidY = faceUHeight * curMidY / faceUWidth;

					isTransformPonit = false;
				}
				else {
					curMidX *= faceUWidth;
					curMidY = land_new_mid_y * faceUHeight;

					scaleRatio = land_scale_ratio;
					anchorOffsetX = land_anchor_offset_x;
					anchorOffsetY = land_anchor_offset_y;
				}
			}
			else {
				curMidX = new_mid_x;
				if (curMidX <= doubleMin) { //old
					if (mIsHuajiaoCamera)
					{
						if (mIsAsGift) {
							curMidX = w * scaleRatio * fixed_ratio / 2.0f;
							curMidY = h * scaleRatio * fixed_ratio / 2.0f;
						}
						else {
							float xn = mid_x;
							float yn = mid_y;
							fixed_ratio = faceUWidth / 720.0f;
							curMidX = (w * xn) * scaleRatio * fixed_ratio;
							curMidY = (h * yn) * scaleRatio * fixed_ratio;
						}
					}
					else {
						curMidX = (w - mid_x) * scaleRatio * fixed_ratio / 2.0f;
						curMidY = (h - mid_y) * scaleRatio * fixed_ratio / 2.0f;
					}

					isTransformPonit = false;
				}
				else {
					curMidX *= faceUWidth;
					curMidY = new_mid_y * faceUHeight;
				}
			}
			//                    if (mirrored) {
			//                        curMidX = faceUWidth - curMidX;
			//                    }

			base_ratio_x = fixed_ratio;
			base_ratio_y = fixed_ratio;

			isOrthProjection = true;
			break;
		case HJFaceuConstants::AnchorAlignType_Mouth:
			// 2.4 mouse type
			curMidX = top_lip.x * ratio_w;
			curMidY = faceUHeight - top_lip.y * ratio_h;
			break;
		default:
			break;
		}

		if (isTransformPonit)
		{
			HJPointf b = sOnTransformPonits(curMidX, curMidY, w, h, anchorOffsetX, anchorOffsetY, radius, scaleRatio, base_ratio_x, mIsHuajiaoCamera);
			curMidX = b.x;
			curMidY = b.y;
		}
		mat4x4 mvp;
		if (isOrthProjection)
		{

			mat4x4_identity(mvp);

			mat4x4 scaleMat;
			mat4x4_identity(scaleMat);

			float yFlip = m_bCreateTexture ? -1.f : 1.f;
			mat4x4_scale_aniso(mvp, scaleMat, w * scaleRatio * base_ratio_x * 0.5, yFlip * h * scaleRatio * base_ratio_y * 0.5, 1);  //*0.5 because, the vertex point is -0.5 -0.5 to 0.5 0.5, if use -1 -1 to 1 1  so  multiple 0.5

			mat4x4 rotateMat;
			mat4x4_identity(rotateMat);
			mat4x4_rotate(rotateMat, rotateMat, 0, 0, 1, radius * PI / 180.f); //radius and rotate is different, sin(radian) cos(radian)

			mat4x4_mul(mvp, rotateMat, mvp);

			mat4x4 translateMat;
			mat4x4_identity(translateMat);
			mat4x4_translate(translateMat, curMidX - (faceUWidth - width) / 2, curMidY - (faceUHeight - height) / 2, 0.f);
			mat4x4_mul(mvp, translateMat, mvp);

			mat4x4 orth;
			mat4x4_ortho(orth, 0, width, 0, height, -1, 1);
			mat4x4_mul(mvp, orth, mvp);

            i_err = priDraw((float*)&mvp);
            if (i_err < 0)
            {
                break;
            }
		}         
    } while (false);
    return i_err;
}
int HJFaceuTextureInfo::priDraw(float* i_mvp)
{
    int i_err = HJ_OK;
    do
    {
        HJFaceuStrategy strategy = getStrategy();
        if (strategy == HJFaceuStrategy_HarmonyBridge)
        {
#if defined(HarmonyOS)
            if (!m_draw)
            {
                m_draw = HJOGCopyShaderStrip::Create();
                i_err = m_draw->init(OGCopyShaderStripFlag_OES);
                if (i_err < 0)
                {
                    break;
                }
            }
            i_err = m_draw->draw(m_bridge->getTextureId(), i_mvp, m_bridge->getTexMatrix());
            if (i_err < 0)
            {
                break;
            }
#endif
        }
        else if ((strategy == HJFaceuStrategy_SharedCtxTexture) || (strategy == HJFaceuStrategy_ExclusiveTexture))
        { 
            if (m_bCreateTexture)
            {
                if (!m_draw)
                {
                    bool bPreMultiple = true;
                    m_draw = HJOGCopyShaderStrip::Create();
                    i_err = m_draw->init(OGCopyShaderStripFlag_2D, bPreMultiple);
                    if (i_err < 0)
                    {
                        break;
                    }

                }
                i_err = m_draw->draw(m_textureId, i_mvp, HJOGCommon::IdentifyMatrix);
                if (i_err < 0)
                {
                    break;
                }

                if (strategy == HJFaceuStrategy_SharedCtxTexture)
                {
                    //fixme lfs
                    glFlush();
                }
                
            }
        }      
    } while (false);
    return i_err;
}
int HJFaceuTextureInfo::onDataReady(unsigned char* data, int width, int height, int nrComponents)
{
    int i_err = HJ_OK;
    do
    {
        HJFaceuStrategy strategy = getStrategy();
        if (strategy == HJFaceuStrategy_HarmonyBridge)
        {
#if defined(HarmonyOS)
            if (m_bridge)
            {
                int r = m_bridge->procPixel([data, width, height, nrComponents](unsigned char* dstData, int dstStride)
                    {
                        int ret = HJ_OK;

                        if (nrComponents == 3)
                        {
                            ret = libyuv::RGB24ToARGB(data, nrComponents * width, dstData, dstStride, width, height);
                        }
                        else if (nrComponents == 4)
                        {
                            ret = libyuv::ARGBCopy(data, nrComponents * width, dstData, dstStride, width, height);
                        }
                        if (ret < 0)
                        {
                            ret = -1;
                            HJFLoge("libyuv error componets:{} width:{} height:{}", nrComponents, width, height);
                        }
                        return ret;
                    }, width, height);
                if (r < 0)
                {
                    HJFLoge("procPixel error componets:{} width:{} height:{}", nrComponents, width, height);
                    stbi_image_free(data);
                }
            }
#endif
        }
        else if (strategy == HJFaceuStrategy_SharedCtxTexture)
        {
			if (!m_bCreateTexture)
			{
				m_textureId = HJOGCommon::textureCreate();
				m_bCreateTexture = true;
				m_rbgabuffer = HJSPBuffer::create(width * height * 4);
			}
			if (nrComponents == 3)
			{
				libyuv::RGB24ToARGB(data, nrComponents * width, m_rbgabuffer->getBuf(), width * 4, width, height);
			}
			else if (nrComponents == 4)
			{
				libyuv::ARGBCopy(data, nrComponents * width, m_rbgabuffer->getBuf(), width * 4, width, height);
			}
			HJOGCommon::textureUploadRGBA(m_textureId, width, height, m_rbgabuffer->getBuf());
			//fixme lfs
			glFlush();          
        }
        else if (strategy == HJFaceuStrategy_ExclusiveTexture)
        {
            HJRawImageDataInfo::Ptr rawInfo = HJRawImageDataInfo::Create();
            rawInfo->m_width = width;
            rawInfo->m_height = height;
            rawInfo->m_components = nrComponents;
            rawInfo->m_buffer = HJSPBuffer::create(width * height * nrComponents, data);
            m_cache.enqueue(rawInfo);
        }
    } while (false);
    return i_err;
    
}
///////////////////////////////////////////////

HJFaceuInfo::~HJFaceuInfo()
{
    priDone();
}
int HJFaceuInfo::update()
{
    int i_err = HJ_OK;
    do
	{
		//BEGIN_TIME;
		for (auto& item : texture)
		{
			i_err = item->update();
			if ((i_err < 0) || (i_err == HJ_WOULD_BLOCK))
			{
				break;
			}
		}
		//END_TIME("update");
    } while (false);
    return i_err;
}
int HJFaceuInfo::draw(const std::vector<HJPointf>& i_points, int width, int height)
{
    int i_err = HJ_OK;
    //BEGIN_TIME;
    for (auto& item : texture)
    {
        i_err = item->draw(i_points, width, height);
        if (i_err < 0)
        {
            break;
        }
    }
    //END_TIME("draw");
    return i_err;
}
void HJFaceuInfo::priOnParseEnd(const HJBaseParam::Ptr& i_param)
{
    if (getStrategy() == HJFaceuStrategy_SharedCtxTexture)
    {
        HJ_CatchMapPlainGetVal(i_param, bool, "bUseSharedCtxTexture", m_bUseSharedCtxTexture);
        if (m_bUseSharedCtxTexture)
        {
            HJThreadTaskFunc funcInit = nullptr;
            HJThreadTaskFunc funcEnd = nullptr;
            HJ_CatchMapPlainGetVal(i_param, HJThreadTaskFunc, "faceuInitFunc", funcInit);
            HJ_CatchMapPlainGetVal(i_param, HJThreadTaskFunc, "faceuEndFunc", funcEnd);
            m_threadTimer->setBeginFunc(funcInit);
            m_threadTimer->setEndFunc(funcEnd);
        }
    }
}
void HJFaceuInfo::priStat(int64_t t0, int i_fps)
{
	int64_t t1 = HJCurrentSteadyMS();
	int64_t diff = t1 - t0;
	m_statIdx++;
	m_statElapseSum += diff;
    int64_t fromToCurTime = t1 - m_statFirstTime;
	if (fromToCurTime > 0)
	{
		if ((m_statIdx % 100) == 0)
		{
			HJFLogi("setfps:{} realfps:{} decode curdiff:{} avg diff:{}", i_fps, m_statIdx * 1000 / fromToCurTime, diff, m_statElapseSum / m_statIdx);
		}
	}

}
int HJFaceuInfo::parse(HJBaseParam::Ptr i_param)
{
	int i_err = HJ_OK;
    do
    {
        HJFLogi("parse enter");
        bool bEveryCompleteNotify = false;
        HJ_CatchMapPlainGetVal(i_param, bool, "bEveryCompleteNotify", bEveryCompleteNotify);

        HJ_CatchMapPlainGetVal(i_param, std::string, HJRteUtils::ParamUrlFaceu, m_rootPath);
        if (m_rootPath.empty())
        {
            i_err = HJErrInvalidPath;
            HJFLoge("parse error m_rootPath empty");
            break;
        }

        std::string configUrl = HJBaseUtils::combineUrl(m_rootPath, "config");
        if (configUrl.empty())
        {
            i_err = HJErrInvalidPath;
            HJFLoge("parse error configUrl empty");
            break;
        }
        HJFLogi("parse configUrl:{}", configUrl);
        std::string config = HJBaseUtils::readFileToString(configUrl);
        if (config.empty())
        {
            i_err = HJErrInvalidPath;
            HJFLoge("parse error config empty");
            break;
        }
        i_err = HJJsonBase::init(config);
        if (i_err < 0)
        {
            i_err = HJErrInvalidResult;
            HJFLoge("json parse err:{}", i_err);
            break;
        }
        i_err = priCreateImgPaths();
        if (i_err < 0)
        {
            i_err = HJErrInvalidResult;
            HJFLoge("priCreateImgPaths err:{}", i_err);
            break;
        }
        HJFLogi("parse faceu:{} success", configUrl);

        HJ_CatchMapGetVal(i_param, HJListener, m_renderListener);

        std::vector<HJFaceuTextureInfo::Ptr>::iterator maxIt = std::max_element(texture.begin(), texture.end(), [](const HJFaceuTextureInfo::Ptr& a, const HJFaceuTextureInfo::Ptr& b)
            {
                return a->mframeCount < b->mframeCount;
            }
        );
        if (maxIt != texture.end())
        {
            (*maxIt)->setMaxCount();
        }

        m_threadTimer = HJTimerThreadPool::Create();

        priOnParseEnd(i_param);


        int curFrameRate = framerate;
        if (curFrameRate == std::numeric_limits<int>::min())
        {
            curFrameRate = 15;
        }
        m_threadTimer->startTimer(1000 / curFrameRate, [this, curFrameRate, bEveryCompleteNotify]
            {
                int64_t t0 = HJCurrentSteadyMS();
                int64_t preDiff = -1;
                if (m_statFirstTime == -1)
                {
                    m_statFirstTime = t0;
                }
                for (auto& item : texture)
                {
                    const std::string& url = item->m_imagePaths[item->m_procIdx];
                    int width = 0, height = 0, nrComponents = 0;
                    unsigned char* data = stbi_load(url.c_str(), &width, &height, &nrComponents, 0);
                    if (data)
                    {
                        item->onDataReady(data, width, height, nrComponents);
                        stbi_image_free(data);
                    }
                    item->m_procIdx++;
                    if (item->m_procIdx >= item->m_imagePaths.size())
                    {
                        item->m_procIdx = 0;
                        if (item->isMaxCount())
                        {
                            item->m_curLoopIdx++;
                            if (((loop > 0) && (item->m_curLoopIdx >= loop)) || bEveryCompleteNotify)
                            {
                                //notify;
                                HJFLogi("loop end {} {} {}", item->imageName, item->m_curLoopIdx, (int)item->m_imagePaths.size());
                                if (m_renderListener)
                                {
                                    m_renderListener(std::move(HJMakeNotification(HJVIDEORENDERGRAPH_EVENT_FACEU_COMPLETE, 0, getInsName())));
                                }
                            }
                        }
                    }
                }
                priStat(t0, curFrameRate);
                return HJ_OK;
            });
    } while (false);
    return i_err;
}

int HJFaceuInfo::priCreateImgPaths()
{
    int i_err = HJ_OK;
    do
    {
        for (auto& item : texture)
        {
            int nCount = item->mframeCount;
            if (nCount <= 0)
            {
                i_err = HJErrInvalidResult;
                HJFLoge("invalid frameCount:{} imageName:{}", nCount, item->imageName);
                break;
            }
            for (int i = 0; i < nCount; i++)
            {
                std::string path = HJBaseUtils::combineUrl(m_rootPath, HJFMT("{}/{}{}.png", item->imageName, item->imageName, i));
                if (path.empty())
                {
                    i_err = HJErrInvalidPath;
                    break;
                }
                item->m_imagePaths.push_back(std::move(path));
            }
            item->setStrategy(getStrategy());
            i_err = item->onParseReady();
            if (i_err < 0)
            {
                break;
            }
        }
    } while (false);    
    return i_err;
}

void HJFaceuInfo::priDone()
{
    if (!m_bDone)
    {
        m_bDone = true;
        

        int64_t t0 = HJCurrentSteadyMS();
        HJFLogi("pridone timer stop enter");
        //must before texture.clear(),  thread use bridge, so must first stop thread;
        if (m_threadTimer)
        {
            m_threadTimer->stopTimer();
            m_threadTimer = nullptr;
        }
        HJFLogi("pridone timer end enter: time:{}", (HJCurrentSteadyMS() - t0));
        texture.clear();
        HJFLogi("pridone texture end");           
    }
}
NS_HJ_END