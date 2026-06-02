#pragma once

#include <vector>
#include "HJMediaUtils.h"

NS_HJ_BEGIN

typedef struct HJFaceDetectPoint
{
    HJFaceDetectPoint(float i_x, float i_y)
    {
        set(i_x, i_y);
    }
	void set(float i_x, float i_y)
	{
        x = i_x;
        y = i_y;
	}
	float x = 0.f;
	float y = 0.f;
} HJFaceDetectPoint;

typedef struct HJFaceDetectRect
{
	void set(float i_x, float i_y, float i_w, float i_h)
	{
        x = i_x;
        y = i_y;
        width = i_w;
        height = i_h;
	}
	float x = 0.f;
    float y = 0.f;
    float width = 0.f;
    float height = 0.f;
} HJFaceDetectRect;

typedef struct HJSingleFaceDetectRet
{
	HJFaceDetectRect m_rect;
	std::vector<HJFaceDetectPoint> m_keyPoints;
	void addKeyPoint(float i_x, float i_y)
	{
		m_keyPoints.push_back(std::move(HJFaceDetectPoint(i_x, i_y)));
	}
	void setRect(float i_x, float i_y, float i_w, float i_h)
	{
		m_rect.set(i_x, i_y, i_w, i_h);
	}
	HJ::HJPointf getPoint(int i_idx)
	{
		if (i_idx < m_keyPoints.size())
		{
			return HJ::HJPointf{m_keyPoints[i_idx].x, m_keyPoints[i_idx].y};
		}
		return HJ::HJPointf{ 0.0f, 0.0f };
	}
	HJ::HJPointf getAvgPoint(std::vector<int>& i_idxs)
	{
		HJ::HJPointf avgPoint{ 0.0f, 0.0f };
		int cnt = 0;
		int size = (int)i_idxs.size();
		for (int i = 0; i < size; i++)
		{
			int idx = i_idxs[i];
			if (idx < (int)m_keyPoints.size())
			{
				avgPoint.x += m_keyPoints[idx].x;
				avgPoint.y += m_keyPoints[idx].y;
				cnt++;
			}
		}
		if (cnt > 0)
		{
			avgPoint.x /= cnt;
			avgPoint.y /= cnt;
		}
		return avgPoint;
	}
} HJSingleFaceDetectRet;

typedef struct HJFaceDetectRet
{
	
	void reset()
	{
        m_bContainFace = false;
		m_faces.clear();
	}
	void setSize(int i_width, int i_height)
	{
        m_width = i_width;
        m_height = i_height;
	}
	void add(std::shared_ptr<HJSingleFaceDetectRet> i_face)
	{
        m_faces.push_back(std::move(i_face));
	}
	const std::vector<std::shared_ptr<HJSingleFaceDetectRet>> & getFaces() const
	{
		return m_faces;
	}
	int m_width = 0;
    int m_height = 0;
	bool m_bContainFace = false;
	int64_t m_systemTime = 0;
	int64_t m_elapseMs = 0;
	int64_t m_timeStamp = 0;
	std::vector<std::shared_ptr<HJSingleFaceDetectRet>> m_faces;
	
} HJFaceDetectRet;

struct HJRGBAStruct {
	HJRGBAStruct(int r = 0, int g = 0, int b = 0, int a = 0) : r(r), g(g), b(b), a(a) {}
	unsigned char r, g, b, a;
};

class HJFaceDetectUtils
{ 
public:
	static void rectangle(void* data_rgba, int image_height, int image_width, int x0, int y0, int x1, int y1, float scale_x, float scale_y);
};

NS_HJ_END