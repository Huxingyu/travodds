# DataSharing / Loan 历史验收状态（2026-10-28）

> 本页为布局 v5 的冻结验收与阶段历史，“当前”“完成”仅指当日版本。布局 v6、发现能力 v4 与无初始化/key-only 准入的当前验收结果见 [当前需求矩阵](DATASHARING-ACCEPTANCE.md) 和 [10/31 报告](DATASHARING-PROGRESS-20261031.md)。旧测试、示例和覆盖率不能替代当前版本结果。

实现与本机验收完成：review三项修复、自动映射/注册重试、有限Reader实例回收、统一源时间/lifespan和Timer/QoS同步；验收中另定位修复并发首次日志terminate和SPDP发送缓冲竞争。

固定冻结输入SHA256：`953cfc4395a9273ef38aa0e01463f46a83de6dfc0f04b7a17be369e2f2b78361`，基线4890671，Boost布局v5。普通和ASan/UBSan各468/468、两版各55×30重点重复、两版三个有限示例各30轮均通过。default/SHM/UDP、TCP/WaitSet5×6通过；独立TSan相关3×30+7×10通过。新增可执行生产行2432/2584=94.118%，最终Boost性能27次通过。小消息共享路径慢于本机RTPS，64KiB显著更快，具体方法和限制见性能手册。

正式结果、八项交付、失败保留与限制见 [本轮报告](DATASHARING-COMPLETION-20261028.md) 和 [验收矩阵](DATASHARING-ACCEPTANCE.md)。本轮证据根为`artifacts/completion-20261028/`，完整补丁和源清单由最终审计生成。没有Git提交、推送或PR。

LSan受宿主ptrace限制未运行，不宣称无泄漏；历史无栈WaitSet崩溃根因未知；Ubuntu SECURITY=OFF，无跨主机性能/双平台/生产SLA承诺。旧438项和旧94.68%仅是历史，不代表本轮版本。

## 历史实施记录

以下保留各阶段原记录，时间与范围仅属于当时版本；其中“未实现”“待验收”与旧数字不作为当前状态。

## 需要保持的约束

- 默认保留正常 RTPS 读写行为。DS 的可用性和通路按匹配端点判定，同一 Writer 可同时面对 DS 与 RTPS Reader。
- 共享段内不保存进程内地址、std::string、std::vector 或虚函数对象。传递段身份、槽位和代次；本地映射地址可以不同。
- bounded 与 plain 分别判定，类型能力默认保守；不为未声明兼容布局的旧类型绕过序列化。
- 写入共享样本后才发布通知；通知获取之后才能访问样本。慢 Reader 持有样本时不能静默覆盖其正在访问的内存。
- 所有池都有可审计的容量和耗尽行为。丢弃、读取确认、Reader Loan 归还与 history 移除是不同事件。
- 同一共享样本可被多个 Reader 使用；引用状态不能让一个 Reader 的归还释放其他 Reader 的借用。
- 进程退出或异常后必须有明确的会话代次及资源处理，不能根据相同名字误接旧段。
- 已知限制和暂未覆盖的场景逐项记录，不能以“支持零拷贝”概括尚未验证的端到端行为。

## 实施顺序

1. 完成普通 Topic 池、策略和注册表，统一载荷归属。
2. 实现共享段池和可独立测试的生命周期协议、通知与监听。
3. 补齐类型元数据和 Loan 容器，接入 DDS API。
4. 接入 discovery、读写旁路、QoS/XML 与 RTPS 混合回退。
5. 多进程异常/并发回归、性能、覆盖率和正式文档。

最小闭环用于提前暴露集成问题；全部目标仍包括多 Topic、多 Reader、QoS、回退、覆盖率和文档。

## 普通池生产接入阶段

