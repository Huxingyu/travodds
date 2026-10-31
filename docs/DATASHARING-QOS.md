# DataSharing 配置与发现能力（当前实现）

本文按本轮共享布局 v6、发现能力 v4 实现更新；Reader 普通池和 native 无初始化改动已通过当前验收，结果见 [10/31 报告](DATASHARING-PROGRESS-20261031.md)，官方交付与历史边界见 [验收矩阵](DATASHARING-ACCEPTANCE.md)。

C++ QoS、XML、Writer 共享池分配、SEDP 能力、Reader 实际映射注册与每对端点的传输选择已接入。有键与无键固定布局类型可以自动走共享路径；同一个 Writer 可以同时发送给共享 Reader 和 RTPS Reader。`ON` 要求本地准备成功；本轮对端不兼容、未注册或 Reader 映射失败仍保留 RTPS，映射异常会记录日志。ON 的契约为本地准备严格、对端不可用回退；兼容对端的暂时失败自动重试。

## C++ 配置

```cpp
DataWriterQos writer_qos;
writer_qos.data_sharing.kind = ON_DATA_SHARING_QOS;
writer_qos.data_sharing.shared_memory_directory = "/dev/shm";
writer_qos.data_sharing.domain_ids = {0, 42};
writer_qos.data_sharing.max_samples = 64;
writer_qos.data_sharing.max_readers = 16;
writer_qos.data_sharing.max_segment_bytes = 16 * 1024 * 1024;

DataReaderQos reader_qos;
reader_qos.data_sharing.domain_ids = {42};
```

| 字段 | 默认值 | 校验/含义 |
|---|---|---|
| kind | AUTO | AUTO 尝试本地准备，失败保留 RTPS；ON 准备失败使 enable 失败；OFF 禁用原生池与能力广告 |
| shared_memory_directory | /dev/shm | 绝对路径，非空，无 NUL，不超过 4096 字节；目录必须已存在，Writer 实际打开并验证资源 |
| domain_ids | {0} | 1～32 个不同 uint32；这是应用共享组，与 DDS domain ID 独立。协商候选需要交集 |
| max_samples | 64 | 1～1048576；Writer 槽位数，未归还的应用借用占用容量 |
| max_readers | 16 | 1～64；每个 Writer 段的 Reader 注册上限 |
| max_segment_bytes | 1 GiB | 非零；整个 Writer 段的预算，含元数据与对齐后的样本区。实际不足在准备时拒绝 |

Reader 不分配 Writer 段；Reader 的目录用于推导要打开的文件，预算在 mmap 前限制段总大小。不能仅凭目录字符串相同认定为同一共享内存。Reader 的 max_samples/max_readers 当前仅作配置保留，不覆盖远端 Writer 的分配参数。

全部字段在现有 endpoint `set_qos` 规则下不可变；值不同返回 IMMUTABLE_POLICY，保留旧配置。非法配置返回 INCONSISTENT_POLICY。默认 Writer/Reader QoS、创建、get_qos 和 XML profile 继承保留这些字段。

Linux 固定布局类型的 Writer 在 enable 时创建段，参数传入真实 SharedPayloadPool。ON 类型/平台不支持、缺失无初始化能力或带键 NativeKeyInfo 非法时返回 UNSUPPORTED；目录/系统调用失败返回 ERROR，超出段预算返回 INCONSISTENT_POLICY，分配失败返回 OUT_OF_RESOURCES。创建时自动 enable 失败表现为 create_datawriter 返回空指针；需要精确错误码时关闭 Publisher 的自动 enable，再显式调用 Writer::enable。借出不保证零初值，发布前由应用填写全部将被读取的字段；当前门控的正式验证结果见 [10/31 报告](DATASHARING-PROGRESS-20261031.md)。

AUTO 准备失败会记录日志并保持普通写入；此端点后续 loan_sample 返回 UNSUPPORTED，避免在 enable 后偷偷改写已发布的会话信息。OFF 的普通写入仍可用。Writer 与 Reader 均通过 supports_native_data_sharing 校验类型；带键类型的 NativeKeyInfo 必须有效，缺失/非法时 ON enable 返回 UNSUPPORTED，AUTO 正常 enable 但不广告 DS、普通 write 保留 CDR/RTPS，loan_sample 返回 UNSUPPORTED。Reader 的 ON 检查同一类型契约；内容过滤 Reader 当前不支持共享，ON enable 返回 UNSUPPORTED，AUTO 不广告共享能力。实际映射/注册失败保留 RTPS。已匹配兼容端点会以25ms至1s有界退避自动重试；实际注册完成且历史重放 ready 后才切换 DS。OFF、类型/host/domain不兼容不重试；CORRUPT/关闭/布局错误会话等待新会话或端点重建。

