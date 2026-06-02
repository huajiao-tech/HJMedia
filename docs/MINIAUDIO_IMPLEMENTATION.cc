#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#include <iostream>
#include <vector>
#include <queue>
#include <mutex>
#include <chrono>
#include <cmath>
#include <atomic>

// 音频核心参数配置（可根据直播场景调整）
const int AUDIO_SAMPLE_RATE = 44100;    // 采样率
const int AUDIO_CHANNELS = 2;           // 立体声
const int AUDIO_FORMAT = ma_format_s16; // 16bit PCM（直播主流格式）
const int FRAME_DURATION_MS = 20;       // 单帧时长20ms（匹配直播推流帧）
const int FRAME_SAMPLES = AUDIO_SAMPLE_RATE * FRAME_DURATION_MS / 1000; // 单帧采样数
const int FRAME_BYTES = FRAME_SAMPLES * AUDIO_CHANNELS * ma_get_bytes_per_sample(AUDIO_FORMAT);

// 分级缓冲阈值（平衡延迟与流畅性）
const int MIN_BUFFER_MS = 200;    // 最小缓冲：低于此值填充延拓帧
const int TARGET_BUFFER_MS = 500; // 目标缓冲：稳定时维持
const int MAX_BUFFER_MS = 2000;   // 最大缓冲：防止延迟过高
const int MAX_BUFFER_FRAMES = MAX_BUFFER_MS / FRAME_DURATION_MS;

// 交叉淡化参数（消除帧拼接噪音）
const int CROSS_FADE_MS = 10;
const int CROSS_FADE_SAMPLES = AUDIO_SAMPLE_RATE * CROSS_FADE_MS / 1000;

// 音频帧结构体（含时间戳和PCM数据）
struct AudioFrame {
    int64_t pts;          // 播放时间戳（ms）
    std::vector<int16_t> data; // 16bit PCM数据（interleaved格式）
};

// 平滑音频播放器核心类
class SmoothAudioPlayer {
private:
    ma_device audioDevice;              // miniaudio设备句柄
    std::queue<AudioFrame> audioBuffer; // 音频帧缓冲队列
    std::mutex bufferMutex;             // 缓冲互斥锁
    std::atomic<int> bufferFrameCount;  // 当前缓冲帧数
    std::atomic<int64_t> currentPts;    // 当前播放到的时间戳（ms）
    AudioFrame lastValidFrame;          // 最后一帧有效音频（用于延拓）
    std::atomic<bool> isRunning;        // 播放器运行状态

    // 生成延拓衰减帧（替代静音包，避免振幅突变）
    AudioFrame generateExtendFrame(int64_t targetPts) {
        AudioFrame extendFrame;
        extendFrame.pts = targetPts;
        extendFrame.data.resize(FRAME_SAMPLES * AUDIO_CHANNELS, 0);

        if (lastValidFrame.data.empty()) {
            return extendFrame; // 初始状态无有效帧，返回静音
        }

        // 复制最后一帧数据并线性衰减（从1→0）
        const auto& lastData = lastValidFrame.data;
        for (size_t i = 0; i < extendFrame.data.size(); ++i) {
            float decayFactor = 1.0f - static_cast<float>(i) / extendFrame.data.size();
            extendFrame.data[i] = static_cast<int16_t>(lastData[i % lastData.size()] * decayFactor);
        }

        return extendFrame;
    }

    // 交叉淡化拼接两帧（前帧淡出，后帧淡入）
    void crossFadeFrames(AudioFrame& prevFrame, AudioFrame& nextFrame) {
        if (prevFrame.data.empty() || nextFrame.data.empty() || CROSS_FADE_SAMPLES <= 0) {
            return;
        }

        int fadeLen = std::min(CROSS_FADE_SAMPLES, (int)prevFrame.data.size() / AUDIO_CHANNELS);
        fadeLen = std::min(fadeLen, (int)nextFrame.data.size() / AUDIO_CHANNELS);

        // 逐采样点淡入淡出（interleaved格式：L/R/L/R...）
        for (int i = 0; i < fadeLen * AUDIO_CHANNELS; ++i) {
            float fadeIn = static_cast<float>(i) / (fadeLen * AUDIO_CHANNELS);
            float fadeOut = 1.0f - fadeIn;

            int prevIdx = prevFrame.data.size() - fadeLen * AUDIO_CHANNELS + i;
            int nextIdx = i;

            prevFrame.data[prevIdx] = static_cast<int16_t>(prevFrame.data[prevIdx] * fadeOut);
            nextFrame.data[nextIdx] = static_cast<int16_t>(nextFrame.data[nextIdx] * fadeIn);
        }
    }

