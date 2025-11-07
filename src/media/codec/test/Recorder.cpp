/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "Recorder.h"
#include "AVCodecSampleLog.h"
#include "dfx/error/AVCodecSampleError.h"

#undef LOG_TAG
#define LOG_TAG "recorder"

namespace {
using namespace std::chrono_literals;
constexpr int64_t MICROSECOND = 1000000;
constexpr int32_t INPUT_FRAME_BYTES = 2 * 1024;
} // namespace

Recorder::~Recorder() { StartRelease(); }

int32_t Recorder::Init(SampleInfo &sampleInfo) {
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(!isStarted_, AVCODEC_SAMPLE_ERR_ERROR, "Already started.");
    CHECK_AND_RETURN_RET_LOG(videoEncoder_ == nullptr && muxer_ == nullptr, AVCODEC_SAMPLE_ERR_ERROR,
                             "Already started.");

    sampleInfo_ = sampleInfo;
    // Audio Capturer Init
    audioEncoder_ = std::make_unique<AudioEncoder>();
    audioCapturer_ = std::make_unique<AudioCapturer>();

    videoEncoder_ = std::make_unique<VideoEncoder>();
    muxer_ = std::make_unique<Muxer>();

    int32_t ret = muxer_->Create(sampleInfo_.outputFd);
    CHECK_AND_RETURN_RET_LOG(ret == AVCODEC_SAMPLE_ERR_OK, ret, "Create muxer with fd(%{public}d) failed",
                             sampleInfo_.outputFd);
    ret = muxer_->Config(sampleInfo_);

    ret = CreateAudioEncoder();
    CHECK_AND_RETURN_RET_LOG(ret == AVCODEC_SAMPLE_ERR_OK, ret, "Create audio encoder failed");

    // Init AudioCapturer
    audioCapturer_->AudioCapturerInit(sampleInfo_, audioEncContext_);

    ret = CreateVideoEncoder();
    CHECK_AND_RETURN_RET_LOG(ret == AVCODEC_SAMPLE_ERR_OK, ret, "Create video encoder failed");

    sampleInfo.window = sampleInfo_.window;

    releaseThread_ = nullptr;
    AVCODEC_SAMPLE_LOGI("Succeed");
    return AVCODEC_SAMPLE_ERR_OK;
}

int32_t Recorder::Start() {
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(!isStarted_, AVCODEC_SAMPLE_ERR_ERROR, "Already started.");
    CHECK_AND_RETURN_RET_LOG(encContext_ != nullptr, AVCODEC_SAMPLE_ERR_ERROR, "Already started.");
    CHECK_AND_RETURN_RET_LOG(videoEncoder_ != nullptr && muxer_ != nullptr, AVCODEC_SAMPLE_ERR_ERROR,
                             "Already started.");

    int32_t ret = muxer_->Start();
    CHECK_AND_RETURN_RET_LOG(ret == AVCODEC_SAMPLE_ERR_OK, ret, "Muxer start failed");
    ret = videoEncoder_->Start();
    CHECK_AND_RETURN_RET_LOG(ret == AVCODEC_SAMPLE_ERR_OK, ret, "Encoder start failed");

    isStarted_ = true;
    encOutputThread_ = std::make_unique<std::thread>(&Recorder::EncOutputThread, this);
    if (encOutputThread_ == nullptr) {
        AVCODEC_SAMPLE_LOGE("Create thread failed");
        StartRelease();
        return AVCODEC_SAMPLE_ERR_ERROR;
    }

    if (audioEncContext_) {
        // Start AudioCapturer
        audioCapturer_->AudioCapturerStart();

        ret = audioEncoder_->Start();
        CHECK_AND_RETURN_RET_LOG(ret == AVCODEC_SAMPLE_ERR_OK, ret, "Audio Encoder start failed");
        isStarted_ = true;
        audioEncInputThread_ = std::make_unique<std::thread>(&Recorder::AudioEncInputThread, this);
        audioEncOutputThread_ = std::make_unique<std::thread>(&Recorder::AudioEncOutputThread, this);
        if (audioEncInputThread_ == nullptr || audioEncOutputThread_ == nullptr) {
            AVCODEC_SAMPLE_LOGE("Create thread failed");
            StartRelease();
            return AVCODEC_SAMPLE_ERR_ERROR;
        }

        // 清空播放缓存队列
        if (audioEncContext_ != nullptr) {
            audioEncContext_->ClearCache();
        }
    }

    AVCODEC_SAMPLE_LOGI("Succeed");
    return AVCODEC_SAMPLE_ERR_OK;
}

