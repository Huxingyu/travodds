# 普通序列化载荷池配置

本轮增加 DataReaderQos::payload_pool，与既有 DataWriterQos::payload_pool 一起控制本地普通 CDR/RTPS history 载荷。19 项 Reader 回归已在正式模块 245 项及独立 ASan/UBSan、TSan 中通过，包含真实接收、分片、配置和寿命验证；本轮证据见 [10/31 报告](DATASHARING-PROGRESS-20261031.md)。普通池与 data_sharing 共享槽位预算独立，不参与端点发现，也不要求远端端点采用同样配置。

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

max_payloads 限制同一 domain/topic/type 池的分片块数，配置兼容的本地 Writer 与 Reader 合计使用这个上限；它不是 DDS 样本数。一个大样本可占多个分片，Reader 转入 history 的链也逐片计数。History 的 depth/resource_limits 仍独立管理样本缓存。transport 接收整包、独立 CDR key snapshot 和 native 共享槽位不混入普通 history 池预算。

payload_size 是实际分配块的字节数。Writer 创建发送链时每块另需 2 * BUFF_SHIFT = 2048 字节 RTPS 预留区，首片还有 4 字节 CDR 封装，例如不分片的 64 字节序列化样本至少需要 2116 字节的块。Reader 准入克隆只按接收 CDR 链的各片 buffer_size 分配，不额外补 Writer 预留区；Reader/Writer 共池时须按两侧请求的最大值预算。PREALLOCATED 不能直接使用原生结构体 sizeof 估计块尺寸。

默认保持兼容行为：DYNAMIC_REUSABLE、payload_size=0、max_payloads=UINT32_MAX、initial_payloads=0、自然最大对齐。默认不预分配 UINT32_MAX 个块。需要可预测内存时应显式设置容量；预分配策略会在创建时实际分配全部块，创建前应预算 `payload_size * max_payloads` 及元数据。

max_payloads 必须大于 0，initial_payloads 不超过它；alignment 必须为不小于指针自然对齐的 2 的幂，预分配策略要求 payload_size 非零。policy、payload_size、max_payloads、initial_payloads、alignment 五个字段在 Writer/Reader 创建后不可更改；先删除相关端点、释放保活引用，再以新配置重建。

同一进程内 domain/topic/type 相同的 Writer/Reader 共享一个弱注册池，五个参数必须完全一致；不同 Topic 或类型分别建池。ContentFilteredTopic 使用关联 wire Topic 的池身份；MultiTopic 没有 wire Topic 名时使用自身结果 Topic 名与类型，不因空发现字段拒绝创建。这项兼容性修复及引用/缓存寿命回归已在本轮正式模块和两个检查版通过。非法配置或活动池冲突返回 INCONSISTENT_POLICY，自动 enable 创建端点时表现为空指针。缓存持有原池强引用，删除端点不会使尚有缓存引用的载荷失效。Writer 池耗尽沿用 RETCODE_APPLY_MEM_ERR；Reader AddChanges 准入池满同样返回该码，不能为失败样本注册新实例或逐出旧 KEEP_LAST 样本。

Reader 在 key snapshot、过滤和 history 淘汰前事务性克隆整条 CDR 链：失败保留原链与原池占用，已分配的部分 Topic 块全部归还；已在相同池中的链重试不再分配。原始 transport 接收缓存仍由原池归还，内置发现接收保留既有路径。native SharedReadLoan 不做这次复制，继续固定共享映射。真实接收、分片重组、耗尽回滚和重复 read 后 take 均已有本轮正式回归。

KEEP_LAST 必须为新载荷预留暂存空间，成功准入后才淘汰旧 history；enable 拒绝 max_payloads <= history.depth 的明确不足配置。这只是每样本一片、单个 history 的必要条件：分片数、共池 Writer/Reader、外部缓存持有者和一条完整入库载荷都要计入预算。容量不足返回 APPLY_MEM_ERR 并保留旧 KEEP_LAST 样本，不通过提前淘汰掩盖不足。长度零的生命周期控制描述符不消耗普通载荷块，ALIVE 空描述符仍拒绝。

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

Reader 在 datareader_qos 下使用同名 payload_pool 节及相同字段。支持 profile 继承与部分字段覆盖；未知字段、重复字段/段、属性、非法嵌套、负数、数值溢出和不一致配置会使 XML 加载失败。配置是本地预算；Reader 的原生共享映射继续由 data_sharing 管理。

本轮验证状态见 [10/31 报告](DATASHARING-PROGRESS-20261031.md)；普通池回归不能代替 DataSharing 端到端和性能验收。
