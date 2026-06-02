# HJ Faceu Render 导出接口说明

本目录提供 ios 导出的运行时依赖及接口头文件，核心 API 位于 `core/HJRender.framework 含头文件及动态库等；
build_ios/HJFaceuExport.xcodeproj demo运行，也可以执行build_ios.sh生成工程；

## 回调与基础类型

- `typedef void(*HJFaceuCallback)(const char*i_uniqueKey, int i_type);`  
  第一个参数：每次添加faceu对应一个唯一的标识符ID，由上层设置，当此faceu每次渲染完成一次即回调，透传此标识符以便上层映射；第二个参数：事件/枚举值。
- `HJPoint2D`：二维点；`HJFacePoints9`：固定 9 点面部关键点。（以人像左上角为原点，依次为，左瞳孔、右瞳孔、鼻尖、左嘴角、右嘴角、 人脸Rect左上，人脸Rect右上，人脸Rect左下，人脸Rect右下）
- `HJRenderFaceuNotifyType`：`HJ_FACEU_NOTIFY_UNKNOWN = -1`，`HJ_FACEU_NOTIFY_COMPLETE = 1`，表示Faceu每循环完毕一次回调。
- `HJEntryContextInfo`：初始化日志/统计等上下文  
  - `logIsValid`：是否启用日志  
  - `logDir`：日志目录  
  - `logLevel`：日志级别（默认 `HJLOGLevel_INFO`）  
  - `logMode`：日志输出模式位标记（控制 console / file  
  - `logMaxFileSize` / `logMaxFileNum`：日志轮转大小与数量，详见demo所示

## API 列表

### 渲染上下文控制

| 函数 | 说明 | 备注 |
| --- | --- | --- |
| `int renderContextInit(const HJEntryContextInfo& ctx);` | 初始化渲染全局上下文（日志等），成功返回 0。 | 全局仅调用一次 |
| `void renderContextUnInit();` | 释放渲染上下文。 | 退出前调用 |

### Faceu 渲染

关于faceu操作的所有函数都应在同一个渲染线程内调用

| 函数 | 说明 | 备注 |
| --- | --- | --- |
| `void* faceuInit(HJFaceuCallback cb, bool useEnv = false);` | 创建 Faceu 句柄并注册回调；`useEnv=true` 时使用EGL环境模式。返回句柄，失败返回 `nullptr`。 | faceuInit, faceuAdd, faceuRemove,faceuRemoveAll, faceuRender,faceuDone均涉及到opengl操作，应在一个线程中调用； |
| `int faceuAdd(void* handle, const char* uniqueKey, const char* faceuUrl, bool debugPoints);` | 为句柄添加特效实例。`uniqueKey` 唯一，`faceuUrl` 资源路径，`debugPoints` 控制调试点渲染。成功 0。 | 回调在 `faceuInit` 处统一设置 |
| `int faceuRemove(void* handle, const char* uniqueKey);` | 删除指定特效实例。 | 同上faceuInit描述 |
| `int faceuRemoveAll(void* handle);` | 删除全部特效实例。 | 同上faceuInit描述 |
| `int faceuRender(void* handle, int width, int height, const HJPoint2D points[9], int pointCnt, bool containFace, unsigned char*& outRGBA);` | 传入宽高和 9 点人脸坐标，渲染输出 RGBA 数据到 `outRGBA`。成功返回 0。 | 同上faceuInit描述 |
| `void faceuDone(void* handle);` | 释放句柄。 | 同上faceuInit描述 |

## 最小示例

```cpp
#include "HJRenderContextExport.h"
#include "HJRenderFaceuExport.h"

HJEntryContextInfo ctx{};
ctx.logDir = "logs";
renderContextInit(ctx);

void onFaceuNotify(const char* i_uniqueKey, int type) {
    // 回调针对i_uniqueKey
}

void* h = faceuInit(onFaceuNotify, false);
faceuAdd(h, "faceu_0", "resource/faceu/XianWeng", false);

HJPoint2D pts[9] = {}; // 填入人脸关键点
unsigned char* rgba = nullptr;
faceuRender(h, 720, 1280, pts, 9, true, rgba);

faceuRemoveAll(h);
faceuDone(h);
renderContextUnInit();
```