void Recorder::EncOutputThread() {
    while (true) {
        CHECK_AND_BREAK_LOG(isStarted_, "Work done, thread out");
        std::unique_lock<std::mutex> lock(encContext_->outputMutex);
        bool condRet = encContext_->outputCond.wait_for(
            lock, 5s, [this]() { return !isStarted_ || !encContext_->outputBufferInfoQueue.empty(); });
        CHECK_AND_BREAK_LOG(isStarted_, "Work done, thread out");
        CHECK_AND_CONTINUE_LOG(!encContext_->outputBufferInfoQueue.empty(),
                               "Buffer queue is empty, continue, cond ret: %{public}d", condRet);

        CodecBufferInfo bufferInfo = encContext_->outputBufferInfoQueue.front();
        encContext_->outputBufferInfoQueue.pop();
        CHECK_AND_BREAK_LOG(!(bufferInfo.attr.flags & AVCODEC_BUFFER_FLAGS_EOS), "Catch EOS, thread out");
        lock.unlock();
        if ((bufferInfo.attr.flags & AVCODEC_BUFFER_FLAGS_SYNC_FRAME) ||
            (bufferInfo.attr.flags == AVCODEC_BUFFER_FLAGS_NONE)) {
            encContext_->outputFrameCount++;
            bufferInfo.attr.pts = encContext_->outputFrameCount * MICROSECOND / sampleInfo_.frameRate;
        } else {
            bufferInfo.attr.pts = 0;
        }
        AVCODEC_SAMPLE_LOGW("Out buffer count: %{public}u, size: %{public}d, flag: %{public}u, pts: %{public}" PRId64,
                            encContext_->outputFrameCount, bufferInfo.attr.size, bufferInfo.attr.flags,
                            bufferInfo.attr.pts);

        muxer_->WriteSample(muxer_->GetVideoTrackId(), reinterpret_cast<OH_AVBuffer *>(bufferInfo.buffer),
                            bufferInfo.attr);
        int32_t ret = videoEncoder_->FreeOutputBuffer(bufferInfo.bufferIndex);
        CHECK_AND_BREAK_LOG(ret == AVCODEC_SAMPLE_ERR_OK, "Encoder output thread out");
    }
    AVCODEC_SAMPLE_LOGI("Exit, frame count: %{public}u", encContext_->outputFrameCount);
    StartRelease();
}

void Recorder::StartRelease() {
    if (releaseThread_ == nullptr) {
        AVCODEC_SAMPLE_LOGI("Start release CodecTest");
        releaseThread_ = std::make_unique<std::thread>(&Recorder::Release, this);
    }
}

void Recorder::Release() {
    std::lock_guard<std::mutex> lock(mutex_);
    isStarted_ = false;
    if (encOutputThread_ && encOutputThread_->joinable()) {
        encOutputThread_->join();
        encOutputThread_.reset();
    }
    if (audioEncInputThread_ && audioEncInputThread_->joinable()) {
        audioEncContext_->inputCond.notify_all();
        audioEncInputThread_->join();
        audioEncInputThread_.reset();
    }
    if (audioEncOutputThread_ && audioEncOutputThread_->joinable()) {
        audioEncContext_->outputCond.notify_all();
        audioEncOutputThread_->join();
        audioEncOutputThread_.reset();
    }
    if (muxer_ != nullptr) {
        muxer_->Release();
        muxer_.reset();
        AVCODEC_SAMPLE_LOGI("Muxer release successful");
    }
    if (videoEncoder_ != nullptr) {
        videoEncoder_->Stop();
        if (sampleInfo_.window != nullptr) {
            OH_NativeWindow_DestroyNativeWindow(sampleInfo_.window);
            sampleInfo_.window = nullptr;
        }
        videoEncoder_->Release();
        videoEncoder_.reset();
        AVCODEC_SAMPLE_LOGI("Video encoder release successful");
    }
    if (audioEncoder_ != nullptr) {
        audioEncoder_->Stop();
        audioEncoder_->Release();
        audioEncoder_.reset();
        AVCODEC_SAMPLE_LOGI("Audio encoder release successful");
    }
    if (audioCapturer_ != nullptr) {
        audioCapturer_->AudioCapturerRelease();
        audioCapturer_.reset();
        AVCODEC_SAMPLE_LOGI("Audio Capturer release successful");
    }
    if (audioEncContext_ != nullptr) {
        delete audioEncContext_;
        audioEncContext_ = nullptr;
    }
    if (encContext_ != nullptr) {
        delete encContext_;
        encContext_ = nullptr;
    }
    doneCond_.notify_all();
    AVCODEC_SAMPLE_LOGI("Succeed");
}

int32_t Recorder::WaitForDone() {
    AVCODEC_SAMPLE_LOGI("Wait called");
    std::unique_lock<std::mutex> lock(mutex_);
    doneCond_.wait(lock);
    if (releaseThread_ && releaseThread_->joinable()) {
        releaseThread_->join();
        releaseThread_.reset();
    }
    AVCODEC_SAMPLE_LOGI("Done");
    return AVCODEC_SAMPLE_ERR_OK;
}

int32_t Recorder::Stop() {
    int32_t ret = videoEncoder_->NotifyEndOfStream();
    CHECK_AND_RETURN_RET_LOG(ret == AVCODEC_SAMPLE_ERR_OK, ret, "Encoder notifyEndOfStream failed");
    return WaitForDone();
}

