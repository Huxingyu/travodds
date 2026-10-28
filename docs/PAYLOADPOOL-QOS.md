# 普通序列化载荷池配置

`DataWriterQos::payload_pool` 控制普通 CDR/RTPS 载荷内存。它与 `data_sharing` 的原生样本槽位预算独立。配置不参与端点发现，也不要求远端 Reader 使用相同设置。

```cpp
DataWriterQos qos;
qos.payload_pool.policy = PayloadMemoryPolicy::DYNAMIC_REUSABLE;
qos.payload_pool.payload_size = 4096;
qos.payload_pool.max_payloads = 8;
qos.payload_pool.initial_payloads = 2;
qos.payload_pool.alignment = 64;
auto* writer = publisher->create_datawriter(topic, qos, nullptr, STATUS_MASK_NONE);
// writer == nullptr 表示非法配置、内存分配失败或活动 Topic 池配置冲突。
```

| 策略 | 创建时 | 归还后 | 大于 payload_size 的请求 |
|---|---|---|---|
| PREALLOCATED | 分配 max_payloads 个块 | 保留复用 | 拒绝 |
| PREALLOCATED_WITH_REALLOC | 分配 max_payloads 个块 | 保留复用 | 只扩容空闲块，保留活动块地址 |
| DYNAMIC_RESERVE | 按需分配，initial_payloads 必须为 0 | 释放块字节 | 按需分配 |
| DYNAMIC_REUSABLE | 分配 initial_payloads 个块，其余按需 | 保留复用 | 只扩容空闲块 |

`max_payloads` 限制同一个 domain/topic/type 池中的分片块数，所有关联 Writer 合计使用这个上限；它不是 DDS 样本数。一个大样本可占多个分片。History 的 depth/resource_limits 仍独立管理样本缓存。

`payload_size` 是实际分配块的字节数，包含 RTPS 前后预留区。当前每块预留 `2 * BUFF_SHIFT = 2048` 字节，首片还需要 4 字节 CDR 封装。例如不分片的 64 字节序列化样本至少需要 2116 字节的块；上面的 4096 足够。PREALLOCATED 使用者应按最大序列化分片尺寸预留，不能直接填原生结构体的 sizeof。

默认保持兼容行为：DYNAMIC_REUSABLE、payload_size=0、max_payloads=UINT32_MAX、initial_payloads=0、自然最大对齐。默认不预分配 UINT32_MAX 个块。需要可预测内存时应显式设置容量；预分配策略会在创建时实际分配全部块，创建前应预算 `payload_size * max_payloads` 及元数据。

`max_payloads` 必须大于 0，initial_payloads 不超过它；alignment 必须为不小于指针自然对齐的 2 的幂。预分配策略要求 payload_size 非零。策略及全部分配参数在 Writer 创建后不可更改；先删除相关 Writer、释放保活引用，再以新配置重建。

同一进程内 domain/topic/type 相同的 Writer 共享一个弱注册池，五个参数必须完全一致。不同 Topic 或类型分别建池。缓存与应用持有的缓冲引用会保活原池；删除 Writer 不会使这些地址失效。池耗尽通过现有写入路径返回 RETCODE_APPLY_MEM_ERR，未发布的分片链自动回收。

XML 写法：

```xml
<datawriter_qos name="bounded_pool">
  <payload_pool>
    <policy>DYNAMIC_REUSABLE</policy>
    <payload_size>4096</payload_size>
    <max_payloads>8</max_payloads>
    <initial_payloads>2</initial_payloads>
    <alignment>64</alignment>
  </payload_pool>
</datawriter_qos>
```

支持 profile 继承与部分字段覆盖。未知字段、重复字段/段、属性、非法嵌套、负数、数值溢出和不一致配置会使 XML 加载失败。普通池参数是本地 Writer 配置；Reader 的原生共享预算继续由 `data_sharing` 管理。

本阶段验证状态见 `DATASHARING-PLAN.md`；普通池回归不能代替 DataSharing 端到端和性能验收。
