#include "HJPointSmooth.h"
#include "HJFacePointsReal.h"

#include <algorithm>
#include <cmath>

NS_HJ_BEGIN

HJSinglePointSmooth::Ptr HJSinglePointSmooth::createSmooth(HJPointSmoothPolicy i_policy)
{
	HJSinglePointSmooth::Ptr smooth = nullptr;
	switch (i_policy)
	{
	case HJPointSmoothPolicy::None:
		smooth = HJPointSmoothNone::Create();
		break;
	case HJPointSmoothPolicy::OneEuro:
		smooth = HJPointSmoothOneEuro::Create();
		break;
	case HJPointSmoothPolicy::Velocity:
		smooth = HJPointSmoothVelocity::Create();
		break;
	default:
		break;
	}
	return smooth;
}

HJPointf HJPointSmoothNone::smooth(HJPointf i_point)
{
	return i_point;
}

HJPointf HJPointSmoothNone::smooth(HJPointf i_point, float i_dt)
{
	(void)i_dt;
	return i_point;
}

void HJPointSmoothNone::reset()
{
}

/// //////////////////////////////////////////////////////////////
namespace {
constexpr float kPi = 3.14159265358979323846f;

float calcAlpha(float i_cutoff, float i_dt)
{
	if (i_cutoff <= 0.0f || i_dt <= 0.0f)
	{
		return 1.0f;
	}
	const float tau = 1.0f / (2.0f * kPi * i_cutoff);
	return i_dt / (i_dt + tau);
}

HJPointf lowPass(const HJPointf& i_prev, const HJPointf& i_cur, float i_alpha)
{
	return HJPointf{
		i_alpha * i_cur.x + (1.0f - i_alpha) * i_prev.x,
		i_alpha * i_cur.y + (1.0f - i_alpha) * i_prev.y};
}
} // namespace

HJPointf HJPointSmoothOneEuro::smooth(HJPointf i_point)
{
	return smooth(i_point, m_defaultDt);
}

HJPointf HJPointSmoothOneEuro::smooth(HJPointf i_point, float i_dt)
{
	const float dt = (i_dt > 0.000001f) ? i_dt : m_defaultDt;
	if (!m_inited)
	{
		m_inited = true;
		m_prevFiltered = i_point;
		m_prevDeriv = HJ_POINT_ZERO;
		return i_point;
	}

	const HJPointf delta{
		(i_point.x - m_prevFiltered.x) / dt,
		(i_point.y - m_prevFiltered.y) / dt};

	const float alphaD = calcAlpha(m_dCutoff, dt);
	const HJPointf deriv = lowPass(m_prevDeriv, delta, alphaD);

	const float speed = std::sqrt(deriv.x * deriv.x + deriv.y * deriv.y);
	const float cutoff = m_minCutoff + m_beta * speed;
	const float alpha = calcAlpha(cutoff, dt);

	const HJPointf filtered = lowPass(m_prevFiltered, i_point, alpha);

	m_prevDeriv = deriv;
	m_prevFiltered = filtered;
	return filtered;
}

void HJPointSmoothOneEuro::reset()
{
	m_inited = false;
	m_prevFiltered = HJ_POINT_ZERO;
	m_prevDeriv = HJ_POINT_ZERO;
}

void HJPointSmoothOneEuro::setParams(float i_minCutoff, float i_beta, float i_dCutoff)
{
	m_minCutoff = (i_minCutoff > 0.0f) ? i_minCutoff : m_minCutoff;
	m_beta = (i_beta >= 0.0f) ? i_beta : m_beta;
	m_dCutoff = (i_dCutoff > 0.0f) ? i_dCutoff : m_dCutoff;
}

HJPointSmoothVelocity::~HJPointSmoothVelocity() = default;

HJPointSmoothVelocity::RelativeVelocityFilter1D::RelativeVelocityFilter1D(size_t i_windowSize, float i_velocityScale, int i_targetFps, DistanceMode i_mode)
	: m_maxWindowSize(i_windowSize)
	, m_targetFps(i_targetFps > 0 ? i_targetFps : 30)
	, m_velocityScale(i_velocityScale)
	, m_mode(i_mode)
{
	m_window.clear();
}