    // 修正时间戳断层（避免播放跳变）
    bool fixPtsGap(AudioFrame& frame) {
        int64_t expectedPts = currentPts + FRAME_DURATION_MS;
        int64_t ptsGap = frame.pts - expectedPts;

        // 小断层（<1.5帧）：直接修正时间戳
        if (std::abs(ptsGap) <= FRAME_DURATION_MS * 1.5) {
            frame.pts = expectedPts;
            return false;
        }

        // 大断层（>100ms）：需要延拓过渡
        return true;
    }

    // miniaudio音频回调函数（核心播放逻辑）
    static void audioCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
        SmoothAudioPlayer* pPlayer = reinterpret_cast<SmoothAudioPlayer*>(pDevice->pUserData);
        if (!pPlayer || !pPlayer->isRunning.load()) {
            memset(pOutput, 0, frameCount * AUDIO_CHANNELS * ma_get_bytes_per_sample(AUDIO_FORMAT));
            return;
        }

        int16_t* pOut = reinterpret_cast<int16_t*>(pOutput);
        int samplesWritten = 0;

        // 循环填充输出缓冲区（直到填满frameCount个采样）
        while (samplesWritten < frameCount) {
            int remainingSamples = frameCount - samplesWritten;
            AudioFrame currentFrame;
            bool hasFrame = false;

            // 1. 检查缓冲状态：不足最小缓冲则填充延拓帧
            if (pPlayer->bufferFrameCount.load() < MIN_BUFFER_MS / FRAME_DURATION_MS) {
                AudioFrame extendFrame = pPlayer->generateExtendFrame(pPlayer->currentPts + FRAME_DURATION_MS);
                int copySamples = std::min(remainingSamples, (int)extendFrame.data.size() / AUDIO_CHANNELS);
                memcpy(pOut + samplesWritten * AUDIO_CHANNELS, 
                       extendFrame.data.data(), 
                       copySamples * AUDIO_CHANNELS * sizeof(int16_t));
                samplesWritten += copySamples;
                pPlayer->currentPts += copySamples * 1000 / AUDIO_SAMPLE_RATE;
                continue;
            }

            // 2. 从缓冲取帧（加锁保护）
            {
                std::lock_guard<std::mutex> lock(pPlayer->bufferMutex);
                if (!pPlayer->audioBuffer.empty()) {
                    currentFrame = pPlayer->audioBuffer.front();
                    pPlayer->audioBuffer.pop();
                    pPlayer->bufferFrameCount--;
                    hasFrame = true;
                }
            }

            // 3. 无有效帧：填充延拓帧
            if (!hasFrame) {
                AudioFrame extendFrame = pPlayer->generateExtendFrame(pPlayer->currentPts + FRAME_DURATION_MS);
                int copySamples = std::min(remainingSamples, (int)extendFrame.data.size() / AUDIO_CHANNELS);
                memcpy(pOut + samplesWritten * AUDIO_CHANNELS, 
                       extendFrame.data.data(), 
                       copySamples * AUDIO_CHANNELS * sizeof(int16_t));
                samplesWritten += copySamples;
                pPlayer->currentPts += copySamples * 1000 / AUDIO_SAMPLE_RATE;
                continue;
            }

            // 4. 修正时间戳断层
            bool hasBigGap = pPlayer->fixPtsGap(currentFrame);
            if (hasBigGap) {
                // 大断层：先播放延拓帧过渡，再播放有效帧（交叉淡化）
                AudioFrame extendFrame = pPlayer->generateExtendFrame(pPlayer->currentPts + FRAME_DURATION_MS);
                pPlayer->crossFadeFrames(extendFrame, currentFrame);

                // 写入延拓帧
                int extendSamples = std::min(remainingSamples, (int)extendFrame.data.size() / AUDIO_CHANNELS);
                memcpy(pOut + samplesWritten * AUDIO_CHANNELS, 
                       extendFrame.data.data(), 
                       extendSamples * AUDIO_CHANNELS * sizeof(int16_t));
                samplesWritten += extendSamples;
                pPlayer->currentPts += extendSamples * 1000 / AUDIO_SAMPLE_RATE;

                if (samplesWritten >= frameCount) break;

                // 写入有效帧
                remainingSamples = frameCount - samplesWritten;
                int frameSamples = std::min(remainingSamples, (int)currentFrame.data.size() / AUDIO_CHANNELS);
                memcpy(pOut + samplesWritten * AUDIO_CHANNELS, 
                       currentFrame.data.data(), 
                       frameSamples * AUDIO_CHANNELS * sizeof(int16_t));
                samplesWritten += frameSamples;
                pPlayer->currentPts += frameSamples * 1000 / AUDIO_SAMPLE_RATE;
            } else {
                // 无断层：直接写入有效帧
                int frameSamples = std::min(remainingSamples, (int)currentFrame.data.size() / AUDIO_CHANNELS);
                memcpy(pOut + samplesWritten * AUDIO_CHANNELS, 
                       currentFrame.data.data(), 
                       frameSamples * AUDIO_CHANNELS * sizeof(int16_t));
                samplesWritten += frameSamples;
                pPlayer->currentPts += frameSamples * 1000 / AUDIO_SAMPLE_RATE;

                // 更新最后一帧有效音频
                pPlayer->lastValidFrame = currentFrame;
            }
        }
    }

    // 初始化miniaudio设备
    bool initAudioDevice() {
        ma_device_config deviceConfig = ma_device_config_init(ma_device_type_playback);
        deviceConfig.playback.format = AUDIO_FORMAT;
        deviceConfig.playback.channels = AUDIO_CHANNELS;
        deviceConfig.sampleRate = AUDIO_SAMPLE_RATE;
        deviceConfig.dataCallback = audioCallback;
        deviceConfig.pUserData = this;
        deviceConfig.playback.pDeviceID = NULL; // 使用默认输出设备
        deviceConfig.periodSizeInFrames = FRAME_SAMPLES; // 周期大小匹配帧大小

        // 初始化设备
        ma_result result = ma_device_init(NULL, &deviceConfig, &audioDevice);
        if (result != MA_SUCCESS) {
            std::cerr << "miniaudio device init failed: " << ma_result_description(result) << std::endl;
            return false;
        }

        // 启动设备
        result = ma_device_start(&audioDevice);
        if (result != MA_SUCCESS) {
            std::cerr << "miniaudio device start failed: " << ma_result_description(result) << std::endl;
            ma_device_uninit(&audioDevice);
            return false;
        }

        return true;
    }