- 用户 Writer 使用按 domain/topic/type 注册的动态复用池；控制缓存显式携带原池。键注册和内置发现缓冲仍保留旧池归属。
- 分片缓冲链由 RAII 管理，部分分配失败、序列化错误/异常与长度溢出均有测试；成功创建 CacheChange 后转移所有权。
- 六种 Reader take 路径去掉直接归还全局 MemoryPool 的操作。HistoryCache::ReadOrTakeInstance 同步移除可能存在的总索引引用、更新样本计数并通知空间等待者。
- 原有 Reader 常规接收通过 AddChanges 保存实例列表，未必使用总索引；总索引清理问题针对同时使用该索引的插入路径。测试分别覆盖两条路径，不能把其中一条的行为泛化到所有 Reader。
- 51 项测试与默认/纯 SHM/纯 UDP 双进程样例已通过；本轮核心回归 247/247 通过；独立 ASan/UBSan 核心库和组件测试构建后，同批 51 项测试通过且无 sanitizer 报告。详见运行报告与原始证据。
- 普通池接入尚不提供共享段、Loan、端点协商或零拷贝能力。全部八项目标仍有效。

## Linux 共享池组件阶段

- `SharedPayloadPool` 使用 Writer GUID 与随机会话命名，独立进程分别映射；共享布局不含进程内指针。
- 环形槽位分开记录 Writer、未读投递和 Reader Loan 引用；全部释放后复用。以会话、槽位和 64 位代次拒绝旧 token。
- process-shared 条件变量通知和 robust 锁处理等待与异常；持锁进程死亡使段进入 CORRUPT，保留已借用数据，停止新分配。
- Reader PID 配合进程启动身份检测死亡；不能确认死亡时不回收。Writer 正常关闭或异常退出后，已存在的 Reader Loan 继续保活映射。
- 本机已完成单线程完整构建、51 项普通池测试、247 项核心回归和三种双进程示例。共享池独立进程验证与前述 DDS 示例是两类证据，不能合并表述成 DDS DataSharing 已完成。
- 共享池 17 项测试通过，包含三 Reader 各 400 条样本的并发与慢 Loan 场景。独立 ASan/UBSan 核心库和测试构建后，51+17 项同样通过；本机 ptrace 限制使 LeakSanitizer 无法工作，最终检查关闭泄漏扫描。运行报告保留限制与失败证据。
- 本阶段仅验证共享池组件；后续类型能力、Loan 容器和 Reader 原生 history 接入进展见下一节。自动 listener、发现/QoS/XML、混合回退、openEuler 和性能验收仍未完成。

## DDS Loan 与类型能力阶段

- `NativeTypeInfo` 显式区分 bounded/plain 并记录大小、对齐和布局 ID；旧 TypeSupport 默认不支持。`NativeTypeSupport<T>` 以编译期约束与 placement-new 初始化固定布局样本，字段无进程内指针仍是类型提供者的约定。
- `SampleLoan` 是绑定 Writer 身份和共享段 token 的移动句柄，析构自动取消。写入返回错误时保留借用，成功的普通 RTPS 回退写入消费句柄。并发借出、池耗尽、错误 Writer、重复操作、构造失败和 Writer 删除保活已有验证。
- Reader 的十种序列读取方法与两个单样本方法统一处理原生缓存。空输出序列可直接借用原生地址，预分配输出复制；`return_loan` 校验 Reader、批次及数据/元信息角色。历史与应用引用分离，Reader 删除后仍可保活样本。
- 当前内存池/Loan 集成测试共 79 项（原有 51 项加本轮 28 项），共享池 17 项；普通与独立 ASan/UBSan 构建均通过。覆盖率来自带 `--coverage` 的独立核心库与测试，排除测试和依赖，合并生产代码模板实例；不是分支覆盖率，也不替代未实现功能的验收。
- 回归暴露原有 WaitSet 残留通知和 ReadCondition 查询消耗变化标记的问题。通知现仅作唤醒，条件按当前状态判断；新增确定性测试覆盖旧通知、多个读取条件、read/take 与应用 Loan 的区别。核心测试 main 已改为返回 Google Test 状态，避免失败时仍退出 0。
- 原生 Reader 测试通过内部 history 注入真实共享段借用，并把反序列化器设为失败来证明旁路；尚未通过发现和 listener 自动连接 DDS Writer/Reader。下一步仍是完整共享发布与接收、端点协商、QoS/XML 及混合端点回退。

## QoS/XML、发现候选与接收线程生命周期阶段

