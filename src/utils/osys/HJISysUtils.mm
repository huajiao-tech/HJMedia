//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJISysUtils.h"
#include "HJUtilitys.h"
#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>
//#import <UIKit/UIKit.h>
#include <sys/sysctl.h>
#import <mach/mach.h>
#import <mach/mach_host.h>
#if defined(HJ_OS_IOS)
#   import "GPUUtilization/GPUUtilization.h"
#endif

NS_HJ_BEGIN
//***********************************************************************************//
std::string HJISysUtils::getDocumentsPath()
{
    NSString* documentPath = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) firstObject];
    return [documentPath UTF8String];
}

std::string HJISysUtils::getLibraryPath()
{
    NSString* libraryPath = [NSSearchPathForDirectoriesInDomains(NSLibraryDirectory, NSUserDomainMask, YES) firstObject];
    return [libraryPath UTF8String];
}

std::string HJISysUtils::getMainBundlePath()
{
    return [[[NSBundle mainBundle] bundlePath] UTF8String];
}

int HJISysUtils::createDirectory(const std::string& dir)
{
    if(dir.empty()) {
        return HJErrFatal;
    }
    BOOL isOK = YES;
    NSString* path = [NSString stringWithUTF8String:dir.c_str()];
    NSFileManager *fileManager = [NSFileManager defaultManager];
    if (![fileManager fileExistsAtPath:path]) {
        isOK = [fileManager createDirectoryAtPath:path withIntermediateDirectories:YES attributes:nil error:nil];
    }
    return (YES != isOK);
}

std::string HJISysUtils::logDir()
{
    return HJISysUtils::getDocumentsPath() + "/Logs";
}

std::string HJISysUtils::defaultCacheDirectory()
{
#if TARGET_OS_IPHONE
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSCachesDirectory, NSUserDomainMask, YES);
    NSString *cacheDir = paths.firstObject;
#else
    NSString *appName = [[NSProcessInfo processInfo] processName];
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSLibraryDirectory, NSUserDomainMask, YES);
    NSString *basePath = ([paths count] > 0) ? paths[0] : NSTemporaryDirectory();
    NSString *cacheDir = [cacheDir stringByAppendingPathComponent:appName];
#endif
    return [cacheDir UTF8String];
}

std::string HJISysUtils::defaultLogsDirectory()
{
#if TARGET_OS_IPHONE
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSCachesDirectory, NSUserDomainMask, YES);
    NSString *baseDir = paths.firstObject;
    NSString *logsDirectory = [baseDir stringByAppendingPathComponent:@"Logs"];
#else
    NSString *appName = [[NSProcessInfo processInfo] processName];
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSLibraryDirectory, NSUserDomainMask, YES);
    NSString *basePath = ([paths count] > 0) ? paths[0] : NSTemporaryDirectory();
    NSString *logsDirectory = [[basePath stringByAppendingPathComponent:@"Logs"] stringByAppendingPathComponent:appName];
#endif
    return [logsDirectory UTF8String];
}

std::string HJISysUtils::devicePlatformString()
{
    size_t size;
    sysctlbyname("hw.machine", NULL, &size, NULL, 0);
    std::string device(size + 1, '\0');
    sysctlbyname("hw.machine", device.data(), &size, NULL, 0);
    HJUtilitys::removeTrailingNulls(device);
    return device;
}

std::tuple<float, float> HJISysUtils::getMemoryUsage()
{
    float memoryUsageInByte = 0;
    float memoryInRemaining = 0;
    task_vm_info_data_t vmInfo;
    mach_msg_type_number_t count = TASK_VM_INFO_COUNT;
    kern_return_t kernelReturn = task_info(mach_task_self(), TASK_VM_INFO, (task_info_t) &vmInfo, &count);
    if(kernelReturn == KERN_SUCCESS) {
        memoryUsageInByte = (int64_t) vmInfo.phys_footprint / (float)(1024 * 1024);
        memoryInRemaining = (int64_t)vmInfo.limit_bytes_remaining / (float)(1024 * 1024);
//        NSLog(@"Memory in use (in bytes): %lld", memoryUsageInByte);
    } else {
        NSLog(@"Error with task_info(): %s", mach_error_string(kernelReturn));
    }
    return std::make_tuple(memoryUsageInByte, memoryInRemaining);
}

float HJISysUtils::getCpuUsage()
{
    NSProcessInfo *processInfo = [NSProcessInfo processInfo];
    NSTimeInterval systemUptime = [processInfo systemUptime];
    
    host_cpu_load_info_data_t cpuinfo;
    mach_msg_type_number_t count = HOST_CPU_LOAD_INFO_COUNT;
    
    if (host_statistics(mach_host_self(), HOST_CPU_LOAD_INFO, (host_info_t)&cpuinfo, &count) == KERN_SUCCESS) {
        natural_t user = cpuinfo.cpu_ticks[CPU_STATE_USER];
        natural_t nice = cpuinfo.cpu_ticks[CPU_STATE_NICE];
        natural_t sys = cpuinfo.cpu_ticks[CPU_STATE_SYSTEM];
        natural_t idle = cpuinfo.cpu_ticks[CPU_STATE_IDLE];
        natural_t total = user + nice + sys + idle;
        
        return (user + nice + sys) / (float)total;
    } else {
        return 0.0;  // Unable to get CPU usage
    }
    return 0.0;
}

float HJISysUtils::getCpuUsageX()
{
    kern_return_t           kr;
    thread_array_t          threadList;         // 保存当前Mach task的线程列表
    mach_msg_type_number_t  threadCount;        // 保存当前Mach task的线程个数
    thread_info_data_t      threadInfo;         // 保存单个线程的信息列表
    mach_msg_type_number_t  threadInfoCount;    // 保存当前线程的信息列表大小
    thread_basic_info_t     threadBasicInfo;    // 线程的基本信息
    
    // 通过“task_threads”API调用获取指定 task 的线程列表
    //  mach_task_self_，表示获取当前的 Mach task
    kr = task_threads(mach_task_self(), &threadList, &threadCount);
    if (kr != KERN_SUCCESS) {
        return -1;
    }
    double cpuUsage = 0;
    for (int i = 0; i < threadCount; i++) {
        threadInfoCount = THREAD_INFO_MAX;
        // 通过“thread_info”API调用来查询指定线程的信息
        //  flavor参数传的是THREAD_BASIC_INFO，使用这个类型会返回线程的基本信息，
        //  定义在 thread_basic_info_t 结构体，包含了用户和系统的运行时间、运行状态和调度优先级等
        kr = thread_info(threadList[i], THREAD_BASIC_INFO, (thread_info_t)threadInfo, &threadInfoCount);
        if (kr != KERN_SUCCESS) {
            return -1;
        }
        
        threadBasicInfo = (thread_basic_info_t)threadInfo;
        if (!(threadBasicInfo->flags & TH_FLAGS_IDLE)) {
            cpuUsage += threadBasicInfo->cpu_usage;
        }
    }
    
    // 回收内存，防止内存泄漏
    vm_deallocate(mach_task_self(), (vm_offset_t)threadList, threadCount * sizeof(thread_t));

    return cpuUsage / (double)TH_USAGE_SCALE * 100.0;
}

float HJISysUtils::getGpuUsage()
{
#if defined(HJ_OS_IOS)
    return [GPUUtilization gpuUsage];
//    [GPUUtilization fetchCurrentUtilization:^(GPUUtilization *current) {
//      NSLog(@"202412181932 gpu usage %ld", static_cast<long>(current.deviceUtilization));
//    }];
#else
    return 0.0f;
#endif
}

NS_HJ_END
