#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSInteger, HJPlayerLogMode) {
    HJPlayerLogModeNone = 1 << 0,
    HJPlayerLogModeConsole = 1 << 1,
    HJPlayerLogModeFile = 1 << 2,
    HJPlayerLogModeCallback = 1 << 3,
};

typedef NS_ENUM(NSInteger, HJPlayerContextLogLevel) {
    HJPlayerContextLogLevelTrace = 0,
    HJPlayerContextLogLevelDebug = 1,
    HJPlayerContextLogLevelInfo = 2,
    HJPlayerContextLogLevelWarn = 3,
    HJPlayerContextLogLevelError = 4,
    HJPlayerContextLogLevelAlarm = 5,
    HJPlayerContextLogLevelFatal = 6,
};

@interface HJPlayerContextConfig : NSObject

@property (nonatomic, assign) BOOL logEnabled;
@property (nonatomic, copy) NSString *logDirectory;
@property (nonatomic, assign) NSInteger logLevel;
@property (nonatomic, assign) NSInteger logMode;
@property (nonatomic, assign) NSInteger logMaxFileSize;
@property (nonatomic, assign) NSInteger logMaxFileCount;
@property (nonatomic, copy) NSString *mediasDirectory;
@property (nonatomic, assign) NSInteger mediasCacheMax;
@property (nonatomic, assign) NSInteger downloadRetryMax;

@end

@interface HJPlayerContext : NSObject

+ (instancetype)sharedInstance;

- (NSInteger)startWithConfig:(nullable HJPlayerContextConfig *)config;
- (void)stop;

@property (nonatomic, assign, readonly, getter=isStarted) BOOL started;

@end

NS_ASSUME_NONNULL_END