- `DataSharingQosPolicy` 提供 AUTO/ON/OFF、共享目录、共享组、槽位数、Reader 上限和段预算。默认值、get/set、创建、校验、不可变检查与 XML 继承保留配置；Writer enable 将其传入真实共享池。
- XML 拒绝未知/重复节点、非法嵌套、负数/溢出和非法共享域。修正旧 Writer/Reader profile 在解析失败时将错误码转为 bool true 的问题。
- SEDP 的自定义 PID 传输版本、布局、共享组、主机/PID namespace 和 Writer 随机会话/容量，正文独立字节序且有边界校验。候选匹配不是实际映射/注册，也没有启用自动共享调度。
- 真实两个独立 exec 进程可获取对端能力并通过 RELIABLE RTPS 收发；测试明确验证序列化/反序列化发生，不把它计为零拷贝验证。
- 旧接收 socket 的最后引用在自身回调释放时会 SIGABRT；现在事件循环由独立 Runtime 保活，自身线程 detach，外部线程 join。旧头文件最小复现中止，修复后的相同程序成功；新增 25 次回调内释放和外部释放回归。
- 当前普通测试 247+96+17=360 项，独立 ASan/UBSan 测试 96+17=113 项；新增生产代码行覆盖率 1166/1225（95.184%）。仍是行覆盖率而非分支覆盖率，只覆盖已实现代码。完整自动共享链路、混合端点与最终验收仍待继续。

## 自动共享调度接入阶段（2026-10-27，已完成本阶段验证）

- 共享布局版本 2 保存 Reader GUID、是否继续接收和源时间戳/实例/关联身份。publish 可选目标，close 清除未读投递但保留已有 Loan；注册可重放 Writer 仍持有的已发布槽位。
- Reader 从本地 QoS 目录推导对端会话文件名，验证并注册；每个共享 Writer 有独立 listener。共享缓存走原 WriterProxy/Reader history，沿用 RTPS heartbeat/ACK。停止和 join 不持有 ingress/匹配/连接映射锁。
- Writer 无键固定布局类型在有实际共享目标时发布原生槽位；普通 write 复制到共享槽位，SampleLoan write 直接提交借用。混合匹配预先生成一次 CDR，并按 Reader entity ID 定向发送；纯共享不调用序列化。
- 共享提交在 history 资源检查/插入之后。发布失败回滚本次 history；发布成功即消费应用可写 Loan，后续 RTPS/定时器错误不能将可写所有权退还应用。
- 4 个独立 exec DDS 场景通过：可靠/尽力 × 纯共享/同参与者 ON+OFF 混合。纯共享双方序列化器显式失败仍收样，混合只调用 3 次 CDR；检查时间戳、无重复及 Writer/Reader 删除后借用保活。新增匹配查询不被 ingress 阻塞的确定性测试。
- 本阶段最终普通测试 247+104+22=373 项通过；独立 ASan/UBSan 104+22=126 项通过。新增生产代码行覆盖率 1504/1617（93.012%），无泄漏扫描结论。5 个 exec 用例额外连续 30 轮（150 次）通过，TCPv4/v6 额外 6 轮（12 次）通过；默认/SHM/UDP 三组双进程 smoke 通过。
- 第一轮完整回归在 ownership 匹配失败后卡住，由工具终止；单独 ownership 和 liveliness→ownership 通过，未获得卡住现场栈。静态检查另发现 liveliness 持锁回调与 ingress 反向依赖，现将匹配表查询改为独立短锁，收包状态继续在 ingress 下处理，不把静态推断当作第一次卡住的已证实栈归因。
- 第一轮独立 sanitizer 没有 ASan/UBSan 诊断，但两个可靠端点启动/匹配超时，故整批未通过；旧日志保留为 *-native-first。最终依赖和测试需一起重编。

- 后续普通回归出现 TCPv6 SIGSEGV，没有取得该次崩溃栈。同一源码另有可独立复现的 TCP 空 socket 问题：异步连接在 socket 构造前已有占位项，SetStop/析构可能解引用空指针。占用端口触发 bind 失败的同一复现程序在旧库 SIGSEGV（-11），在修复库正常退出；增加两项确定性回归。该独立证据不证明它就是前次 TCPv6 崩溃的直接原因。
- TCP 修复版的独立 ASan/UBSan 104+22 项已通过，但普通版双进程启动间歇失败。临时信号栈记录抓到 `Participant::AddLocalReader` 在 `std::_Rb_tree_increment` 崩溃；基线中此函数也未持锁遍历 `remoteWriters_`，而发现线程可在另一线程删除 Writer。现补上与 Add/RemoveRemoteWriter 相同的 `mtxReaders_`，修复后的重复及完整回归已通过，最终证据见下一条。信号记录仅用于定位，最终验收运行不加载该诊断模块。