## XML

```xml
<dds_qos>
  <qos_library name="sharing">
    <qos_profile name="local">
      <datawriter_qos name="writer">
        <data_sharing>
          <kind>ON</kind>
          <shared_memory_directory>/dev/shm</shared_memory_directory>
          <domain_ids><id>42</id></domain_ids>
          <max_samples>64</max_samples>
          <max_readers>16</max_readers>
          <max_segment_bytes>16777216</max_segment_bytes>
        </data_sharing>
      </datawriter_qos>
      <datawriter_qos name="fallback" base_name="writer">
        <data_sharing><kind>AUTO</kind></data_sharing>
      </datawriter_qos>
      <datareader_qos name="reader">
        <data_sharing><domain_ids><id>42</id></domain_ids></data_sharing>
      </datareader_qos>
    </qos_profile>
  </qos_library>
</dds_qos>
```

Writer/Reader 支持相同节点。省略字段沿用默认值或基 profile；显式 domain_ids 替换原列表。kind 只接受 AUTO/ON/OFF；数值必须完整十进制且不溢出。未知节点、重复字段/重复 data_sharing、未知属性、非法嵌套、空或重复域、负数和尾随字符均拒绝。单条策略解析采用临时副本，校验完成后才替换。整个 XML 仓库 load 的事务性不在本次变更范围内。

## 发现扩展与回退边界

SEDP 使用 TravoDDS 自定义 PID `0x80e1`。本轮有能力的 Writer/Reader 使用发现版本 4，携带原生布局 ID/大小/对齐、共享组、Linux boot ID 与 PID namespace 身份。Writer 另外发送随机会话、段字节数与槽位/Reader 数；不发送或接受任意远端文件路径。

能力正文采用固定大端格式，与外层 ParameterList 的字节序无关；整体最多 1024 字节，布局 ID 最多 256 字节，共享组最多 32 个。解码按边界验证后分配，拒绝重复能力、未知版本、截断、错误角色和非零尾部填充。缺少扩展、发现能力 v1/v2/v3 或未知版本时不能启用 native，保留 RTPS。v4 同时表示无初始化能力及强制带键 key-only 准入，隔离仍可能清零完整载荷的旧 v3 对端。

`compatible_with` 要求 Writer/Reader 角色、版本、类型布局、同主机/同 PID namespace 和共享组交集全部成立。结果仅是候选；后续实际 open 还必须通过共享池已有的文件属主/权限、会话、GUID、长度、pthread ABI、进程身份等校验，并完成 Reader 注册和重放 ready 握手，才可切换传输。只有 ready 的兼容 Reader 才会被选为该样本的 DDS 共享目标。能力广告本身不代表已打开共享段，也不能当作安全认证。启用 DDS Security 时不准备原生共享路径。

TypeSupport/DataWriter 虚表、QoS、BuiltinTopicData 和序列布局均有变动；库和应用必须一起重新编译。

## 当前调度与容量行为

- 每次写入从已注册且 ready 的 Reader 中选择匹配的共享目标。没有共享目标时使用原有 CDR 路径；TRANSIENT_LOCAL Writer 也可先保留原生历史供晚加入 Reader 重放。
- 有共享目标时，Loan 写入直接使用该槽位；普通裸指针写入分配槽位并复制固定布局字节。带键类型使用相同的共享路径；存在 SQL Reader 过滤器时继续使用 CDR。
- 混合匹配只生成一份 CDR，并按 Reader entity ID 定向发送，避免误投给同一参与者内的共享 Reader；DATA、DATA_FRAG 和重传均保留目标身份。
- 原生预分配耗尽：loan_sample 返回 OUT_OF_RESOURCES；普通 ON 写入返回分配错误，AUTO 普通写入可回退 CDR。共享 listener 停止注册后，后续投递可使用 RTPS。
- Reader 关闭会停止新投递、丢弃未读通知并等待监听线程退出；已经交给应用的 Loan 继续保活映射。
- 双方均为 TRANSIENT_LOCAL 时允许注册时重放 Writer 仍持有的共享槽位。Writer 无 Reader 预发布、可靠/尽力独立进程晚加入、混合晚加入及 VOLATILE 排除旧数据已有测试；BEST_EFFORT 不提供丢包重发保证。