void HJPointSmoothVelocity::RelativeVelocityFilter1D::Reset()
{
	m_window.clear();
	m_lowPass.Reset();
	m_hasLast = false;
	m_lastValue = 0.0f;
	m_lastValueScale = 1.0f;
}

float HJPointSmoothVelocity::RelativeVelocityFilter1D::Apply(float i_value, float i_valueScale, float i_dtSeconds)
{
	if (i_dtSeconds <= 0.0f && m_hasLast)
	{
		return i_value;
	}

	float alpha = 1.0f;
	if (m_hasLast)
	{
		float distance = 0.0f;
		if (m_mode == DistanceMode::LegacyTransition)
		{
			distance = i_value * i_valueScale - m_lastValue * m_lastValueScale;
		}
		else
		{
			distance = i_valueScale * (i_value - m_lastValue);
		}

		const int64_t duration = static_cast<int64_t>(i_dtSeconds * 1e9);
		float cumulativeDistance = distance;
		int64_t cumulativeDuration = duration;

		const int64_t kAssumedMaxDuration = 1000000000 / m_targetFps;
		const int64_t maxCumulativeDuration = (1 + static_cast<int64_t>(m_window.size())) * kAssumedMaxDuration;

		for (const auto& el : m_window)
		{
			if (cumulativeDuration + el.duration > maxCumulativeDuration)
			{
				break;
			}
			cumulativeDistance += el.distance;
			cumulativeDuration += el.duration;
		}

		if (cumulativeDuration > 0)
		{
			const float velocity = cumulativeDistance / (cumulativeDuration * 1e-9f);
			alpha = 1.0f - 1.0f / (1.0f + m_velocityScale * std::abs(velocity));
		}
		else
		{
			alpha = 1.0f;
		}

		m_window.push_front({ distance, duration });
		if (m_window.size() > m_maxWindowSize)
		{
			m_window.pop_back();
		}
	}

	m_lastValue = i_value;
	m_lastValueScale = i_valueScale;
	m_hasLast = true;

	return m_lowPass.ApplyWithAlpha(i_value, alpha);
}

HJPointf HJPointSmoothVelocity::smooth(HJPointf i_point)
{
	return smooth(i_point, m_defaultDt);
}

HJPointf HJPointSmoothVelocity::smooth(HJPointf i_point, float i_dt)
{
	const float dt = (i_dt > 0.000001f) ? i_dt : m_defaultDt;
	if (!m_filterX)
	{
		auto mode = m_distanceMode == 1 ? RelativeVelocityFilter1D::DistanceMode::ForceCurrentScale
			: RelativeVelocityFilter1D::DistanceMode::LegacyTransition;
		m_filterX = std::make_unique<RelativeVelocityFilter1D>(m_windowSize, m_velocityScale, m_targetFps, mode);
		m_filterY = std::make_unique<RelativeVelocityFilter1D>(m_windowSize, m_velocityScale, m_targetFps, mode);
	}

	HJPointf out;
	out.x = m_filterX->Apply(i_point.x, m_valueScale, dt);
	out.y = m_filterY->Apply(i_point.y, m_valueScale, dt);
	return out;
}

void HJPointSmoothVelocity::reset()
{
	if (m_filterX)
	{
		m_filterX->Reset();
	}
	if (m_filterY)
	{
		m_filterY->Reset();
	}
}

