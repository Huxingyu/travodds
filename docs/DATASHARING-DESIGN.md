# DataSharing 与 Loan 设计

本文按本轮实现更新：共享布局 v6、发现能力 v4，Reader 普通池及 native 无初始化改动已通过当前验收。结果见 [10/31 报告](DATASHARING-PROGRESS-20261031.md)，官方交付对应关系见 [验收矩阵](DATASHARING-ACCEPTANCE.md)，数据面既有协议见 [历史生产报告](DATASHARING-PRODUCTION-20261030.md)。零完整载荷复制限定显式兼容的 Writer Loan → Reader Loan 分支；普通裸指针 write 和预分配输出仍复制，无初始化需要单独满足类型契约。

## 当前分配与归还路径

普通 DDS 写入先通过 `DataWriterImpl::CreateSerializedBuffer` 创建 CDR 缓冲链，再序列化并调用 `Writer::NewChange`。用户载荷使用按 `(domain_id, topic_name, type_name)` 注册的 `TopicPayloadPool`；同一键的 Writer 共享同一池。注册表持弱引用，Writer、临时缓冲链和历史缓存持强引用。

`PayloadBufferPtr` 在写入准备阶段独占整条链，包括进程内描述符及其载荷。失败、提前返回或序列化抛异常时，析构逐个归还载荷并删除描述符。`NewChange` 成功后才把链所有权转给 `CacheChange`，后者持有原始池直至最终释放。Writer 删除与某条缓存最终释放可以发生在不同时间，缓存不会依赖已析构的 Writer。

普通 CDR 注册快照及内置发现报文仍有独立的旧池路径；原生实例快照使用独立的对齐键存储，带键类型只有声明有效 NativeKeyInfo 并承诺 capture/restore 钩子契约才能启用 native；原生完整样本快照回退已移除，旧类型保留 CDR/RTPS。键哈希由 TypeSupport::MakeKey 计算。不能将这些指针交给 Topic 池回收，也不能将 Topic 池的指针交给全局 `MemoryPool`。Writer 的注销、dispose 控制缓存与正常用户缓存采用相同的显式原池参数。

Reader 的反序列化读取临时持有 `shared_ptr<CacheChange>`。`take` 从历史中移除可读取样本；它不直接释放复制出来的 `SerializedBuffer::origin_buffer`。当所有历史、临时读取及其他持有者均释放缓存时，才由缓存归还原池。RTPS 接收的 `CacheSerializedBuffer` 也可能只借用 `originMessage` 的空间而没有 `origin_buffer`；该路径依靠消息的强引用保活。

Reader 普通载荷池接入是本轮补全项：计划在 AddChanges/history 准入边界将完整 CDR 分片链事务性转入 Reader 配置的 Topic 池，失败归还已分配分片并拒绝准入；配置相同的同 domain/topic/type Reader/Writer 复用池。transport 接收整包与独立 CDR key snapshot 保留原池，不计 history Topic 预算；native SharedReadLoan 保留原映射，不复制到普通池。该路径尚待本轮真实 RTPS/分片与生命周期回归，不能用旧 Writer 普通池结果证明 Reader 已接入。

## 四种普通池策略

| 策略 | 创建时 | 超过原容量的请求 | 归还时 |
|---|---|---|---|
| PREALLOCATED | 分配 max_payloads 个固定大小槽位 | 拒绝 | 保留存储 |
| PREALLOCATED_WITH_REALLOC | 分配 max_payloads 个槽位 | 只扩容空闲槽位 | 保留存储 |
| DYNAMIC_RESERVE | 不预分配载荷 | 按需申请，受 max_payloads 限制 | 释放载荷，保留槽位元数据 |
| DYNAMIC_REUSABLE | 可预分配 initial_payloads 个槽位 | 按需申请或扩容空闲槽位 | 保留存储 |

所有策略检查对齐、容量及配置合法性，拒绝外部指针、内部偏移地址和未再次借出前的重复归还。仅凭地址不能识别地址复用后的旧别名，因此这不是 Loan 的代次验证接口。

