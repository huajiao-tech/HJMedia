#pragma once

#include "HJPrerequisites.h"
#include "HJComUtils.h"
#include "HJMediaUtils.h"

#include <memory>
#include <vector>
#include <deque>
#include <cstdint>

NS_HJ_BEGIN

enum class HJPointSmoothPolicy
{
	None,
	OneEuro,
	Velocity,
};

class HJMoreFacePointsReal;

class HJSinglePointSmooth : public HJBaseSharedObject
{
public:
	HJSinglePointSmooth() = default;
	virtual ~HJSinglePointSmooth() = default;

	using Ptr = std::shared_ptr<HJSinglePointSmooth>;
	using Wtr = std::weak_ptr<HJSinglePointSmooth>;

	static HJSinglePointSmooth::Ptr createSmooth(HJPointSmoothPolicy i_policy = HJPointSmoothPolicy::OneEuro);
	virtual HJPointf smooth(HJPointf i_point) = 0;
	virtual HJPointf smooth(HJPointf i_point, float i_dt) = 0;
	virtual void reset() = 0;

private:
};

class HJPointSmoothNone : public HJSinglePointSmooth
{
public:
	HJ_DEFINE_CREATE(HJPointSmoothNone);
	HJPointSmoothNone() = default;
	virtual ~HJPointSmoothNone() = default;

	HJPointf smooth(HJPointf i_point) override;
	HJPointf smooth(HJPointf i_point, float i_dt) override;
	void reset() override;
};

class HJPointSmoothOneEuro : public HJSinglePointSmooth
{
public:
	HJ_DEFINE_CREATE(HJPointSmoothOneEuro);
	HJPointSmoothOneEuro() = default;
	virtual ~HJPointSmoothOneEuro() = default;

	HJPointf smooth(HJPointf i_point) override;
	HJPointf smooth(HJPointf i_point, float i_dt) override;
	void reset() override;
	void setParams(float i_minCutoff, float i_beta, float i_dCutoff);
private:
	bool m_inited = false;
	float m_minCutoff = 2.2f;
	float m_beta = 0.06f;
	float m_dCutoff = 1.5f;
	float m_defaultDt = 1.0f / 30.0f;

	HJPointf m_prevFiltered = HJ_POINT_ZERO;
	HJPointf m_prevDeriv = HJ_POINT_ZERO;
};
class HJPointSmoothVelocity : public HJSinglePointSmooth
{
public:
	HJ_DEFINE_CREATE(HJPointSmoothVelocity);
	HJPointSmoothVelocity() = default;
	virtual ~HJPointSmoothVelocity();

	HJPointf smooth(HJPointf i_point) override;
	HJPointf smooth(HJPointf i_point, float i_dt) override;
	void reset() override;
	void setParams(int i_windowSize, float i_velocityScale, int i_targetFps, int i_distanceMode = 0);
private:
	struct LowPassFilter1D
	{
		explicit LowPassFilter1D(float i_alpha)
			: m_alpha(i_alpha)
		{
		}

		float Apply(float i_value)
		{
			float result;
			if (m_inited)
			{
				result = m_alpha * i_value + (1.0f - m_alpha) * m_value;
			}
			else
			{
				result = i_value;
				m_inited = true;
			}
			m_raw = i_value;
			m_value = result;
			return result;
		}

		float ApplyWithAlpha(float i_value, float i_alpha)
		{
			if (i_alpha >= 0.0f && i_alpha <= 1.0f)
			{
				m_alpha = i_alpha;
			}
			return Apply(i_value);
		}

		void Reset()
		{
			m_inited = false;
		}

		bool m_inited = false;
		float m_alpha = 1.0f;
		float m_raw = 0.0f;
		float m_value = 0.0f;
	};

	class RelativeVelocityFilter1D
	{
	public:
		enum class DistanceMode
		{
			LegacyTransition = 0,
			ForceCurrentScale = 1
		};

		RelativeVelocityFilter1D(size_t i_windowSize, float i_velocityScale, int i_targetFps, DistanceMode i_mode);
		void Reset();
		float Apply(float i_value, float i_valueScale, float i_dtSeconds);

	private:
		struct WindowElement
		{
			float distance = 0.0f;
			int64_t duration = 0;
		};

		float m_lastValue = 0.0f;
		float m_lastValueScale = 1.0f;
		bool m_hasLast = false;

		size_t m_maxWindowSize = 5;
		int m_targetFps = 30;
		std::deque<WindowElement> m_window;
		LowPassFilter1D m_lowPass{ 1.0f };
		float m_velocityScale = 10.0f;
		DistanceMode m_mode = DistanceMode::LegacyTransition;
	};
	std::unique_ptr<RelativeVelocityFilter1D> m_filterX;
	std::unique_ptr<RelativeVelocityFilter1D> m_filterY;
	int m_windowSize = 5;
	float m_velocityScale = 10.0f;
	int m_targetFps = 30;
	int m_distanceMode = 0;
	float m_defaultDt = 1.0f / 30.0f;
	float m_valueScale = 1.0f;
};
class HJMorePointSmooth : public HJBaseSharedObject
{
public:
	HJ_DEFINE_CREATE(HJMorePointSmooth);
	explicit HJMorePointSmooth(HJPointSmoothPolicy i_policy = HJPointSmoothPolicy::Velocity);
	virtual ~HJMorePointSmooth() = default;
	void reset();
	std::shared_ptr<HJMoreFacePointsReal> smooth(std::shared_ptr<HJMoreFacePointsReal> i_point);
private:
	struct FaceSmoother
	{
		std::vector<HJSinglePointSmooth::Ptr> pointSmooth;
		std::vector<HJSinglePointSmooth::Ptr> rectSmooth;
	};

	std::vector<FaceSmoother> m_faceSmooth;
	HJPointSmoothPolicy m_policy = HJPointSmoothPolicy::Velocity;
	int64_t m_lastTimestamp = 0;

};

NS_HJ_END