- 本阶段证据：`artifacts/local-loan/results/{core,payloadpool,sharedpayloadpool}-native-final.xml`、`verification-native.json`、`smoke-native-final/summary.json`，独立 instrumented 结果与覆盖率在 `artifacts/local-loan-sanitize/`。完整核心耗时 250.426 秒，普通增量构建最大单子进程 RSS 520048 KiB，独立检查版本次增量构建 826644 KiB；不是整机峰值或完整构建耗时。

剩余范围包括：带键旁路、完整持久性/并发/异常与配置匹配矩阵、三个完整 DataSharing 示例、openEuler 与性能验收。全部八项目标保持有效。


## 普通池公开配置与发现回归阶段（2026-10-27，已完成本阶段验证）

- `DataWriterQos::payload_pool` 将四种普通池策略及块尺寸、容量、初始块数、对齐传入真实 Writer Topic 池。默认参数保留原动态复用行为。
- 同 domain/topic/type 的活动池要求配置完全一致；创建接口将配置冲突及分配异常转换为 nullptr。所有分配参数不可变，独立于端点协商及原生共享段预算。
- XML 支持严格解析与继承覆盖；块计数及 RTPS 预留区说明见 `PAYLOADPOOL-QOS.md`。
- 新增配置合法性/不可变性、XML、四策略真实分配与耗尽、Writer 删除后的缓冲保活、配置冲突不影响现有 Writer 的回归。最终验证为普通版 247+113+22=382 项通过；独立 ASan/UBSan 113+22=135 项通过，LeakSanitizer 仍关闭。

- 本阶段首次独立 sanitizer 运行有一个可靠混合端点在子进程创建 Reader 前超时，未出现 ASan/UBSan 诊断。随后重复捕获三个超时现场：SPDP 线程持有 Participant Reader 表锁，在匹配内置 SEDP Reader 时进入 StartDataSharing 等待该 Reader ingress；SEDP 收包线程持有该 ingress，反向等待 Participant 表锁。主线程创建 Reader 也等待表锁。线程栈保留于 `artifacts/local-loan-sanitize/logs/native-child-*-gdb.log`。
- DataSharing listener 仅用于用户 Reader，现于获取 ingress 前排除所有内置端点。两个确定性回归在同一旧库上均因等待忙 ingress 失败，在修复库上通过；没有以延长超时规避。修复后的全部测试、重复运行及覆盖率将重新记录。

- 修复内置端点死锁后的 ASan/UBSan 重复运行中，四种原生共享场景全部通过，但 RTPS 回退场景在第 27 轮出现匹配句柄可见、查询发布信息返回 BAD_PARAMETER。新增双向匹配回调查询回归，旧库在 Reader/Writer 两边稳定返回 BAD_PARAMETER，修复后均返回 OK。现在 AddRemoteWriter/AddRemoteReader 在公布匹配之前保存元数据，查询与增删使用同一短锁；回调查询不重新获取 Reader/Writer 表锁。最终检查再次重跑；该轮失败日志以 `native-exec-public-pool-metadata-failed.log` 保留。

- 最终源码上普通与独立 ASan/UBSan 版各连续通过 30 轮 × 5 个 exec 场景（各 150 次），包含真实原生收发和 RTPS 回退。TCPv4/v6 另连续 6 轮共 12 次通过；默认、纯 SHM、纯 UDP 三组 smoke 通过。核心回归耗时 250.568 秒。
- 新增生产代码行覆盖率 1559/1682（92.687%），仍按固定基线的新增可执行行计算，排除测试、依赖和文档；不代表分支覆盖或完整功能验收。普通与检查版运行时链接及安装头文件均核对到各自任务前缀。
- 证据为 `artifacts/local-loan/results/verification-public-pool.json`、`{core,payloadpool,sharedpayloadpool}-public-pool-final.xml`、`metadata-{before,after}.xml`、`builtin-ingress-{before,after}.xml`，独立检查与覆盖率在 `artifacts/local-loan-sanitize/`。先前失败日志完整保留；前一阶段源清单、补丁、覆盖率与 sanitizer XML 归档于 `results/native-stage/`。
- `bootstrap.sh` 默认并行度现为 1；独立检查脚本在 coverage 模式下于测试枚举前清除旧 gcda 校验和，并在实际测试前重新清零，避免将枚举数据计入覆盖率。普通池说明见 `PAYLOADPOOL-QOS.md`。

