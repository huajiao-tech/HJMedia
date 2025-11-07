#include "HJPrioComFaceu.h"
#include "HJFLog.h"
#include "HJBaseUtils.h"
#include "HJComEvent.h"

#include "HJFaceuInfo.h"
#include "HJFacePointMgr.h"
#if defined(HarmonyOS)
#include "HJOGFBOCtrl.h"
#include "HJOGEGLSurface.h"
#include "HJOGBaseShader.h"
#include "HJOGPointShader.h"
#include "HJOGRenderWindowBridge.h"
#endif

NS_HJ_BEGIN

HJPrioComFaceu::HJPrioComFaceu()
{
    HJ_SetInsName(HJPrioComFaceu);
    setPriority(HJPrioComType_FaceU2D);
}
HJPrioComFaceu::~HJPrioComFaceu()
{
}

int HJPrioComFaceu::init(HJBaseParam::Ptr i_param)
{
    int i_err = HJ_OK;
    do
    {
        i_err = HJPrioCom::init(i_param);
        if (i_err < 0)
        {
            break;
        }
#if defined(HarmonyOS)
        m_draw = HJOGCopyShaderStrip::Create();
        i_err = m_draw->init(OGCopyShaderStripFlag_2D, false);
        if (i_err < 0)
        {
            break;
        }
#endif
        m_faceuInfo = HJFaceuInfo::Create();
        i_err = m_faceuInfo->parse(i_param);
        if (i_err < 0)
        {
            break;
        }
    } while (false);
    return i_err;
}
void HJPrioComFaceu::done()
{
    m_faceuInfo = nullptr;
    HJPrioCom::done();
}
int HJPrioComFaceu::update(HJBaseParam::Ptr i_param)
{
    int i_err = HJ_OK;
    do
    {
        if (m_faceuInfo)
        {
            for (auto &item : m_faceuInfo->texture)
            {
                item->update();
            }
        }

        if (!m_point)
        {
            i_err = HJ_WOULD_BLOCK;
            break;
        }
#if defined(HarmonyOS)
        m_bDraw = false;
        int sourceWidth = 0;
        int sourceHeight = 0;
        HJ_CatchMapPlainGetVal(i_param, int, "SourceWidth", sourceWidth);
        HJ_CatchMapPlainGetVal(i_param, int, "SourceHeight", sourceHeight);

        if ((sourceWidth > 0) && (sourceHeight > 0))
        {
            if ((sourceWidth != m_sourceWidth) || (sourceHeight != m_sourceHeight))
            {
                m_fbo = HJOGFBOCtrl::Create();
                HJFLogi("{} fbo change width:{} height:{}", getInsName(), sourceWidth, sourceHeight);
                m_fbo->init(sourceWidth, sourceHeight);
                m_sourceWidth = sourceWidth;
                m_sourceHeight = sourceHeight;
            }
        }
        if (m_fbo)
        {
            m_fbo->attach();

            if ((m_point->width() == m_sourceWidth) && (m_point->height() == m_sourceHeight))
            {
                // HJFLogi("Faceu face points wh<{} {}> p0:<{} {}> p3:<{} {}>", point->m_width, point->m_height, point->m_p0.x, point->m_p0.y, point->m_p3.x, point->m_p3.y);
                if (!m_point->isContainFace())
                {
                    m_bDraw = false;
                    m_fbo->detach();
                    break;
                }

                for (auto &item : m_faceuInfo->texture)
                {
                    item->draw(m_point->getFilterPt(), m_fbo->getWidth(), m_fbo->getHeight());
                }

                if (m_bUsePointDebug)
                {
                    if (!m_pointShader)
                    {
                        m_pointShader = HJOGPointShader::Create();
                        i_err = m_pointShader->init(9);
                        if (i_err < 0)
                        {
                            m_fbo->detach();
                            break;
                        }
                    }

                    std::vector<HJPointf> tmpPoints;
                    for (auto &pt : m_point->getFilterPt())
                    {
                        tmpPoints.push_back(HJOGPointShader::convert(pt, m_point->width(), m_point->height()));
                    }
                    i_err = m_pointShader->update(tmpPoints);
                    if (i_err < 0)
                    {
                        m_fbo->detach();
                        break;
                    }
                    i_err = m_pointShader->draw();
                    if (i_err < 0)
                    {
                        m_fbo->detach();
                        break;
                    }
                }

                m_bDraw = true;
            }
            else
            {
                HJFLogi("pricom face points {} {} not match {} {}", m_point->width(), m_point->height(), m_sourceWidth, m_sourceHeight);
            }

            m_fbo->detach();
        }
#endif
    } while (false);
    return i_err;
}
//int HJPrioComFaceu::sendMessage(HJBaseMessage::Ptr i_msg)
//{
//    int i_err = HJ_OK;
//    do
//    {
//        switch (i_msg->getMessageType())
//        {
//        case HJCOM_MESSAGE_FACEPOINT:
//            HJ_CatchMapGetVal(i_msg, HJFacePointsReal::Ptr, m_point);
//            break;
//        }
//    } while (false);
//    return i_err;
//}
int HJPrioComFaceu::render(HJBaseParam::Ptr i_param)
{
    int i_err = HJ_OK;
    do
    {
        if (!m_bDraw)
        {
            break;
        }
        if (!m_fbo)
        {
            break;
        }
#if defined(HarmonyOS)
        HJOGEGLSurface::Ptr surface = nullptr;
        HJ_CatchMapGetVal(i_param, HJOGEGLSurface::Ptr, surface);
        if (!surface)
        {
            i_err = -1;
            break;
        }

        if (renderModeIsContain(surface->getSurfaceType()))
        {
            std::vector<HJTransferRenderModeInfo::Ptr> &renderModes = renderModeGet(surface->getSurfaceType());
            for (auto it = renderModes.begin(); it != renderModes.end(); it++)
            {
                HJTransferRenderViewPortInfo::Ptr viewpotInfo = HJTransferRenderModeInfo::compute((*it), surface->getTargetWidth(), surface->getTargetHeight());

                glViewport(viewpotInfo->x, viewpotInfo->y, viewpotInfo->width, viewpotInfo->height);
                i_err = m_draw->draw(m_fbo->getTextureId(), (*it)->cropMode, m_fbo->getWidth(), m_fbo->getHeight(), viewpotInfo->width, viewpotInfo->height);
                if (i_err < 0)
                {
                    break;
                }
            }
            if (i_err < 0)
            {
                break;
            }
        }
#endif
    } while (false);
    return i_err;
}

HJFaceSubscribeFuncPtr HJPrioComFaceu::getFaceSubscribePtr()
{
    if (!m_faceSubscribePtr)
    {
        HJPrioComFaceu::Wtr wtr = HJBaseSharedObject::getSharedFrom(this);
        m_faceSubscribePtr = std::make_shared<HJFaceSubscribeFunc>([wtr](HJFacePointsReal::Ptr i_point)
        {
                HJPrioComFaceu::Ptr ptr = wtr.lock();
                if (ptr)
                {
                    ptr->m_point = i_point;
                    //HJFLogi("{} face get publish result", ptr->getInsName());
                }
        });
    }
    return m_faceSubscribePtr;
}

NS_HJ_END