void HJPointSmoothVelocity::setParams(int i_windowSize, float i_velocityScale, int i_targetFps, int i_distanceMode)
{
	bool changed = false;
	if (i_windowSize > 0 && i_windowSize != m_windowSize)
	{
		m_windowSize = i_windowSize;
		changed = true;
	}
	if (i_velocityScale > 0.0f && i_velocityScale != m_velocityScale)
	{
		m_velocityScale = i_velocityScale;
		changed = true;
	}
	if (i_targetFps > 0 && i_targetFps != m_targetFps)
	{
		m_targetFps = i_targetFps;
		changed = true;
	}
	if (i_distanceMode == 0 || i_distanceMode == 1)
	{
		if (i_distanceMode != m_distanceMode)
		{
			m_distanceMode = i_distanceMode;
			changed = true;
		}
	}
	if (changed)
	{
		m_filterX.reset();
		m_filterY.reset();
	}
}
/////////////////////////////////////////////////////////
std::shared_ptr<HJMoreFacePointsReal> HJMorePointSmooth::smooth(std::shared_ptr<HJMoreFacePointsReal> i_point)
{
	if (!i_point)
	{
		return nullptr;
	}

	const int64_t curTs = i_point->getTimestamp();
	float dt = 0.0f;
	if (m_lastTimestamp > 0 && curTs > m_lastTimestamp)
	{
		dt = static_cast<float>(curTs - m_lastTimestamp) / 1000.0f;
	}
	m_lastTimestamp = curTs;

	auto result = HJMoreFacePointsReal::Create<HJMoreFacePointsReal>(i_point->width(), i_point->height(), i_point->isContainFace(), i_point->getTimestamp());
	result->setIsDebugPoints(i_point->isDebugPoints());
	result->setSystemTime(i_point->getSystemTime());
	result->setElapsedTime(i_point->getElapsedTime());

	if (!i_point->isContainFace())
	{
		reset();
		return result;
	}

	const auto& faces = i_point->getPoints();
	if (m_faceSmooth.size() < faces.size())
	{
		m_faceSmooth.resize(faces.size());
	}

	for (size_t i = 0; i < faces.size(); ++i)
	{
		const auto& face = faces[i];
		if (!face)
		{
			continue;
		}

		auto& smoother = m_faceSmooth[i];
		if (smoother.pointSmooth.size() < 5)
		{
			smoother.pointSmooth.resize(5);
			for (auto& ptr : smoother.pointSmooth)
			{
				if (!ptr)
				{
					ptr = HJSinglePointSmooth::createSmooth(m_policy);
				}
			}
		}
		if (smoother.rectSmooth.size() < 4)
		{
			smoother.rectSmooth.resize(4);
			for (auto& ptr : smoother.rectSmooth)
			{
				if (!ptr)
				{
					ptr = HJSinglePointSmooth::createSmooth(m_policy);
				}
			}
		}

		auto outFace = HJSingleFacePointsReal::Create();

		const auto& points = face->getFilterPt();
		const size_t ptCount = std::min<size_t>(5, points.size());
		for (size_t p = 0; p < ptCount; ++p)
		{
			outFace->add(smoother.pointSmooth[p]->smooth(points[p], dt));
		}

		const HJRectf& rect = face->getFaceRect();
		const HJPointf rectPts[4] = {
			HJPointf{rect.x, rect.y},
			HJPointf{rect.x + rect.w, rect.y},
			HJPointf{rect.x, rect.y + rect.h},
			HJPointf{rect.x + rect.w, rect.y + rect.h}};

		HJPointf smoothRectPts[4];
		for (int r = 0; r < 4; ++r)
		{
			smoothRectPts[r] = smoother.rectSmooth[r]->smooth(rectPts[r], dt);
		}

		float minX = smoothRectPts[0].x;
		float minY = smoothRectPts[0].y;
		float maxX = smoothRectPts[0].x;
		float maxY = smoothRectPts[0].y;
		for (int r = 1; r < 4; ++r)
		{
			minX = (std::min)(minX, smoothRectPts[r].x);
			minY = (std::min)(minY, smoothRectPts[r].y);
			maxX = (std::max)(maxX, smoothRectPts[r].x);
			maxY = (std::max)(maxY, smoothRectPts[r].y);
		}

		outFace->setFaceRect(minX, minY, maxX - minX, maxY - minY);
		result->add(outFace);
	}

	return result;
}

void HJMorePointSmooth::reset()
{
	m_lastTimestamp = 0;
	for (auto& smoother : m_faceSmooth)
	{
		for (auto& ptr : smoother.pointSmooth)
		{
			if (ptr)
			{
				ptr->reset();
			}
		}
		for (auto& ptr : smoother.rectSmooth)
		{
			if (ptr)
			{
				ptr->reset();
			}
		}
	}
}

HJMorePointSmooth::HJMorePointSmooth(HJPointSmoothPolicy i_policy)
	: m_policy(i_policy)
{
}


NS_HJ_END
