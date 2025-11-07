#pragma once

#include "HJMediaUtils.h"
#include "HJPrerequisites.h"
#include <algorithm>

NS_HJ_BEGIN

// 定义包含5个关键点的数组类型
using HJLandmarks = std::vector<HJPointf>;

/**
 * @class LandmarkSmoother
 * @brief 使用自适应指数移动平均算法平滑人脸关键点。
 *
 * 该算法根据关键点的移动速度动态调整平滑系数。
 * - 对于微小移动（可能由检测噪声引起），应用强平滑以减少抖动。
 * - 对于大幅度移动（真实的头部运动），快速适应新位置以减少延迟。
 */
class HJLandmarkSmoother
{
public:
  HJ_DEFINE_CREATE(HJLandmarkSmoother);
  virtual ~HJLandmarkSmoother() = default;
  /**
   * @brief 构造函数
   * @param min_alpha 用于平滑的最小alpha值（用于小运动）。值越小，平滑效果越强。建议范围：0.05 -
0.2。
   * @param max_alpha 用于平滑的最大alpha值（用于大运动）。值越大，适应速度越快。建议范围：0.6 -
0.95。
   * @param movement_threshold
区分小运动和大运动的距离阈值。当移动距离达到此值时，alpha会从min_alpha线性增加到max_alpha。
   */
  HJLandmarkSmoother(float min_alpha = 0.05f, float max_alpha = 0.6f, float movement_threshold = 25.0f)
      : min_alpha_(min_alpha),
        max_alpha_(max_alpha),
        movement_threshold_(movement_threshold),
        is_initialized_(false)
  {
  }

  /**
   * @brief 对一帧新的关键点数据进行平滑处理。
   * @param current_landmarks 从检测器获取的当前帧的原始关键点。
   * @return Landmarks 平滑处理后的关键点。
   */
  HJLandmarks smooth(const HJLandmarks &current_landmarks)
  {
    if (!is_initialized_)
    {
      // 如果是第一帧，直接使用当前值进行初始化
      smoothed_landmarks_ = current_landmarks;
      is_initialized_ = true;
      return smoothed_landmarks_;
    }

    HJLandmarks new_smoothed_landmarks;
    for (int i = 0; i < 5; i++)
    {
      new_smoothed_landmarks.push_back(HJPointf{0.f, 0.f});
    }
    for (size_t i = 0; i < current_landmarks.size(); ++i)
    {
      // 1. 计算当前帧原始点与上一帧平滑点的距离
      float dx = current_landmarks[i].x - smoothed_landmarks_[i].x;
      float dy = current_landmarks[i].y - smoothed_landmarks_[i].y;
      float distance = std::sqrt(dx * dx + dy * dy);

      // 2. 根据距离自适应计算alpha值
      // 使用min函数确保比例不超过1.0，从而将alpha限制在[min_alpha, max_alpha]区间内
      float ratio = std::min(distance / movement_threshold_, 1.0f);
      float alpha = min_alpha_ + ratio * (max_alpha_ - min_alpha_);

      // 3. 应用指数移动平均公式
      // new_value = alpha * current_value + (1 - alpha) * previous_value
      new_smoothed_landmarks[i].x = alpha * current_landmarks[i].x + (1.0f - alpha) * smoothed_landmarks_[i].x;
      new_smoothed_landmarks[i].y = alpha * current_landmarks[i].y + (1.0f - alpha) * smoothed_landmarks_[i].y;
    }

    // 更新状态以备下一帧使用
    smoothed_landmarks_ = new_smoothed_landmarks;
    return smoothed_landmarks_;
  }
  void reset()
  {
    is_initialized_ = false;
  }

private:
  //      // 打印关键点，用于调试
  //      static void print_landmarks(const Landmarks& landmarks, const std::string& title) {
  //          std::cout << title << ": [";
  //          for (size_t i = 0; i < landmarks.size(); ++i) {
  //              std::cout << "(" << landmarks[i].x << ", " << landmarks[i].y << ")" << (i == landmarks.size() - 1 ? "" : ", ");
  //          }
  //          std::cout << "]" << std::endl;
  //      }

  float min_alpha_;
  float max_alpha_;
  float movement_threshold_;
  bool is_initialized_ = false;
  HJLandmarks smoothed_landmarks_;
};

class HJLandmarkDistanceStat
{
public:
  HJ_DEFINE_CREATE(HJLandmarkDistanceStat);
  HJLandmarkDistanceStat() = default;
  virtual ~HJLandmarkDistanceStat() = default;

  float stat(const HJLandmarks &current_landmarks)
  {
    if (m_bReset)
    {
      m_catchLandmarks = current_landmarks;
      m_bReset = false;
      return 0.f;
    }

    float sum = 0.f;
    for (size_t i = 0; i < current_landmarks.size(); ++i)
    {
      // 1. 计算当前帧原始点与上一帧平滑点的距离
      float dx = current_landmarks[i].x - m_catchLandmarks[i].x;
      float dy = current_landmarks[i].y - m_catchLandmarks[i].y;
      float distance = std::sqrt(dx * dx + dy * dy);
      sum += distance;
    }
    m_catchLandmarks = current_landmarks;
    return sum / current_landmarks.size();
  }
  void reset()
  {
    m_bReset = true;
  }
  void save(const HJLandmarks &filter_landmarks)
  {
      m_catchLandmarks = filter_landmarks;  
  }  

private:
  bool m_bReset = true;
  HJLandmarks m_catchLandmarks;
};

NS_HJ_END