全部八项目标仍有效。后续验收包括带键原生旁路、完整持久性/异常/并发与配置匹配矩阵、三个有限运行的 DataSharing 示例、openEuler 与性能，以及最终设计/API/调试文档。现有有限测试未替代这些验收。


## 带键实例生命周期与完整检查（2026-10-27，本阶段通过）

本阶段继续在当前 Ubuntu 机器上单线程编译，未使用远端。带键固定布局类型已接入原生共享路径；普通 write 与 Writer Loan write 均可绕过载荷序列化。注册、lookup/get_key_value、错误句柄、max_instances 耗尽、dispose、unregister 和重新注册已有验证。键哈希仍执行 TypeSupport 的键操作；生命周期控制通知通过 RTPS inline key/status，不将其称为完全没有网络消息。

Writer 与 Reader 分别保存独立的本地实例快照。Reader 在 take/归还后仍可重复查询键，原生快照不钉住共享槽位；CDR 快照复制全部分片，每次查询使用独立游标。原生快照按类型对齐并处理构造失败，CDR 注册的错误/异常回滚通过原池回收。详细语义与内存预算见 `DATASHARING-API.md`。

- 最终普通版 **247+122+22=391 项**通过，核心耗时 **250.530 秒**。
- 本阶段首次扩展到独立 ASan/UBSan 核心测试，最终 **247+122+22=391 项**通过，核心耗时 **253.613 秒**，无检查器诊断。LeakSanitizer 仍因本机 ptrace 限制关闭。
- 两版分别连续通过 **30 轮 × 9 个 exec 场景 = 270 次**，包含有键/无键、可靠/尽力、纯共享/混合与 CDR 回退。带键场景逐阶段检查 ALIVE/dispose/重新 ALIVE/unregister/重新注册、实例句柄与代数；纯共享载荷序列化/反序列化仍为零调用，混合每次写入只生成一份载荷 CDR。
- TCPv4/v6 再通过 6 轮共 12 次检查，默认/纯 SHM/纯 UDP 三组旧示例 smoke 通过。旧示例仍由工具停止，不能作为三个有限运行 DataSharing 示例的交付。
- 新增生产代码行覆盖率为 **1727/1831（94.32%）**，按固定基线合并 131 个翻译单元；此次也包含完整核心测试的计数。它不是分支覆盖率或项目完成度。

扩展检查抓到并修正基线旧 SHM 路径的未对齐结构体访问：字节环形记录不能直接 reinterpret_cast 为 Message_t，现用 memcpy 读取长度。原始 UBSan 失败、修复后两个分片用例通过及源/库 SHA 保存在 `artifacts/debug-native/shm-alignment-proof.json`。内容过滤测试的 28 个样本也改为值初始化，避免枚举数组含随机值；过滤断言保持原样。`sanitize.sh` 提供 `CORE_TESTS=1` 复现完整核心检查。

原有重复注销用例假定旧句柄仍然有效，与公开 API 的“当前注册的实例”约束冲突。本阶段在注销后释放实例注册与快照，测试改为分别检查首次注销成功、旧句柄 BAD_PARAMETER、lookup 为 NIL，以及重新注册后可再次注销。

本阶段还保留三类早期失败：Writer CDR 重复键查询推进了保存的游标；新 Reader 快照逻辑最初未排除没有应用 TypeSupport 的内置 Reader；一次普通完整核心运行在最后的 WaitSet 用例 SIGSEGV。前两项已修正并有确定性回归。第三项未取得栈，随后带栈记录的完整回归、30 轮 WaitSet 重复、最终普通/独立检查完整回归均通过，**根因仍未确定**；不将另行证实的 SHM 对齐问题当成这次崩溃的已证实原因。原始失败日志、诊断和说明见 `artifacts/debug-native/keyed-stage-issues.json`。崩溃遗留的一段 DataSharing 文件仅在核对 Writer/会话、PID/start 和路径后由 removeOrphan 清理，未进行通配删除。

