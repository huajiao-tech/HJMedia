#include "HJFaceuInfo.h"
#include "HJFLog.h"
#include "HJBaseUtils.h"
#include "HJThreadPool.h"
#include "stb_image.h"
#include "libyuv.h"
#include "../HJComEvent.h"

#if defined(HarmonyOS)
#include "HJOGRenderWindowBridge.h"
#include "HJOGBaseShader.h"
#endif

#define PI 3.1415926
#define IRadianRatio PI / 180.f

NS_HJ_BEGIN


static double getFaceAngle(HJPointf leftEye, HJPointf rightEye, int faceDetWidth) {
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
static HJPointf getRotatePoint(HJPointf o, HJPointf a, float angle) {
	HJPointf b{ 0.f, 0.f };
	double angleRatio = angle * IRadianRatio;

	b.x = (a.x - o.x) * (float)std::cos(angleRatio) - (a.y - o.y) * (float)std::sin(angleRatio) + o.x;
	b.y = (a.x - o.x) * (float)std::sin(angleRatio) + (a.y - o.y) * (float)std::cos(angleRatio) + o.y;
	return b;
}
static HJPointf onTransformPonits(float mid_x, float mid_y, float w, float h, float anchorOffsetX, float anchorOffsetY, float angle, float scaleRatio, float ratio_w, bool isHJCamera) {
	HJPointf o{ mid_x, mid_y };
	float scale = scaleRatio * ratio_w;

	float mx = isHJCamera ? mid_x + (w / 2 + anchorOffsetX) * scale : mid_x - (w / 2 + anchorOffsetX) * scale;
	float my = mid_y - (h / 2 + anchorOffsetY) * scale;
	HJPointf a{ mx, my };

	HJPointf b = getRotatePoint(o, a, angle);

	return b;
}

//    static HJPointi filter(HJPointi src, int width, int height)
//    {
//        HJPointi target{180, 320};
//        
//        return HJPointi{(int)((float)src.x * target.x / width), (int)((float)src.y * target.y / height)};
//    }
//
//    static HJPointi testFixPoint(int x, int y)
//    {
//        return HJPointi{x, y};
//    }

//////////////////////////////////
HJFaceuTextureInfo::~HJFaceuTextureInfo()
{
    if (m_bridge)
    {
#if defined(HarmonyOS)
        m_bridge->done();
        m_bridge = nullptr;
#endif
    }
}
void HJFaceuTextureInfo::update()
{
#if defined(HarmonyOS)
    if (m_bridge->IsStateAvaiable())
    {
        m_bridge->update();
    }
#endif
}
int HJFaceuTextureInfo::draw(std::vector<HJPointf> i_points, int width, int height)
{
    int i_err = HJ_OK;
    do 
    {
#if defined(HarmonyOS)
        if (m_bridge->IsStateAvaiable())
        {
            int mFaceDetWidth = width;
            int mFaceDetHeight = height;
            HJPointf leftEye = i_points[0];
            HJPointf rightEye = i_points[1];
            HJPointf nose = i_points[2];
            //fixme shang zui chun, but not have this point, so use left and right mouse average; is not precise
            HJPointf top_lip = HJPointf{(i_points[3].x + i_points[4].x) / 2, (i_points[3].y + i_points[4].y) / 2};
            
//            int mFaceDetWidth = 180;
//            int mFaceDetHeight = 320;
//            HJPointi leftEye = filter(point->m_p0, width, height);
//            HJPointi rightEye = filter(point->m_p1, width, height);
//            HJPointi nose = filter(point->m_p2, width, height);
//            HJPointi P3 = filter(point->m_p3, width, height);
//            HJPointi P4 = filter(point->m_p4, width, height);
            
            //fixme lfs test
//            leftEye = testFixPoint(77,206);
//            rightEye = testFixPoint(109,207);
//            nose = testFixPoint(96,221);
//            P3 = testFixPoint(84,242);
//            P4 = testFixPoint(106,242);
            //fixme shang zui chun, but not have this point, so use left and right mouse average; is not precise
            //HJPointi top_lip = HJPointi{(P3.x + P4.x) / 2, (P3.y + P4.y) / 2};
            
            float FloatMin = std::numeric_limits<float>::min();
            int IntMin = std::numeric_limits<int>::min();
            bool mIsAsGift = false;
            
            bool mIsHuajiaoCamera = false;
            int faceUWidth = width;
            int faceUHeight = height;

            float   mid_x = 0.5f;
            float   mid_y = 0.5f;
            float   ratio_w = faceUWidth / (float)mFaceDetWidth;
            float   ratio_h = faceUHeight / (float)mFaceDetHeight;
            bool isLandscape = faceUWidth > faceUHeight;
            
            float angle = (float)getFaceAngle(leftEye, rightEye, mFaceDetWidth);
            float eyeDistance = (float) sqrt((float) ((rightEye.x - leftEye.x) * (rightEye.x - leftEye.x) + (rightEye.y - leftEye.y) * (rightEye.y - leftEye.y)));
            int radiusType      = radius_Type;
            float radius        = (0 == radiusType) ? angle : (float)mradius;
            
            int midType         = mid_Type;
            int scaleType       = scale_Type;
            float scaleRatio    = (0 == scaleType) ? eyeDistance / scale_ratio : scale_ratio;
            if(mIsHuajiaoCamera) {
                scaleRatio = (1== scaleType) ? scale_ratio : eyeDistance / scale_ratio;
            }
            float anchorOffsetX = (float)anchor_offset_x;
            float anchorOffsetY = (float)anchor_offset_y;
            float w             = (float)asize_offset_x;
            float h             = (float)asize_offset_y;
            float fixed_ratio   = isLandscape ? faceUHeight / w : faceUWidth / w;                             //portrait -> landscape, because faceU gift design by portrait
            float base_ratio_x  = ratio_w;
            float base_ratio_y  = ratio_h;
            bool isTransformPonit = true;
            bool isOrthProjection = mIsHuajiaoCamera ? false : true;// mIsOrthProjection;
            
            switch (midType)
            {
                case 0:
                    // 2.1 head type
                    mid_x = ((rightEye.x + leftEye.x) / 2.0f) * ratio_w;
                    mid_y = faceUHeight - ((rightEye.y + leftEye.y) / 2.0f) * ratio_h;
                    break;
                case 1:
                    // 2.2 nose
                    mid_x = nose.x * ratio_w;
                    mid_y = faceUHeight - nose.y * ratio_h;
                    //HJFLogi("radius:{} radiusType:{} angle:{} x1 y1:{} {} x2 y2:{} {}", radius, radiusType, angle, leftEye.x, leftEye.y, rightEye.x, rightEye.y);
                    break;
                case 2: // 2.3 fixed type
                    if(isLandscape)
                    {
                        mid_x = FloatMin;
                        if (mid_x <= FloatMin){ //old
                            mid_x = (w - mid_x) * scaleRatio * fixed_ratio / 2.0f;
                            mid_y = (h - mid_y) * scaleRatio * fixed_ratio / 2.0f;
                            //portrait -> landscape, map middle point to landscape
                            mid_x = faceUWidth * mid_x / faceUHeight;
                            mid_y = faceUHeight * mid_y / faceUWidth;

                            isTransformPonit = false;
                        }else{
                            mid_x *= faceUWidth;
                            mid_y = FloatMin * faceUHeight;

                            scaleRatio      = FloatMin;
                            anchorOffsetX   = IntMin;
                            anchorOffsetY   = IntMin;
                        }
                    }else {
                        mid_x = FloatMin;
                        if (mid_x <= FloatMin) { //old
                            if (mIsHuajiaoCamera)
                            {
                                if(mIsAsGift){
                                    mid_x = w * scaleRatio * fixed_ratio / 2.0f;
                                    mid_y = h * scaleRatio * fixed_ratio / 2.0f;
                                }else{
                                    float xn = mid_x;
                                    float yn = mid_y;
                                    fixed_ratio = faceUWidth / 720.0f;
                                    mid_x = (w * xn) * scaleRatio * fixed_ratio;
                                    mid_y = (h * yn) * scaleRatio * fixed_ratio;
                                }
                            } else {
                                mid_x = (w - mid_x) * scaleRatio * fixed_ratio / 2.0f;
                                mid_y = (h - mid_y) * scaleRatio * fixed_ratio / 2.0f;
                            }

                            isTransformPonit = false;
                        }else{
                            mid_x *= faceUWidth;
                            mid_y = FloatMin * faceUHeight;
                        }
                    }
//                    if (mirrored) {
//                        mid_x = faceUWidth - mid_x;
//                    }

                    base_ratio_x = fixed_ratio;
                    base_ratio_y = fixed_ratio;

                    isOrthProjection = true;
                    break;
                case 3:
                    // 2.4 mouse type
                    mid_x = top_lip.x * ratio_w;
                    mid_y = faceUHeight - top_lip.y * ratio_h;
                    break;
                default:
                    break;
            } 
            
            if (isTransformPonit) 
            {
                HJPointf b = onTransformPonits(mid_x, mid_y, w, h, anchorOffsetX, anchorOffsetY, radius, scaleRatio, base_ratio_x, mIsHuajiaoCamera);
                mid_x = b.x;
                mid_y = b.y;
            }
            mat4x4 mvp;
            if (isOrthProjection)
            {
                
			    mat4x4_identity(mvp);
                
                mat4x4 scaleMat;
                mat4x4_identity(scaleMat);
                mat4x4_scale_aniso(mvp, scaleMat, w * scaleRatio * base_ratio_x * 0.5, h * scaleRatio * base_ratio_y * 0.5, 1);  //*0.5 because, the vertex point is -0.5 -0.5 to 0.5 0.5, if use -1 -1 to 1 1  so  multiple 0.5
                
                mat4x4 rotateMat;
                mat4x4_identity(rotateMat);
                mat4x4_rotate(rotateMat, rotateMat, 0, 0, 1, radius * PI / 180.f); //radius and rotate is different, sin(radian) cos(radian)
                
                mat4x4_mul(mvp, rotateMat, mvp);
                
                mat4x4 translateMat;
                mat4x4_identity(translateMat);
                mat4x4_translate(translateMat, mid_x - (faceUWidth - width) / 2, mid_y - (faceUHeight - height) / 2, 0.f);
                mat4x4_mul(mvp, translateMat, mvp);
                
                mat4x4 orth;
			    mat4x4_ortho(orth, 0, width, 0, height, -1, 1);
                mat4x4_mul(mvp, orth, mvp);
                
                //m_bridge->
            }   
//            else 
//            {
//                
//            }    
            
            
            HJTransferRenderModeInfo::Ptr p = HJTransferRenderModeInfo::Create();
            //p->cropMode = "fit";
            //m_bridge->draw(p, width, height);
            if (!m_draw)
            {
                m_draw = HJOGCopyShaderStrip::Create();
                m_draw->init(OGCopyShaderStripFlag_OES);
            }    
            m_draw->draw(m_bridge->getTextureId(), (float *)&mvp, m_bridge->getTexMatrix());
        }   
#endif
    } while (false);
    return i_err;
}
///////////////////////////////////////////////

HJFaceuInfo::~HJFaceuInfo()
{
    priDone();
}

int HJFaceuInfo::parse(HJBaseParam::Ptr i_param)
{
	int i_err = HJ_OK;
	do
	{
        HJFLogi("parse enter");
        HJ_CatchMapPlainGetVal(i_param, std::string, "faceuUrl", m_rootPath);
        if (m_rootPath.empty())
        {
            i_err = HJErrInvalidPath;
            break;
        }
         
        std::string configUrl = HJBaseUtils::combineUrl(m_rootPath, "config");
        if (configUrl.empty())
        {
            i_err = HJErrInvalidPath;
            break;
        }
        HJFLogi("parse configUrl:{}", configUrl);
        std::string config = HJBaseUtils::readFileToString(configUrl);
        if (config.empty())
        {
            i_err = HJErrInvalidPath;
            break;
        }
        i_err = HJJsonBase::init(config);
        if (i_err < 0)
        {
            i_err = HJErrInvalidResult;
            break;
        }
        i_err = priCreateImgPaths();
        if (i_err < 0)
        {
            i_err = HJErrInvalidResult;
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
        m_threadTimer->startTimer(1000 / framerate, [this] 
        {
            for (auto& item : texture)
            {
                const std::string &url = item->m_imagePaths[item->m_procIdx];
                int width = 0, height = 0, nrComponents = 0;
                unsigned char* data = stbi_load(url.c_str(), &width, &height, &nrComponents, 0);
                if (data)
                {
#if defined(HarmonyOS)
                    if (item->m_bridge)
                    {
                        int r = item->m_bridge->procPixel([data, width, height, nrComponents](unsigned char* dstData, int dstStride)
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
                    stbi_image_free(data);
                }
                item->m_procIdx++;
                if (item->m_procIdx >= item->m_imagePaths.size())
                {
                    item->m_procIdx = 0;
                    if (item->isMaxCount())
                    {
                        item->m_curLoopIdx++;
                        if ((loop > 0) && item->m_curLoopIdx >= loop)
                        {
                            //notify;
                            HJFLogi("loop end {} {} {}", item->imageName, item->m_curLoopIdx, (int)item->m_imagePaths.size());
                            if (m_renderListener)
                            {
                                m_renderListener(std::move(HJMakeNotification(HJVIDEORENDERGRAPH_EVENT_FACEU_COMPLETE)));
                            } 
                        }
                    }
                }
            }

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
#if defined(HarmonyOS)
            item->m_bridge = HJOGRenderWindowBridge::Create();
            item->m_bridge->setInsName(HJFMT("{}_bridge", item->imageName));
            i_err = item->m_bridge->init();
            if (i_err < 0)
            {
                i_err = HJErrInvalidResult;
                break;
            }
#endif
        }
    } while (false);    
    return i_err;
}

void HJFaceuInfo::priDone()
{
    if (!m_bDone)
    {
        m_bDone = true;
        
        HJFLogi("pridone timer stop enter");
        //must before texture.clear(),  thread use bridge, so must first stop thread;
        if (m_threadTimer)
        {
            m_threadTimer->done();
            m_threadTimer = nullptr;
        }
        HJFLogi("pridone timer end enter");
        texture.clear();
        HJFLogi("pridone texture end");           
    }
}
NS_HJ_END