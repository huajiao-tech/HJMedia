Faceu渲染模块 Java API 摘要
================================
包含

1、HJMediasdk.jar为Jar包，libs文件夹下

2、libHJRender.so  32位so文件，64位so文件  jniLibs文件夹下

详细调用见 FaceuActivity.java

## HJRenderContextJni
- `contextInit(String logDir, int logLevel, int logMode, int max_size, int max_files)`: 全局初始化，内容上下文并配置日志目录、级别、模式与滚动大小/文件数；APP加载时仅调用一次。
- `contextUnInit()`: 内容上下文反初始化; APP卸载时仅调用一次。


## HJRenderFacePoints
- `addPoint(float x, float y)`: 添加人脸关键点（固定 9 点面部关键点。（以人像左上角为原点，依次为，左瞳孔、右瞳孔、鼻尖、左嘴角、右嘴角、 人脸Rect左上，人脸Rect右上，人脸Rect左下，人脸Rect右下））；
- `clear()`: 清空点集，并将“检测到人脸”标记重置为 `false`。
- `isContainFace()` / `setContainFace(boolean)`: 是否检测到人脸的标记。
- `FacePoint`: 内部结构体，包含单个点的 `x`/`y` 坐标。

## HJRenderFaceuEntryJni
以下函数因涉及到GL上下文均在渲染线程内执行；《同一个渲染线程》

| 名称 | 返回值 | 说明 | 备注 |
| --- | --- | --- | --- |
| `faceuInit(HJBaseListener listener)` | `int` | 保存监听器（弱引用）并初始化 FaceU native 资源 | 使用前必调；`listener` 接收回调 |
| `faceuAdd(String uniqueKey, String faceuUrl, boolean debugPoints)` | `int` | 通过唯一键，每一个faceu资源使用不同的uniqueKey,Faceu资源路径传入底层即可 | `debugPoints` 可输出调试点位 |
| `faceuRemove(String uniqueKey)` | `int` | 按唯一键移除指定特效 | - |
| `faceuRemoveAll()` | `int` | 移除全部已加载特效 | 清空后需重新 `faceuAdd` |
| `faceuRender(HJRenderFacePoints points, int width, int height)` | `int` | 将当前点9个传入底层进行渲染 |
| `faceuDone()` | `void` | 释放 FaceU 相关资源 | 收尾调用 |
| `onNotify(int id, int value, String desc)` | `int` | 接收 native 回调并转发给上层监听器， HJ_FACEU_NOTIFY_COMPLETE表示对应的uniqueKey faceu已经播放一遍完毕，每播放完毕一次就回调一次 | 