当前汇总为 `artifacts/local-loan/results/verification-keyed.json`，普通结果为 `*-keyed-final.xml`，独立检查结果和覆盖率在 `artifacts/local-loan-sanitize/`。源清单、完整补丁及安装/运行库一致性可独立核对。上一阶段源清单、补丁、覆盖率和原始 sanitizer 结果已归档到 `results/public-pool-stage/`。改动仍未提交、未推送。

全部八项交付目标仍未完成：剩余包含完整持久性/并发/异常与匹配矩阵、注销与定时 QoS 的交互、Reader 已知实例回收、三个有限运行的 DataSharing 示例、openEuler、性能验收与最终调试手册。


## 三个有限示例与迟加入 Reader 的 GAP 修复（2026-10-27，本阶段通过）

`example/cpp/data_sharing/` 提供三个只使用公开 DDS API 的独立进程示例：基础 Loan、带键实例生命周期、共享 ON Reader 与普通 OFF Reader 混用。每个示例完成固定数据量后自然退出，校验 Writer/Reader 删除后的借用保活，并检查共享段 unlink 及私有目录清理。构建、参数与调用流程见 [示例 README](../example/cpp/data_sharing/README.md)，排查方法见 [调试手册](DATASHARING-DEBUG.md)。`bootstrap.sh` 与 `sanitize.sh` 都包含这三个示例的单线程编译。

发现匹配不等于共享映射已可用。示例先禁用完整载荷序列化/反序列化，以真实 Reader Loan 完成共享通路探测，再重置计数；混合示例在探测后创建 OFF Reader。基础示例实际收 6 条，带键示例收 5 条 ALIVE 和 2 条控制消息，二者完整载荷 CDR 均为零调用；混合示例两个 Reader 各收 6 条，Writer 序列化与 OFF Reader 反序列化分别精确为 6 次。

混合示例首轮在正式收样时超时，保留了失败日志。确定性回归证实旧 WriterProxy 的两处 GAP 状态问题：VOLATILE Reader 首先收到序号 2 后，没有序号 1 的占位，后来 GAP(1) 未补上缺口；扩展 GAP 范围时还会覆盖范围中的已收缓存。现在显式补入尚未提交的缺口，仅向已有最高序号之后扩展占位，并保留已收到的数据。5 项回归在旧库上 2 项失败，修复库全部通过。该修复不用于解释此前无栈的核心崩溃。证据为 `artifacts/debug-native/late-gap-proof.json`、`gap-before.xml` 与 `gap-after.xml`。

- 最终普通版与独立 ASan/UBSan 版均 **247+127+22=396 项**通过，核心分别耗时 **250.124 秒**和 **252.810 秒**；无 sanitizer 诊断，LeakSanitizer 仍关闭。
- 两版各连续通过 **30 轮 × 3 个有限示例 = 90 次**，共 180 次；均使用两个独立进程、自然退出，无示例共享目录残留。
- 检查器还验证了退出非零、缺失结果和超时三个失败分支。超时即判失败并清理自身进程组，不把收到输出后强制停止算作成功。
- 当前新增可执行生产行覆盖率为 **1739/1841（94.46%）**，固定基线不变，合并 137 个翻译单元；包括本阶段完整测试及有限示例计数，不是分支覆盖率。
- 两个安装前缀、11 个变更公开头文件、运行库路径、源码清单和补丁均可核对；完整汇总为 `artifacts/local-loan/results/verification-finite.json`。上一阶段源清单、补丁、覆盖率与原始检查结果归档于 `results/keyed-stage/`。修改仍未提交、未推送。

全部八项交付仍有剩余：完整持久性/故障/并发与匹配矩阵、注销与 deadline/lifespan 的交互、Reader 已知实例回收、ON 映射失败及恢复语义、openEuler 和性能验收，以及随最终行为更新文档。此前普通完整核心运行在最终 WaitSet 用例的无栈 SIGSEGV 仍未确定根因；本阶段两套完整回归通过不等于已解释那次崩溃。


## Deadline/lifespan 生命周期与首次心跳缺口（2026-10-27，本阶段通过）