动态策略的元数据随实际需求增长。每次借出新的槽位前预留足够的归还栈容量，保证归还不分配内存。默认普通 Writer 使用动态复用策略和 uint32 上限，沿用旧池的容量语义；不会因为普通池组件的默认 64 个槽位而额外限制一个 Topic 的所有 Writer。单个 Writer 的 history/resource_limits 继续由历史层约束。共享池容量已接入 DataSharing QoS，普通池四策略通过 payload_pool QoS/XML 选择，详见 PAYLOADPOOL-QOS.md。

## 已实现的共享池组件

共享池与普通进程内池采用不同的存储协议。共享段不能保存进程本地地址、标准容器、虚表或 shared_ptr。Reader 接收到段身份、槽位索引、样本代次后，用自己的映射基址计算访问地址。

`SharedPayloadPool` 使用 `travodds_ds_<Writer GUID>_<128 位随机会话>.shm` 文件，默认目录 `/dev/shm`，创建权限 0600。文件保存 magic、协议版本、布局长度、pthread ABI 大小、Writer GUID、会话、PID 命名空间、进程启动身份、样本大小/对齐、槽位数以及运行状态。创建使用 InterprocessType 中的 Boost 设备与初始化文件锁，并在 Boost mapped_region 映射前通过 `posix_fallocate` 预留实际存储，空间不足时抛出异常。映射前检查文件类型、属主、权限、长度、布局与会话；不对未验证的内容执行共享锁操作。

每个槽位包含独立的 64 位代次、序列号、有效长度、Writer 持有状态，以及每个 Reader 的未读与借用位图，最多 64 个 Reader。槽位从 FREE 到 WRITING，再到 PUBLISHED；只有 Writer 引用、未读投递和所有 Reader Loan 都释放后才能回到 FREE。样本代次、Reader 注册代次不会回绕；达到上限后停止复用相应槽位或注册项。序列号必须严格递增。

分配采用固定槽位池的轮转扫描：递增起点后按模槽位数扫描可复用项。它不是依靠 head/tail 保证 FIFO 的环形队列，起点轮转也不表示按顺序覆盖最旧样本；仍有 Writer/pending/Reader Loan pin 的槽位不能复用。

发布顺序为：取得可写槽位 → 开始对象寿命但不初始化载荷 → 应用填写所有将被读取的字段 → 发布槽位身份 → 通知 Reader。借出不保证零初值。Reader 必须在获取发布同步之后访问样本；通知只是唤醒提示，不能作为唯一的数据队列，也不能因为通知合并而漏读样本。

Reader 需要有独立注册代次，防止 Reader 退出重启后错误归还上一个会话的样本。多个 Reader 的引用互相独立。慢 Reader 持有 Loan 时，Writer 根据池容量和可靠性策略等待或报资源不足，不能覆盖尚在读取的数据。

跨进程注册和控制维护保留 robust、process-shared pthread mutex；槽位预留、发布、Reader 借用/归还和 ready 注册快照不获取它。槽位用原子 gate 组合 phase 与短操作计数，取得 lease 后再验证独立 64 位代次；预留只有在短操作退出且 Writer/pending/Loan pin 全空时才能覆写。Reader 先建立 Loan pin 再清 pending，长期借用只占 pin，不占短操作计数。控制锁持有者暂停时，已有数据面仍可推进；新的注册仍可能等待控制锁。

通知采用 always-lock-free 的 32 位原子代次和 Linux process-shared futex。等待者先采样通知代次，再检查数据谓词，无数据时按原单调时钟期限比较等待；并发通知通过代次不符避免漏唤醒。等待后重扫，不依赖通知次数保存队列。Writer 关闭后还需重扫最终发布水位再返回 CLOSED。futex 系统调用及 DDS 本地 Writer/history/发现锁不构成整个 write 路径的形式化 lock-free 保证。

