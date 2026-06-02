#import "HJPlayerContext.h"

#include <string>

#include "HJEntryContext.h"
#include "HJError.h"

NS_HJ_USING;

namespace {

std::string HJNSStringToStdString(NSString *value) {
    if (value == nil || value.length == 0) {
        return std::string();
    }
    return std::string(value.UTF8String);
}

HJEntryContextPlayerInfo HJMakeContextInfo(HJPlayerContextConfig *config) {
    HJEntryContextPlayerInfo info;
    info.logIsValid = config.logEnabled;
    info.logDir = HJNSStringToStdString(config.logDirectory);
    info.logLevel = static_cast<int>(config.logLevel);
    info.logMode = static_cast<int>(config.logMode);
    info.logMaxFileSize = static_cast<int>(config.logMaxFileSize);
    info.logMaxFileNum = static_cast<int>(config.logMaxFileCount);
    info.medias_dir = HJNSStringToStdString(config.mediasDirectory);
    info.medias_cache_max = static_cast<int>(config.mediasCacheMax);
    info.download_retry_max = static_cast<int>(config.downloadRetryMax);
    return info;
}

}  // namespace

@implementation HJPlayerContextConfig

- (instancetype)init {
    self = [super init];
    if (self) {
        _logEnabled = YES;
        _logDirectory = [NSTemporaryDirectory() copy] ?: @"";
        _logLevel = HJPlayerContextLogLevelInfo;
        _logMode = HJPlayerLogModeConsole;
        _logMaxFileSize = 5 * 1024 * 1024;
        _logMaxFileCount = 2;
        _mediasDirectory = @"";
        _mediasCacheMax = 200;
        _downloadRetryMax = 10;
    }
    return self;
}

@end

@interface HJPlayerContext ()

@property (nonatomic, assign, readwrite, getter=isStarted) BOOL started;

@end

@implementation HJPlayerContext {
    BOOL _has_attempted_start;
    NSInteger _start_result;
}

+ (instancetype)sharedInstance {
    static HJPlayerContext *shared_context = nil;
    static dispatch_once_t once_token;
    dispatch_once(&once_token, ^{
        shared_context = [[HJPlayerContext alloc] init];
    });
    return shared_context;
}

- (instancetype)init {
    self = [super init];
    if (self) {
        _started = NO;
        _has_attempted_start = NO;
        _start_result = HJ_OK;
    }
    return self;
}

- (NSInteger)startWithConfig:(HJPlayerContextConfig *)config {
    @synchronized(self) {
        if (_has_attempted_start) {
            return _start_result;
        }

        HJPlayerContextConfig *resolved_config = config;
        if (resolved_config == nil) {
            resolved_config = [[HJPlayerContextConfig alloc] init];
        }

        const HJEntryContextPlayerInfo context_info = HJMakeContextInfo(resolved_config);
        _start_result = HJEntryContext::init(HJEntryType_Player, context_info);
        _has_attempted_start = YES;
        _started = (_start_result >= 0);
        return _start_result;
    }
}

- (void)stop {
    @synchronized(self) {
        HJEntryContext::unInit();
        _started = NO;
        _has_attempted_start = NO;
        _start_result = HJ_OK;
    }
}

@end
