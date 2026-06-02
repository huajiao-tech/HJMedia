# HJSyncObject

## 用途
- 作为具备生命周期与线程同步控制的基类，统一提供 `init`/`done` 的加锁入口。
- 封装一次性初始化/释放的模板，派生类只需实现内部的资源获取与回收逻辑。

## 主要接口
| 接口 | 参数 | 返回 | 线程限制/说明 |
| --- | --- | --- | --- |
| `init(HJKeyStorage::Ptr param = nullptr)` | 可选配置参数 | `int`：`HJ_OK` 成功；`HJ_INITED` 已初始化；`HJErrAlreadyDone` 已结束；其他负值为错误 | 生产者锁（独占）；成功后调用 `afterInit`；负值会自动调用 `internalRelease` 回滚并重置状态。 |
| `done()` | - | `int`：`HJ_OK`；`HJErrAlreadyDone` 重复调用 | 生产者锁（独占）；将状态置 `HJSTATUS_Done` 后，在锁外调用 `internalRelease`，要求可重入、线程安全。派生析构应在基类析构前调用。 |
| `internalInit(HJKeyStorage::Ptr param)` | 由派生实现，默认返回 `HJ_OK` | 同上 | 在 `init` 的生产者锁内调用；派生可返回 `HJ_INITED` 表示已初始化，负值表示失败并触发回滚；需要时调用基类实现。 |
| `internalRelease()` | 由派生实现，默认空 | `void` | 可能在锁内或锁外被调用；必须可重复调用且线程安全；派生实现需调用基类版本以确保链式释放。 |
| `beforeDone()` | 可选重写，默认 `HJ_OK` | `int` | 在加锁前调用，非线程安全且可能被重复调用，可用于提前阻塞或校验。 |
| `afterInit()` | 可选重写，默认空 | `void` | 在 `init` 成功后、持有 `m_sync` 锁内调用，可安全触发受保护的回调。 |

## 生命周期与线程安全
- 初始状态为 `HJSTATUS_NONE`。`init` 成功后置为 `HJSTATUS_Inited`；`done` 将状态置为 `HJSTATUS_Done`。负值初始化失败时基类会调用 `internalRelease` 并重置状态。
- `init`/`done` 均使用 `SYNC_PROD_LOCK`（独占）保护状态与派生的内部初始化逻辑。
- `internalRelease` 可能在锁外执行（如 `done` 之后），派生必须自行保证并发安全与可重入；推荐在实现开头检查/保护成员指针是否已清理。
- 析构阶段基类仅调用 `internalRelease`。约定派生析构或外部使用方需在析构前调用派生的 `done()`，以确保派生层释放逻辑被执行。

## 常见用法
### 自定义对象初始化/清理
```cpp
class MyObj : public HJSyncObject {
protected:
    int internalInit(HJKeyStorage::Ptr param) override {
        auto ret = HJSyncObject::internalInit(param);
        if (ret != HJ_OK) return ret;
        // 分配资源...
        return HJ_OK;
    }

    void internalRelease() override {
        // 可重入的清理
        // if (ptr) { ...; ptr = nullptr; }
        HJSyncObject::internalRelease();
    }
};

// 使用
auto obj = std::make_shared<MyObj>();
obj->init();
// ... 使用 ...
obj->done();          // 建议显式调用
```

### 派生析构中确保释放
```cpp
MyObj::~MyObj() {
    done();            // 派生析构前调用，触发派生 internalRelease
}
```
