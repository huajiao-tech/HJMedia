#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// 场景分为直播流周期的1直播 周期2 直播即时的 直播设备 看播流周期1 看播流周期2
// 时间统一用毫秒级别
// 场景值
	enum QualityScene {
		QualitySceneNone = 0,
	QualitySceneLiveStreamPeriod = 1, // 直播流
	QualitySceneLiveDevicePeriod = 2, // 直播流设备相关的信息
	QualitySceneLiveStat = 3, // 直播即时信息
	QualitySceneLiveDeviceInfo = 4, // 开播设备指标
	QualitySceneWatchDevicePeriod = 5, // 看播
	QualitySceneLiveException = 6, //异常信息
	QualitySceneWatchStreamPeriod = 7, // 看播流信息
	QualitySceneUserDevicePeriod = 8, // 主播设备信息相关 cpu 内存 安装盘剩余空间 无关sn liveId
	QualitySceneWatchEnterLiveConsumePeriod = 9, // 看播秒开
	QualitySceneWatchEnterLiveFailedPeriod = 10, // 拉流失败
	QualitySceneWatchStuckRatePeriod = 11, // 看播卡顿统计 客户端上报卡顿率*1000000 一百万
	QualitySceneUserSdkInfoStat = 12, // sdk版本信息 触发时机：启动之后
	QualitySceneLiveEncodeInfoStat = 13, // 推流编码相关信息
	QualitySceneWatchDecodeInfoStat = 14, // 拉流解码相关信息
	QualitySceneAudioRenderDeviceNumStat = 15, // 底层音频设备渲染设备数量 触发时机：当播放器释放度的时候
	QualityScenePushStreamErrorStat = 16, // 推流失败
	QualityScenePullStreamErrorStat = 17, // 拉流失败
	QualitySceneWatchStreamEventStat = 18, // 看播过程中的事件
	QualitySceneWatchStreamScheduleConsumePeriod = 19, // 进入直播间调度耗时
	};
	// QualitySceneLiveStreamPeriod 1 直播流场景信息
	enum QualitySceneLiveStreamPeriodSceneInfo {
		LiveStreamPeriodSceneInfoNone = 0,
	LiveStreamPeriodSceneInfoSnString = 1,// sn
	LiveStreamPeriodSceneInfoPlatformString = 2,
	LiveStreamPeriodSceneInfoSDKVersionString = 3,
	};
	// QualitySceneLiveDevicePeriod 2 周期报告设备相关信息
	enum QualitySceneLiveDevicePeriodSceneInfo {
		LiveDevicePeriodSceneInfoNone = 0,
	LiveDevicePeriodSceneInfoSnString = 1,// sn
	};

	//  QualitySceneLiveStat 场景信息 3 周期报告设备相关信息
	enum QualitySceneLiveStatSceneInfo {
		LiveStatSceneInfoNone = 0,
		LiveStatSceneInfoSnString = 1,// sn
		LiveStatSceneInfoPlatformString = 2,
		LiveStatSceneInfoSDKVersionString = 3,
	};

	// QualitySceneLiveDeviceInfo 场景信息 4
	enum QualitySceneLiveDeviceInfoSceneInfo {
		LiveDeviceInfoNone = 0,
	LiveDeviceInfoSnString = 1,// sn
	};

	// QualitySceneWatchDevicePeriod 场景信息 5
	enum QualitySceneWatchDevicePeriodSceneInfo {
		WatchDevicePeriodNone = 0,
	WatchDevicePeriodSnString = 1,// sn
	};

	// QualitySceneLiveException 场景信息 6
	enum QualitySceneLiveExceptionSceneInfo {
		LiveExceptionNone = 0,
	LiveExceptionSnString = 1,
	};

	// QualitySceneWatchStreamPeriod 场景信息 7
	enum QualitySceneWatchStreamPeriodSceneInfo {
		WatchStreamPeriodNone = 0,
	WatchStreamPeriodSnString = 1,
	WatchStreamPeriodPlatformString = 2,
	WatchStreamPeriodSDKVersionString = 3,
	WatchStreamSessionIdString = 4,
	};

	// QualitySceneUserDevicePeriod 场景信息 8
	enum QualitySceneUserDevicePeriodSceneInfo {
		UserDevicePeriodNone = 0,
	};

	// QualitySceneWatchEnterLiveConsume 场景信息 9
	enum QualitySceneWatchEnterLiveConsumeSceneInfo {
		WatchEnterLiveConsumeNone = 0,
	WatchEnterLiveConsumeSnString = 1,
        WatchEnterLiveConsumePlatformString = 2,
        WatchEnterLiveConsumeSDKVersionString = 3,
	};

	// QualitySceneWatchEnterLiveFailed 场景信息 10
	enum QualitySceneWatchEnterLiveFailedSceneInfo {
		WatchEnterLiveFailedNone = 0,
	WatchEnterLiveFailedSnString = 1,
	};

	// QualitySceneWatchStuckRate 场景信息 11
	enum QualitySceneWatchStuckRateSceneInfo {
		WatchStuckRateNone = 0,
		WatchStuckRateSnString = 1,
		WatchStuckSessionIdString = 2,
		WatchStuckPlatformString = 3,
		WatchStuckSDKVersionString = 4,
	};

	// QualitySceneUserSdkInfoStat 场景信息 12 暂不需要scene信息
	enum QualitySceneUserSdkInfoStatSceneInfo {
		UserSdkInfoStatNone = 0,
	};

	// QualitySceneLiveEncodeInfoStat 场景信息 13
	enum QualitySceneLiveEncodeInfoStatSceneInfo {
		LiveEncodeInfoStatNone = 0,
	LiveEncodeInfoStatSnString = 1,
	};

	// QualitySceneWatchDecodeInfoStat 场景信息 14
	enum QualitySceneWatchDecodeInfoStatSceneInfo {
		WatchDecodeInfoStatNone = 0,
	WatchDecodeInfoStatSnString = 1,
	};

	// QualitySceneAudioRenderDeviceNumStat 场景信息 15
	enum QualitySceneAudioRenderDeviceNumStatSceneInfo {
		AudioRenderDeviceNumStatNone = 0,
	AudioRenderDeviceNumStatSnString = 1,
	};

	// QualityScenePushStreamErrorStat 场景信息 16
	enum QualityScenePushStreamErrorStatSceneInfo {
		PushStreamErrorStatNone = 0,
	PushStreamErrorStatSnString = 1,
	};

	// QualityScenePullStreamErrorStat 场景信息 17
	enum QualityScenePullStreamErrorStatSceneInfo {
		PullStreamErrorStatNone = 0,
	PullStreamErrorStatSnString = 1,
	};

	// QualitySceneWatchStreamEventStat 场景信息 18
	enum QualitySceneWatchStreamEventStatSceneInfo {
		WatchStreamEventStatNone = 0,
	WatchStreamEventStatSnString = 1,
	};

	enum QualitySceneWatchStreamScheduleConsumeSceneInfo {
		WatchStreamScheduleConsumeNone = 0,
		WatchStreamScheduleConsumeSnString = 1,
		WatchStreamScheduleConsumePlatformString = 2,
		WatchStreamScheduleConsumeSDKVersionString = 3,
	};

	// QualitySceneLiveStreamPeriod 指标
	enum QualitySceneLiveStreamPeriodMetrics {
		LiveStreamPeriodMetricsNone = 0,
	LiveStreamPeriodMetricsVideoBitrateInt64 = 1, // 视频码率
	LiveStreamPeriodMetricsVideoFramerateInt64 = 2, // 视频帧率
	LiveStreamPeriodMetricsAudioBitrateInt64 = 3, // 音频码率
	LiveStreamPeriodMetricsVideoPushDelayInt64 = 4, // 推流时延
	LiveStreamPeriodMetricsVideoPushDropInt64 = 5, // 推流丢帧率 *1000000
	LiveStreamPeriodMetricsPrepareVideoFramerateInt64 = 6, // 预设帧率 告警的时候使用 不做存储
	};

	// QualitySceneLiveDevicePeriod 指标
	enum QualitySceneLiveDevicePeriodMetrics {
		LiveDevicePeriodMetricsNone = 0,
	LiveDevicePeriodMetricsCpuUsageInt64 = 1, // cpu占用率 0.5634 存储5634 *10000
	LiveDevicePeriodMetricsGpuUsageInt64 = 2, // gpu占用率 *10000
	LiveDevicePeriodMetricsMemoryUsageInt64 = 3, // 内存占用率 *10000
	LiveDevicePeriodMetricsTemperatureInt64 = 4,// 设备温度 *100
	LiveDevicePeriodMetricsMemoryRemainingInt64 = 5, // 内存剩余 字节大小
	};

	// QualitySceneLiveStat 指标
	enum QualitySceneLiveStatMetrics {
		LiveStatMetricsNone = 0,
	LiveStatMetricsVideoEncodeString = 1,// 视频编码
	LiveStatMetricsVideoWidthInt64 = 2, // 分辨率宽
	LiveStatMetricsVideoHeightInt64 = 3, // 分辨率高
	LiveStatMetricsAudioEncodeString = 4,// 音频编码
	LiveStatMetricsAudioSampleRateInt64 = 5, //采样率
	LiveStatMetricsAudioChannelsInt64 = 6, // 声道数
	LiveStatMetricsPowerTypeString = 7, // 电源类型
	LiveStatMetricsPreFpsInt64 = 8, // 预设帧率
	LiveStatMetricsPreKbpsInt64 = 9, // 预设码率
	LiveStatMetricsHighCpuInt64 = 10, // 高性能cpu 0/1
	LiveStatMetricsVideoGamutString = 11, // 视频色域
	LiveStatMetricsSdkVersionString = 12, // sdk版本
	};


	// QualitySceneLiveDeviceInfo 指标
	enum QualitySceneLiveDeviceInfoMetrics {
		LiveDeviceInfoMetricsNone = 0,
	LiveDeviceInfoMetricsDeviceTypeString = 1, // 设备型号
	LiveDeviceInfoMetricsCpuTypeString = 2, // cpu型号
	LiveDeviceInfoMetricsGpuTypeString = 3, // gpu型号
	LiveDeviceInfoMetricsMemoryString = 4,// 内存大小
	LiveDeviceInfoMetricsRemainingString = 5, // 剩余空间 pc端 指c盘剩余
	LiveDeviceInfoMetricsMainBoardString = 6, // 主板型号
	LiveDeviceInfoMetricsBiosVersionString = 7, // bios版本
	LiveDeviceInfoMetricsAppVersionString = 8, // 版本号
	};

	// QualitySceneWatchDevicePeriod 指标
	enum QualitySceneWatchDevicePeriodMetrics {
		WatchDevicePeriodMetricsNone = 0,
	WatchDevicePeriodMetricsC2cDelayInt64 = 1,// 端到端时延
	WatchDevicePeriodMetricsPlayerDelayInt64 = 2,// 播放器时延
	};

	// QualitySceneLiveException 指标
	enum QualitySceneLiveExceptionMetrics {
		LiveExceptionMetricsNone = 0,
	LiveExceptionMetricsLiveCompanionString = 1,            //直播伴侣异常
	LiveExceptionMetricsCameraString = 2,                   //采集异常
	LiveExceptionMetricsOBSString = 3,                      //OBS 异常
	LiveExceptionMetricsVideoEncoderString = 4,             //视频编码异常
	LiveExceptionMetricsPushFailedInt64 = 5,                //推流失败, 重试次数
	LiveExceptionMetricsPushDisconnectionString = 6,        //推流断
	LiveExceptionMetricsDeviceDisconnectionString = 7,      //设备断网
	};

	// QualitySceneWatchStreamPeriod 指标
	enum QualitySceneWatchStreamPeriodMetrics {
		WatchStreamPeriodMetricsNone = 0,
	WatchStreamPeriodFPSInt64 = 1,// 看播帧率
	WatchStreamPeriodNetDelayInt64 = 2, // 网络延迟
	WatchStreamPeriodPlayDelayInt64 = 3,// 播放延迟
	WatchStreamPeriodTotalDelayInt64 = 4,// 总时延
	WatchStreamPeriodCaptureTimeInt64 = 5, // 推流端：采集时间
	WatchStreamPeriodUploadTimeInt64 = 6, // 推流端：推流时间
	WatchStreamPeriodRecvTimeInt64 = 7, // 播放端：接收时间
	WatchStreamPeriodRenderTimeInt64 = 8, // 播放端：渲染时间
	};

	// QualitySceneUserDevicePeriod 指标
	enum QualitySceneUserDevicePeriodMetrics {
		UserDevicePeriodMetricsNone = 0,
	UserDevicePeriodMetricsCpuUsageInt64 = 1, // cpu 使用率 *10000
	UserDevicePeriodMetricsMemoryUsageInt64 = 2, // 内存使用 *10000
	UserDevicePeriodMetricsStorageRemainingInt64 = 3, // 空间剩余 字节大小
	UserDevicePeriodMetricsTemperatureInt64 = 4,// 设备温度 *100
	UserDevicePeriodMetricsMemoryRemainingInt64 = 5, // 内存剩余 字节大小

	};

	// QualitySceneWatchEnterLiveConsume 指标
	enum QualitySceneWatchEnterLiveConsumeMetrics {
		WatchEnterLiveConsumeMetricsNone = 0,
	WatchEnterLiveConsumeMetricsDurationInt64 = 1,// 秒开耗时
    WatchEnterLiveConsumeMetricsEnterTimeInt64 = 2, // 进入直播间时间
    WatchEnterLiveConsumeMetricsRenderTimeInt64 = 3, // 渲染时间
	};

	// QualitySceneWatchEnterLiveFailed 指标
	enum QualitySceneWatchEnterLiveFailedMetrics {
		WatchEnterLiveFailedMetricsNone = 0,
	WatchEnterLiveFailedMetricsPlayFailedInt64 = 1, // 播放失败
	};

	// QualitySceneWatchStuckRate 指标 卡顿率
	enum QualitySceneWatchStuckRateMetrics {
		WatchStuckRateMetricsNone = 0,
		WatchStuckRateInt64 = 1, // 卡顿率 *1000000
		WatchDurationInt64 = 2, // 观看时长 毫秒 切片形式
		WatchStuckDurationInt64 = 3, // 卡顿时长 切片形式
	};

	// QualitySceneUserSdkInfo 指标 底层版本
	enum QualitySceneUserSdkInfoMetrics {
		UserSdkInfoMetricsNone = 0,
	UserSdkInfoMetricsSdkVersionString = 1, // sdk版本
	};

	// QualitySceneLiveEncodeInfo 指标 推流编码信息
	enum QualitySceneLiveEncodeInfoMetrics {
		LiveEncodeInfoMetricsNone = 0,
	LiveEncodeInfoMetricsEncodeTypeInt64 = 1, // 编码类型 1-软编码 2-硬编码
	};

	// QualitySceneWatchDecodeInfo 指标 拉流解码信息
	enum QualitySceneWatchDecodeInfoMetrics {
		WatchDecodeInfoMetricsNone = 0,
	WatchDecodeInfoMetricsDecodeTypeInt64 = 1, // 解码类型 1-软解码 2-硬解码
	};

	// QualitySceneAudioRenderDeviceNumStat 指标 当前音频设备数量
	enum QualitySceneAudioRenderDeviceNumStatMetrics {
		AudioRenderDeviceNumStatMetricsNone = 0,
	AudioRenderDeviceNumStatMetricsNumInt64 = 1, // 当前音频设备数量
	};

	// QualityScenePushStreamErrorStat 16
	enum QualityScenePushStreamErrorStatMetrics {
		PushStreamErrorStatMetricsNone = 0,
	PushStreamErrorStatMetricsReasonString = 1,
	};

	// QualityScenePullStreamErrorStat 17
	enum QualityScenePullStreamErrorStatMetrics {
		PullStreamErrorStatMetricsNone = 0,
	PullStreamErrorStatMetricsReasonString = 1,
	};

	// QualitySceneWatchStreamEventStat 18
	enum QualitySceneWatchStreamEventStatMetrics {
		WatchStreamEventStatMetricsNone = 0,
	WatchStreamEventStatMetricsLoadingInt64 = 1, // 1：标识loading开始  2：标识loading结束
	};

	enum QualitySceneWatchStreamScheduleConsumeMetrics {
		WatchStreamScheduleConsumeMetricsNone = 0,
	WatchStreamScheduleConsumeMetricsDurationInt64 = 1,
	};

#ifdef __cplusplus
};
#endif