Reader 进程身份由 PID 和 `/proc/<pid>/stat` 启动时间共同确定，仅在确认进程已死、成为僵尸或 PID 已复用时处理引用。无法读取的状态视为未知，不因超时回收。Writer 每 100 ms 的弱引用维护定时器及 Reader 空等待执行控制维护；统计使用 try-lock，忙返回 TIMEOUT。若死亡登记仍有无法安全归属的短操作计数，保守将 session 标为 CORRUPT，不强减计数；管理锁持有者死亡也使 session 损坏。新分配停止，旧 Loan 的映射和已发布字节继续保活，恢复需要新会话。当前要求相同 PID 命名空间且 `/proc` 编号与调用者一致。

Writer 正常关闭会标记 CLOSED、释放 Writer 引用并解绑当前文件；已映射 Reader 仍可读完已发布样本，Loan 在归还前继续保活。Writer 崩溃时 Reader 等待检测关闭；`removeOrphan` 仅在确认原 Writer 已死亡后处理对应会话和 inode。新 Writer 即使 GUID 相同也使用新会话。

### 内部 API 与集成边界

- `create(guid, config, directory)` 创建池；`open(descriptor)` 在另一进程建立独立映射。
- Writer 的 `loan(size, token, buffer, timeout)` 返回可写地址，`publish(token, size, sequence)` 发布，`releaseWriter(token)` 释放其引用。尚未发布的样本也通过 `releaseWriter` 取消。
- `registerReader()` 返回独立注册，默认只接收注册之后的发布。`waitNext(afterSequence, timeout, loan)` 按序获取未读样本；`returnLoan(loan)` 或 Loan 析构归还借用；`discard(token)` 丢弃尚未借出的投递。
- `SharedReadLoan` 只能移动，持有注册和映射。销毁 Reader 句柄不会提前释放仍被 Loan 使用的地址。错误 Reader、会话、槽位代次和重复归还会被拒绝。
- `loan` 无等待时资源不足返回 EXHAUSTED，有等待时到期返回 TIMEOUT；`waitNext` 无可读样本到期返回 TIMEOUT。同一本地 SharedPoolReader 的并发 take/discard 竞争立即返回 TIMEOUT/false，已有 Loan 归还独立进行。直接并发 publish 的发布 lane 竞争返回 EXHAUSTED，DDS Writer 本身已串行化提交。关闭返回 CLOSED，损坏 session 返回 CORRUPT，参数或身份不符返回 INVALID。创建/映射配置错误抛出 `invalid_argument`，系统调用失败通常抛出 `system_error`。
- DataWriter 删除时显式调用 `closeWriter()`。仅丢弃一个 owner 句柄不能保证关闭，因为本进程 Reader 注册可能仍强引用该池。
- 原始 `IPayloadPool` 地址接口仅供内部历史使用，不能识别地址复用后的旧别名；DDS Loan 必须保留 token。`SampleLoan` 已保留 token；Reader 的 `CacheChange::nativePayload` 已能通过 aliasing shared_ptr 保活 `SharedReadLoan`，QoS、发现及自动接收已接入；命名注册可按 GUID 选择发布目标，带有时间戳、实例与关联样本身份元信息。双方 TRANSIENT_LOCAL 时注册可重放仍被 Writer 持有的槽位；无 Reader 时的原生预发布与独立进程晚加入已有可靠/尽力回归。

共享池 fork+exec 测试证明不同地址映射同一载荷；独立 DDS exec 测试进一步证明正常发现、Publisher/Subscriber、Loan/普通 write、take/return_loan 可以自动绕过两端序列化器。原有 HelloWorld 传输示例仍使用 CDR，不作为共享旁路证据。

## 类型能力与 Loan 接口