本阶段继续在当前 Ubuntu 机器单线程编译。修正 deadline 查询与调度：未启用监视的实例不参与最早期限选择，更新期限不能插入未知句柄；回调按实际到期时间处理，跳过空 history 和已被后续写入更新的旧唤醒。dispose 暂停监视，成功 ALIVE 写入/接收恢复；Writer unregister 成功后删除注册，旧回调不复活实例、不占用 max_instances。Listener 在调度锁外通知，允许回调注销自己的实例。Reader 仅在接收准入和 lifespan 检查通过后刷新 deadline。

真实 lifespan 过期处理加 unregister 后，应用持有的 Reader Loan 仍可访问原样本，并阻止槽位被复用，直至归还。该测试与原生 Reader 可靠/尽力准入、dispose/恢复等用例一起，覆盖本轮生命周期修正；不将其扩展为完整时间戳/并发 QoS 验收。

扩展测试还抓到一个发现序号问题：SEDP Writer 旧公告/注销被移除，只剩序号 3；Reader 已收缓存，但首次 HEARTBEAT 的 firstSN=3 未推进旧缺口，导致公告没有提交。现在首次提交前也执行 LostChangesUpdate，同时保留实际收到的旧数据。3 个新增确定性回归在旧库上 2 项失败，修复后通过。

- 最终普通与独立 ASan/UBSan 版均 **247+140+22=409 项**通过，核心分别耗时 **255.087 秒**和 **258.101 秒**。无检查器诊断，LeakSanitizer 仍因本机 ptrace 限制关闭。
- 两版各通过 **30 轮 × 18 个针对性测试 = 540 次**，并各通过 **30 轮 × 3 个有限示例 = 90 次自然退出**。重复次数不计作新增独立用例数。
- 新增可执行生产行覆盖率为 **1784/1885（94.64%）**，固定基线不变，合并 137 个翻译单元；不是分支覆盖率，也不是项目完成比例。
- 此次独立检查版核心库依赖重编耗时 213.719 秒，最大单个编译子进程 RSS 为 829340 KiB；不含后续测试/示例编译，不是从空目录的完整构建，也不是整机峰值。

保留旧 deadline 逻辑的 6/6 失败与初次崩溃记录、首次心跳回归的 4 项中 2 项失败、发现现场及修复后 18 项通过证据。第一次完整核心为 245/247：两个旧状态测试未写入/接收任何样本就期待 deadline，且非致命大小断言失败后仍索引空条件序列。现改为先验证空端点不误报，再真实 write/receive 后用定时器和 WaitSet 验证超时；增加致命边界检查和清理保护。这不证明此前无栈 WaitSet SIGSEGV 的原因。详情见 `artifacts/debug-native/deadline-lifecycle-proof.json`。

当前汇总为 `artifacts/local-loan/results/verification-deadline.json`，普通结果为 `*-deadline-final.xml`，独立检查、重复示例与覆盖率在 `artifacts/local-loan-sanitize/`。源清单、完整补丁、两版安装头文件及运行库路径均可核对；上一阶段证据保存在 `results/finite-stage/`。修改仍未提交、未推送。

全部八项交付仍有剩余：完整持久性/故障/并发与 QoS 匹配矩阵、Reader 已知实例回收、ON 映射失败与恢复语义、openEuler 和性能验收。deadline 的并发 set_qos/定时器替换、定时器内部同步、仅注册尚未写入的语义，以及多个 Writer 共享同一实例时的注销/监视交互也需单独验证。原有普通完整核心的无栈 WaitSet SIGSEGV 仍未确定根因。


## 多 Writer 共享实例的 deadline 生命周期（2026-10-27，本阶段通过）

两个新增确定性回归在前一阶段已验证的库上均失败：SHARED 所有权下，一个 Writer 发出 unregister 就停掉整个实例的 requested deadline；最后一个 Writer 消失时，实例却保留活动 deadline。现在 Reader 按 history 中的实际实例状态更新期限：ALIVE 样本刷新，生命周期控制在实例非 ALIVE 时暂停；若其他 Writer 仍使实例存活，则保留原期限，不把 unregister 当成新数据刷新。RemoveRemoteWriter 在移除最后一个 Writer 时清除该实例期限。

回归在可靠和尽力 Reader 下分别验证两个 Writer 对同一键的 ALIVE、一个 Writer 退出后原期限保持、到期通知，以及最后一个退出后不再通知。样本来自独立的真实共享池，经已匹配端点的 SubmitDataChange 提交；注销通过控制缓存提交，Writer 消失直接调用 history 的 RemoveRemoteWriter。它们验证接收/状态逻辑，不代替独立进程崩溃、发现租约或重启的故障验收。