public:
    SmoothAudioPlayer() 
        : bufferFrameCount(0), currentPts(0), isRunning(false) {
        // 初始化设备
        if (!initAudioDevice()) {
            std::cerr << "Audio device init failed!" << std::endl;
            return;
        }
        isRunning = true;
    }

    ~SmoothAudioPlayer() {
        stop();
        ma_device_uninit(&audioDevice); // 释放音频设备
    }

    // 推送音频帧到缓冲（直播拉流数据入口）
    void pushAudioFrame(const AudioFrame& frame) {
        if (!isRunning.load() || frame.data.empty()) {
            return;
        }

        std::lock_guard<std::mutex> lock(bufferMutex);
        // 超过最大缓冲则丢弃（防止延迟过高）
        if (bufferFrameCount.load() >= MAX_BUFFER_FRAMES) {
            return;
        }

        audioBuffer.push(frame);
        bufferFrameCount++;
    }

    // 停止播放器
    void stop() {
        isRunning = false;
        ma_device_stop(&audioDevice);

        // 清空缓冲
        std::lock_guard<std::mutex> lock(bufferMutex);
        while (!audioBuffer.empty()) {
            audioBuffer.pop();
        }
        bufferFrameCount = 0;
        currentPts = 0;
        lastValidFrame.data.clear();
    }

    // 获取当前缓冲时长（ms）
    int getBufferDurationMs() const {
        return bufferFrameCount.load() * FRAME_DURATION_MS;
    }
};

// 测试代码：模拟直播拉流的音频帧推送（含网络波动）
int main() {
    // 创建播放器
    SmoothAudioPlayer player;
    if (!player.getBufferDurationMs()) {
        std::cerr << "Player init failed!" << std::endl;
        return -1;
    }

    // 模拟推送100帧音频（实际场景替换为直播拉流数据）
    int64_t testPts = 0;
    for (int i = 0; i < 100; ++i) {
        AudioFrame frame;
        frame.pts = testPts;
        frame.data.resize(FRAME_SAMPLES * AUDIO_CHANNELS);

        // 生成440Hz正弦波测试音频（模拟有效直播音频）
        for (int j = 0; j < FRAME_SAMPLES; ++j) {
            float time = static_cast<float>(testPts * FRAME_SAMPLES + j) / AUDIO_SAMPLE_RATE;
            int16_t sample = static_cast<int16_t>(32767 * sin(2 * M_PI * 440 * time)); // 440Hz A调
            frame.data[j * AUDIO_CHANNELS] = sample;     // 左声道
            frame.data[j * AUDIO_CHANNELS + 1] = sample; // 右声道
        }

        // 推送帧到播放器
        player.pushAudioFrame(frame);
        testPts += FRAME_DURATION_MS;

        // 模拟网络波动：每10帧丢1帧（制造数据不连续）
        if (i % 10 == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }

    // 持续播放10秒后退出
    std::cout << "Playing audio... (10s)" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(10));

    // 停止播放器
    player.stop();
    std::cout << "Playback stopped." << std::endl;

    return 0;
}