TypeSupport 的 `get_native_type_info()` 默认返回无布局能力，`supports_native_uninitialized_sample()` 默认 false，`construct_native_sample` 默认失败。NativeTypeInfo 区分 bounded/plain，携带大小、对齐与版本化布局 ID。NativeTypeSupport<T> 对标准布局、可平凡复制/析构类型仅在 trivially default constructible 时默认允许 native，并用 placement new(storage) T 开始对象寿命，不使用 T{}；其他旧类型仍可编译并使用 CDR。统一门控还检查带键 NativeKeyInfo；缺失无初始化资格或键元数据非法时 ON 拒绝，AUTO 不广告 DS 并保留 CDR/RTPS。应用仍须保证无进程内指针，发布前填写所有可能被读取的字段。仓库没有 IDL 自动 native 适配生成器，示例声明与键钩子为手写；HelloWorld 无界 string 和普通 string/vector 类型走 CDR。

Writer 的 `SampleLoan` 是不可复制、可移动的句柄，内部绑定独立 Writer 身份、共享池、会话、槽位和代次。`loan_sample(SampleLoan&)` 创建记录，`write(SampleLoan&, handle)` 成功后消费记录，返回错误时保留以便重试；`discard_loan` 或句柄析构取消未提交的借用。错误 Writer、空/已消费句柄及重复提交/取消返回 PRECONDITION_NOT_MET。没有暴露无法验证 ABA 的裸地址 Loan 提交接口。

Writer 删除时显式关闭共享池；未归还句柄继续保活映射，直到析构。当前由 DataSharing QoS 控制容量、目录与预算，enable 时准备共享池并生成不可变的发现能力；固定布局类型存在兼容且已注册的共享 Reader 时，成功 write 将该槽位提交给共享 Reader，并保留在 CacheChange 中；没有共享目标时仍走序列化 RTPS。

`SequenceBase`/`SequenceData<T>` 已区分自有对象与借用对象。借用序列持有共享 pin 与独立的地址数组，`maximum()==0`、`has_loan()==true`，不会 delete 样本地址；追加和修改长度被拒绝。复制借用序列会深拷贝到自有对象，移动则转移 pin；析构/unloan 释放自己的引用。空序列不拥有样本存储，`has_ownership()==false`。

Reader 的 10 个序列 read/take 变体统一经过 `MaterializeSamples`。当调用前数据序列 maximum 为 0，且当前批次全是匹配布局的有效原生样本时，返回原生地址并为数据与 SampleInfo 绑定同一个批次 pin；不调用序列化、反序列化或 copy_data。预分配数据序列、混合批次或控制样本走自有对象分支，其中原生数据使用 copy_data，普通 RTPS 数据使用 CDR。两个 single-sample API 使用复制分支。

`return_loan` 校验 Reader 身份、批次 pin、数据/元信息角色与长度，配对成功才释放双方引用。错误配对不改变已有借用；双方都没有借用时保持原行为并返回 OK。已有 Loan 时重复读取到同一输出序列返回 PRECONDITION_NOT_MET，避免隐式丢弃。`read` 保留 history 引用，`take` 移除 history 引用；任意应用 Loan 仍存活时都保留相应缓存与共享槽位。Reader 删除后，借用序列仍可读取并通过析构归还，不依赖 Reader 裸指针。

借出的 Reader 样本必须视为只读。现有 DDS 序列 API 返回 T&，类型系统不能阻止应用修改共享字节，调用者必须遵守该约定。API 细节见 `DATASHARING-API.md`。

## 发现、实际映射与传输选择

发现数据已声明版本、共享域、随机会话、段容量和类型布局；主机 boot ID 与 PID namespace 必须相同。自定义 PID 的正文有独立字节序和严格长度限制，旧端点缺失扩展或输入不合法时能力为空。Reader 以自己的本地目录拼接对端 GUID/会话文件名，验证 descriptor、文件与布局后注册 GUID。Writer 将实际注册列表与已匹配的能力候选取交集，按每个样本固定共享目标；其他匹配 Reader 使用定向 RTPS。共享监听产生的 CacheChange 经现有 WriterProxy 排序、去重及 Notify 入 history，RTPS heartbeat/ACK 仍管理可靠性。