- 最终普通与独立 ASan/UBSan 版各 **247+142+22=411 项**通过，核心耗时分别 **254.446 秒**和 **257.4 秒**；无检查器诊断，仍关闭 LeakSanitizer。
- 两版各 **30 轮 × 20 个针对性测试 = 600 次**通过；三个有限示例各再通过 30 轮，共每版 90 次自然退出。
- 固定基线新增可执行生产行覆盖率为 **1794/1897（94.57%）**，合并 137 个翻译单元；仍是行覆盖率，不是分支覆盖率或项目完成比例。

首次完整核心为 246/247：旧 QoS deadline 测试删除唯一 Writer 后仍期待 Reader 的 requested deadline。现增加匹配等待和有界 promise/future，保持 Writer 存活直到 Reader 实际超时回调及断言完成，并将样本值初始化。保留该次完整失败日志/XML；未删减状态计数断言。

证据为 `artifacts/debug-native/multiwriter-deadline-proof.json`、`artifacts/local-loan/results/verification-multiwriter.json` 与 `*-multiwriter-final.xml`，独立检查及覆盖率在 `artifacts/local-loan-sanitize/`。前一阶段完整源码/补丁、覆盖率和检查报告已归档到 `results/deadline-stage/`；代码仍未提交、未推送。

全部八项交付仍有剩余：独立进程持久性/故障/重启、多 Topic/多 Reader/多 Writer 并发与资源矩阵、Reader 实例回收、ON 映射失败/恢复、openEuler 和性能验收。定时器内部同步、并发 set_qos/替换与完整时序语义，以及普通 CDR/native 的 source timestamp/lifespan 一致性仍需验证。先前无栈 WaitSet SIGSEGV 的根因仍未确定。


## Deadline 到期与实例状态变化的交错（2026-10-27，本阶段通过）

新增确定性回归构造定时器已读到到期实例、接收路径随后处理 dispose/unregister 或最后一个 Writer 消失、旧回调再尝试更新期限的顺序。旧库的同一用例在四种状态变化中均会重新启用已非 ALIVE 的实例；修复前 1 项失败，修复后连同原有 20 项针对性测试全部通过。

HistoryCache 现在在同一把锁内检查当前实例状态并推进 requested deadline，拒绝更新非 ALIVE 实例并清除旧期限。Reader 回调在推进失败后仍重新扫描其他活动实例并安排下一期限，避免无效回调导致其他实例延迟。实例注册及键快照继续保留，未把停止监视等同于删除实例。

- 普通与独立 ASan/UBSan 版各 **247+143+22=412 项**通过，核心分别耗时 **254.299 秒**和 **257.036 秒**；无检查器诊断，LeakSanitizer 仍关闭。
- 两版各通过 **30 轮 × 21 个针对性测试 = 630 次**，并各通过三个有限示例各 30 轮，共每版 90 次自然退出。
- 固定基线新增可执行生产行覆盖率 **1799/1900（94.68%）**，合并 137 个翻译单元；不是分支覆盖率或项目完成比例。

该回归使用公开的内部 Reader 接收准入和 history 操作，确定性模拟允许的交错；它不是实际并发调度或 TSan 证明。最初的测试私有方法访问未编译通过，随后旧二进制零测试匹配的尝试已另存，未计入结果；正式修复前报告严格验证 1 项实际执行并失败。证据见 `artifacts/debug-native/expiry-interleave-proof.json` 与 `artifacts/local-loan/results/verification-expiry.json`。前一阶段源码/补丁、覆盖率及普通/检查版结果归档于 `results/multiwriter-stage/`。修改仍未提交、未推送。

全部八项目标仍有剩余：独立进程持久性/故障/重启、多端点/资源和 QoS 矩阵、Reader 已知实例容量与回收策略、ON 映射失败/恢复、openEuler 和性能验收。Timer 内部同步、并发 set_qos/替换和完整时间戳/lifespan 一致性仍需单独验证；历史无栈 WaitSet SIGSEGV 未确定根因。openEuler 官方根目录仅完成下载校验、解包和基本用户态探测，尚不构成项目的平台通过证据。