int32_t Recorder::CreateVideoEncoder() {
    int32_t ret = videoEncoder_->Create(sampleInfo_.videoCodecMime);
    CHECK_AND_RETURN_RET_LOG(ret == AVCODEC_SAMPLE_ERR_OK, ret, "Create video encoder failed");

    encContext_ = new CodecUserData;
    ret = videoEncoder_->Config(sampleInfo_, encContext_);
    CHECK_AND_RETURN_RET_LOG(ret == AVCODEC_SAMPLE_ERR_OK, ret, "Encoder config failed");

    return AVCODEC_SAMPLE_ERR_OK;
}

int32_t Recorder::CreateAudioEncoder() {
    int32_t ret = audioEncoder_->Create(sampleInfo_.audioCodecMime);
    CHECK_AND_RETURN_RET_LOG(ret == AVCODEC_SAMPLE_ERR_OK, ret, "Create audio encoder(%{public}s) failed",
                             sampleInfo_.audioCodecMime.c_str());
    AVCODEC_SAMPLE_LOGI("Create audio encoder(%{public}s)", sampleInfo_.audioCodecMime.c_str());

    audioEncContext_ = new CodecUserData;
    ret = audioEncoder_->Config(sampleInfo_, audioEncContext_);
    CHECK_AND_RETURN_RET_LOG(ret == AVCODEC_SAMPLE_ERR_OK, ret, "Encoder config failed");

    return AVCODEC_SAMPLE_ERR_OK;
}

void Recorder::AudioEncInputThread() {
    while (true) {
        CHECK_AND_BREAK_LOG(isStarted_, "Encoder input thread out");
        std::unique_lock<std::mutex> lock(audioEncContext_->inputMutex);
        bool condRet = audioEncContext_->inputCond.wait_for(lock, 5s, [this]() {
            return !isStarted_ || (!audioEncContext_->inputBufferInfoQueue.empty() &&
                                   (audioEncContext_->remainlen >= sampleInfo_.audioMaxInputSize));
        });

        CHECK_AND_CONTINUE_LOG(!audioEncContext_->inputBufferInfoQueue.empty(),
                               "Audio Buffer queue is empty, continue, cond ret: %{public}d", condRet);

        if (isStarted_ && audioEncContext_->remainlen < sampleInfo_.audioMaxInputSize) {
            continue;
        }

        CodecBufferInfo bufferInfo = audioEncContext_->inputBufferInfoQueue.front();
        audioEncContext_->inputBufferInfoQueue.pop();
        audioEncContext_->inputFrameCount++;
        
        uint8_t *inputBufferAddr = OH_AVBuffer_GetAddr(reinterpret_cast<OH_AVBuffer *>(bufferInfo.buffer));
        audioEncContext_->ReadCache(inputBufferAddr, sampleInfo_.audioMaxInputSize);
        lock.unlock();

        bufferInfo.attr.size = sampleInfo_.audioMaxInputSize;
        if (isFirstFrame_) {
            bufferInfo.attr.flags = AVCODEC_BUFFER_FLAGS_CODEC_DATA;
            isFirstFrame_ = false;
        } else {
            bufferInfo.attr.flags = AVCODEC_BUFFER_FLAGS_NONE;
        }
        int32_t ret = audioEncoder_->PushInputData(bufferInfo);
        CHECK_AND_BREAK_LOG(ret == AVCODEC_SAMPLE_ERR_OK, "Push data failed, thread out");
    }
}

void Recorder::AudioEncOutputThread() {
    while (true) {
        CHECK_AND_BREAK_LOG(isStarted_, "Work done, thread out");
        std::unique_lock<std::mutex> lock(audioEncContext_->outputMutex);
        bool condRet = audioEncContext_->outputCond.wait_for(
            lock, 5s, [this]() { return !isStarted_ || !audioEncContext_->outputBufferInfoQueue.empty(); });
        CHECK_AND_BREAK_LOG(isStarted_, "Work done, thread out");
        CHECK_AND_CONTINUE_LOG(!audioEncContext_->outputBufferInfoQueue.empty(),
                               "Buffer queue is empty, continue, cond ret: %{public}d", condRet);

        CodecBufferInfo bufferInfo = audioEncContext_->outputBufferInfoQueue.front();
        audioEncContext_->outputBufferInfoQueue.pop();
        audioEncContext_->outputFrameCount++;
        AVCODEC_SAMPLE_LOGW(
            "Audio Out buffer count: %{public}u, size: %{public}d, flag: %{public}u, pts: %{public}" PRId64,
            audioEncContext_->outputFrameCount, bufferInfo.attr.size, bufferInfo.attr.flags, bufferInfo.attr.pts);
//        lock.unlock();
        muxer_->WriteSample(muxer_->GetAudioTrackId(), reinterpret_cast<OH_AVBuffer *>(bufferInfo.buffer),
                            bufferInfo.attr);
        int32_t ret = audioEncoder_->FreeOutputData(bufferInfo.bufferIndex);
        CHECK_AND_BREAK_LOG(ret == AVCODEC_SAMPLE_ERR_OK, "Encoder output thread out");
    }
    AVCODEC_SAMPLE_LOGI("Exit, frame count: %{public}u", audioEncContext_->inputFrameCount);
    StartRelease();
}