纯共享匹配无需调用序列化器。混合匹配只为需要 RTPS 的端点生成序列化载荷，共享 Reader 继续读取原样本。拒绝共享能力或类型不兼容的端点仍应获得正常 RTPS 数据，不能因为一个共享 Reader 匹配而让远程 Reader 丢失数据。

DataSharingQosPolicy 的 AUTO/ON/OFF、共享目录/域、容量及段预算已接入默认值、set/get、合法性、不可变策略检查和 XML 继承。Writer 的 ON 在本地准备失败时返回错误，AUTO 记录资源失败并保留 RTPS；OFF 不分配原生池。Reader 映射失败保留 RTPS 并记录日志；兼容但映射/注册暂时失败的 Reader 先退回RTPS，再通过可取消的独立worker自动重试；可靠/尽力独立exec恢复不需重建Reader。普通池策略见 PAYLOADPOOL-QOS.md。配置边界见 `DATASHARING-QOS.md`。


### 提交点与监听生命周期

Writer 在原有 history 容量检查和插入之后调用共享发布，再交给 flow controller。发布失败移除本次缓存，保留可写 Loan；发布成功后不再把可写所有权退给应用，即使后续 RTPS 发送失败或抛异常也返回接受成功并记录日志。混合匹配在发布前生成必要的 CDR；纯共享匹配不序列化，晚加入的回退端点可以通过 CacheChange 的互斥保护延迟生成 CDR。

Reader 每个已匹配共享 Writer 有独立线程与运行状态。运行状态持有共享注册，线程通过 weak_ptr 获取 Reader；停止先关闭新/未读投递，随后在不持有 ingress、匹配或连接映射锁时 join。自身线程的最后一次析构使用 detach，运行状态继续保活；Reader 应用 Loan 的存储与监听线程相互独立。数据、分片、heartbeat、GAP 以及原生提交使用同一 Reader ingress 锁，避免并发操作 WriterProxy 状态。匹配表使用独立短锁，获取 shared_ptr 后释放；liveliness 回调查询匹配时不等待 ingress，也不在持有匹配表锁时调用 liveliness manager。

共享布局版本为 6，包含原子 slot/registration gate、完整 64 位代次、Reader GUID/接受及 ready 状态、replay 标志、样本元信息与 futex 通知代次。本轮发现能力版本为 4，以隔离仍可能清零借出载荷的 v3 对端；v1/v2/v3 和未知版本都不选择 native。v4 同时要求无初始化与强制 key-only 准入。建立共享路径前的不兼容保留 RTPS。已经损坏的 native session 需要新会话，不能把协议降级等同于任意运行时错误都透明恢复。

## 验收证据

- 普通池：四策略的容量、对齐、并发、回收；真实 Writer 的池共享、分片边界、写入失败和历史保活；六种 take 路径保活。
- 共享池：两个独立进程映射同一载荷；不同基址；多 Reader、慢 Reader、耗尽、代次复用、退出/崩溃/重启和资源清理。
- 端到端：将测试类型序列化/反序列化函数设为失败，纯共享 Loan 仍成功；纯 RTPS 和混合匹配的数据内容、顺序及投递次数正确。
- 配置：API/XML 一致，默认值及非法值明确，AUTO/ON/OFF 匹配矩阵完整。
- 工程：新代码覆盖率达到 70%，完整 Loan 发布、读取和 QoS 示例可复现；设计和 API 手册与最终实现一致。


## 带键注册与查询

键哈希使用现有 TypeSupport::MakeKey，统一按 0 成功处理，失败不会继续注册或发送。Writer 的注册、写入、注销与键查询串行协调，history 的注册与快照获取使用短互斥锁及共享所有权。显式句柄必须已注册且与样本键一致；隐式注册保留资源错误码。