普通序列化载荷池由本地 payload_pool 配置；本轮新增 DataReaderQos/XML 接入，Writer 既有配置保留。配置兼容的同 domain/topic/type 端点复用池，Reader 在 history 准入边界转入普通 CDR 分片，native Loan 旁路；19 项 Reader 回归已在普通与两个检查版中通过。四种策略、容量、对齐及 XML 写法见 [普通池配置手册](PAYLOADPOOL-QOS.md)。data_sharing.max_samples 约束共享槽位，payload_pool.max_payloads 约束普通 history CDR 分片块，transport 整包与独立 CDR key snapshot 不计后者预算。

带键注册与 Reader 键查询额外保留独立本地键快照；native 仅存声明的键字节，不存在全样本快照兼容分支。Writer 与 Reader 两端必须各自满足 key-only 契约才能广告 DS；具体键表示仍是本地元数据，捕获/还原失败不会回退完整样本复制。CDR 类型继续使用其原有序列化快照。键和控制元数据仍有复制，不承诺任何字节都不复制。详见 [API 手册](DATASHARING-API.md#带键实例与本地快照)。它不占共享槽位，也不计入段预算；配置大量实例时需要另外预算内存。

## Reader 有限生命周期与时间

`reader_data_lifecycle.autopurge_nowriter_samples_delay` 与 `autopurge_disposed_samples_delay` 默认无限。有限值必须非负且 nanosec<1e9，零表示下一次维护时可回收；C++与XML继承相同。状态转移以单调时钟计时，10ms维护周期加系统调度决定观察延迟。disposed与最后Writer离开的条件分开；仍有活Writer时不执行no-writer回收，新的ALIVE取消旧状态的回收。到期释放样本、键快照、索引和max_instances预算。已归还或仍借用的样本引用与实例身份分离：旧Loan保活，旧句柄键查询返回BAD_PARAMETER。

```xml
<reader_data_lifecycle>
  <autopurge_nowriter_samples_delay><sec>0</sec><nanosec>30000000</nanosec></autopurge_nowriter_samples_delay>
  <autopurge_disposed_samples_delay><sec>1</sec><nanosec>0</nanosec></autopurge_disposed_samples_delay>
</reader_data_lifecycle>
```

lifespan使用Writer源时间，进入history前检查、读取前及维护期间再次检查；合法INFO_TS在CDR/native间保留相同语义，缺失/失效时使用接收时间作为内部寿命基准。deadline用单调时钟；修改周期仅更新已写入且ALIVE的活动实例，有限/无限切换不会启动从未写入或已停止的实例。端点调度和Timer运行状态分别同步；deadline应用listener在释放调度锁和状态锁后调用，可查询状态、修改QoS及注销。

KEEP_LAST普通write在槽位耗尽且同实例history已满时先释放待替换history引用，再尝试预留。应用Writer Loan、未读Reader及Reader Loan独立固定槽位；仍被固定时ON返回OUT_OF_RESOURCES，AUTO普通write回退CDR。KEEP_ALL不会为了新写入提前淘汰保留历史，容量需包含在途与应用借用预算。不能借此增加段槽位或覆盖未归还Loan。不同键共用总槽位预算，不会为新键淘汰其他实例历史。


2026-10-31 共享布局 v6 / discovery v4 的 Reader 普通 Topic 池与 native 无初始化契约已补齐。当前 321 项冻结输入对应 529 项普通回归、9 次有限示例、93.936% 新增生产可执行行覆盖率，ASan/UBSan 与 TSan focused 各 86 项通过；最终审计及主包/历史附包搬迁复核通过，原始范围、哈希和复现命令见 [10/31 报告](DATASHARING-PROGRESS-20261031.md)。[10/30 报告](DATASHARING-STRICT-20261030.md) 的 500 项、93.920%、9 次示例及两版 focused 各 57 项仅证明当日 discovery v3 冻结源码。LSan 未启用，历史 WaitSet 崩溃根因仍未知，当前通过不作根因归属证明。