Writer 注册与 Reader 首次接受 ALIVE 时建立独立本地键快照。原生带键类型必须提供有效 NativeKeyInfo，capture_native_key/restore_native_key 只捕获和还原键；非键字段保持调用前值。端点 enable 只验证元数据契约，不用假样本探测钩子；声明了元数据却未实现钩子时，实际操作失败关闭。缺失、非法、false 或标准异常均不触发原生全样本 memcpy/copy_data 回退。Writer 和 Reader 两端独立执行相同准入门控，discovery v3 排除容许旧全样本快照的 v2 对端。具体键表示仍属于本地元数据，不通过共享段或发现传送。键快照和索引会复制少量元数据，“零载荷复制”指完整用户载荷没有复制分支，不是任何字节都不复制。快照不保存共享槽位引用；CDR 类型仍保存独立分片链，每次查询使用独立游标。首次捕获失败时，Reader 在排序、过滤、淘汰及所有权状态变更前拒绝本条准入并回滚新建空实例，不占 max_instances；既有实例与历史保持。已有键快照还原失败不会丢弃保存的快照。详细类型契约与错误码见 [API 手册](DATASHARING-API.md#带键实例与本地快照)。

带键样本与无键样本走相同的原生提交/定向混合回退路径。实例句柄已包含在共享元数据内；dispose/unregister 继续使用原有 RTPS inline key/status，Reader 沿原代理和 history 更新状态。内置发现 Reader 没有应用 TypeSupport，必须跳过实例快照逻辑。注销之后 Writer 清理该实例历史和注册快照；当前验证覆盖代表性的持久性和控制通知组合；任意丢包、重传与线程交错的完整矩阵未穷尽，这是验证范围限制，不另作为官方八项之外的交付要求。


## 历史、目标隔离与进程重启

TRANSIENT_LOCAL Writer 即使尚无 Reader，也保留原生共享历史。重放注册先 OPEN，历史扫描与发布水位协调完成后才置 ready；registeredReaders 只暴露 ready 登记，过渡期继续 RTPS。共享发布同时考虑 OPEN 的过渡登记，已有序号逻辑处理可能的重复。VOLATILE Reader 不重放旧槽位。晚加入 OFF Reader 按需生成 CDR。BEST_EFFORT 的保留历史在匹配时定向发送一次，仍受发现顺序和丢包影响，不提供可靠重发保证。混合晚加入回归先建立原生 Reader，再创建 OFF Reader。

native 缓存回退为定向 RTPS 时，DATA、DATA_FRAG 及分片重发带 INFO_DST 的 Participant 前缀；接收端在查 Reader entityId 前检查目标前缀，UNKNOWN 保留原广播语义。Reader entityId 仅在 Participant 内唯一，不能独立作为跨 Participant 的目标身份。

共享池控制维护以 PID 和启动身份确认死 Reader；登记无短操作时才回收 pending/Loan pin，不能确定短操作安全性时损坏会话。DDS discovery lease 对失联端点的保留是另一套生命周期。旧 Reader 仍在 lease 中时，Writer 可能为其生成 CDR；重启路径不能承诺 Writer 始终零序列化。死 Writer 的 orphan 只能在验证 GUID、session、文件身份和实际死亡后精确移除；已有 Loan 的映射保持有效。持锁进程死亡时仍安全拒绝损坏段，应用需重建端点，不承诺原池无损恢复。

Reader 实例在 history 准入成功后才进入已知键索引，max_instances 不被预登记绕过。默认无限 autopurge 下，take、Writer 消失或归还 Loan 不删除已知键和键快照；重建 Reader 获得新预算。有限autopurge现按状态转移的单调时间回收实例、键快照与预算；Loan共享引用保活，回收不强制复用其物理槽位。


## Reader 销毁与发现回调

用户 Reader 的匹配状态回调与解除 DCPS 关联使用同一个 ingress 锁。析构先在锁外停止并等待原生监听，再解除关联，以等待正在执行的匹配回调结束。仅保活 RTPS Reader 不能保活其关联的 DCPS 对象。

内置发现 Reader 的 DCPS owner 在 Participant 停止接收后销毁，沿用该生命周期。其匹配通知不能获取另一个内置 Reader 的 ingress 锁：SPDP 匹配在 Reader 表锁内运行，而 SEDP 接收先持有自身 ingress 再等待 Reader 表锁，反向加锁会死锁。受控回归同时检查内置通知不等待 ingress、用户 Reader 通知等待 ingress，以及用户 Reader 解除关联等待在途回调。


Reader 的任意时刻 SIGKILL 可能遗留短操作计数或打断控制事务，不能保证原池每次都继续服务。registeredReaders 只读取 ready 快照，不做死亡探测；维护在可安全回收时清引用，否则标记 CORRUPT，应用删除并重建对应端点，以新会话恢复。已有 Loan 保留旧映射，不能为恢复容量强制覆写。

共享段后端组件选择、目录兼容、通知与控制锁职责，以及 Boost 固定版本依赖见 [Boost 后端设计](DATASHARING-BOOST.md)。当前实现和验收范围见需求矩阵；旧后端通过数字仅作历史记录。

## 2026-10-28 同步与失败边界

映射worker以weak_ptr获取Reader，不在匹配表、ingress或连接表锁内执行open/register。初始化flock为非阻塞；退避等待可取消，listener等待按100ms维护间隔重查，实际响应还受系统调度影响。新的控制注册可能等待管理锁，不承诺端到端硬实时上界。相同GUID/session重复发现保留代理序号流和单一注册，新session取消旧worker，跨匹配锁不join，旧运行状态自行退出。实际注册先于共享目标选择；RTPS与DS沿用同一WriterProxy排序/去重。损坏段不重新启用，新会话重新建立。

Reader序列API采用preview→materialize→commit，缓存身份而非全局序号作为提交依据。所有可判断的尺寸/对齐/非法输出预校验在history操作前执行；分配或反序列化失败保留未读样本，输出可能已清空。应用读取互斥序列化选择与提交，复制/反序列化不持history锁；若并发过期/KEEP_LAST淘汰，preview的shared_ptr仍固定存储，commit跳过已移除身份。

源时间用于lifespan，接收时间仅为缺失INFO_TS的内部替代，单调时间用于deadline/autopurge/TBF。已入history的过期数据按缓存身份删除并维护计数；不同Writer相同SN互不误删。通用Timer的共享Runtime在回调外保活，period/next/running/revision由同一锁同步；回调不持Timer锁，外部Exit在锁外join，自释放回调可detach自身。端点无限QoS停止已存在Timer，避免在调度锁内销毁/join。历史无栈WaitSet崩溃的根因仍未知；条件析构现在先交换附着表再逐一detach，重复验证不能证明旧崩溃已归因。

TBF定时提交在完成history事务和deadline推进后释放ingress，再调用应用listener。DCPS Reader析构先在锁外Exit并等待该Timer，之后解除关联；closing状态拒绝新的set_qos，避免回调释放锁后的对象寿命空窗。DATA_AVAILABLE状态先更新再交给listener，回调内take可以正确清除状态，不会被回调返回后的旧置位覆盖。


2026-10-31 共享布局 v6 / discovery v4 的 Reader 普通 Topic 池与 native 无初始化契约已补齐。当前 321 项冻结输入对应 529 项普通回归、9 次有限示例、93.936% 新增生产可执行行覆盖率，ASan/UBSan 与 TSan focused 各 86 项通过；最终审计及主包/历史附包搬迁复核通过，原始范围、哈希和复现命令见 [10/31 报告](DATASHARING-PROGRESS-20261031.md)。[10/30 报告](DATASHARING-STRICT-20261030.md) 的 500 项、93.920%、9 次示例及两版 focused 各 57 项仅证明当日 discovery v3 冻结源码。LSan 未启用，历史 WaitSet 崩溃根因仍未知，当前通过不作根因归属证明。
