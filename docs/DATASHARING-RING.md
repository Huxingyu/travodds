# 共享发布历史环与载荷写入边界

本轮完整证据与最终审计已通过，范围与证明边界见下文。文档单独提交，不改变受测代码身份；之后源码/测试/构建工具再变化，受影响证据必须重跑。本报告不声称文档已提交或分支已推送；普通推送 origin 的最终 HEAD 以交付记录为准，不 force、不推 upstream/gitee、不提 PR。

最终受测代码提交 `f32da8390b0af07500f67eb1d0a1279a9c968f4f`，共享布局 **v7 / discovery v5**；`5609` 项源码/测试/依赖/构建工具输入的同一规范身份为 `65a3cde4be51c42897dfd974a2eea661e4c74b06abd1d2a5d40a18810da82861`。本次实测范围：plain、coverage 普通全量各 **555 = 259 模块 + 49 共享池 + 247 core**；ASan/UBSan、TSan 各 **308 = 259 + 49**（不含 core）；有限示例 **9 次**（Loan/keyed/mixed 各 3）；覆盖率 **95.556%（3096/3240，144 个翻译单元）**。当前入口为 [11/1 报告](DATASHARING-PROGRESS-20261101.md)和[验收矩阵](DATASHARING-ACCEPTANCE.md)，旧 542/99/99 和 94.132% 不作为新源码通过证据。

## 官方原文与验收范围

项目 261760062 的 programDesc 将方案描述为“按Writer GUID命名的共享环形历史段”，并在不足和改进中分别写明“DataSharingPayloadPool（环形段历史）”与“DataSharingPayloadPool（Writer命名环形段）”。因此需要真实的有界共享历史环；有限槽位的轮转扫描和环形空闲分配器本身不充分。

原文功能目标是“同主机plain/bounded类型数据读写全程物理内存仅写入1次，无任何用户态拷贝”。outputRequirement 第 5 项同时明确“支持跳过序列化/反序列化的零拷贝分支，同时保留RTPS降级回退机制”，第 7 项要求 Loan Sample 写发布与 Loaned Sample 读订阅示例。严格零拷贝验收因而针对双方兼容、实际选中的本机 native Writer Loan → Reader Loan 分支；普通 write、预分配复制输出和 RTPS 继续提供兼容行为。

原文留存于 10/31 历史证据根 `artifacts/strict-ring-20261031/requirements/` 的 official-programDesc.html、official-outputRequirement.json 和 acceptance-plan.json；来源是 10/29 已保存的官方响应，本轮没有重新联网取文。上述引文与后述可执行不变量对应，不把旧“固定槽位池”文档的措辞当作新实现通过证据。

## 分配位置与发布历史是两套顺序

| 结构 | 管理内容 | 顺序依据 |
|---|---|---|
| reservation 环 | 下一次尝试借出的固定共享槽位；Header.ring_tail 为不回绕的绝对位置，Slot.ring_position 记录归属 | 借出时的 reservation 位置，物理候选槽位对容量取模 |
| published history 环 | 已成功发布、尚被历史或投递/借阅引用保活的节点 | 实际 DDS publish sequence，由 history_head/tail/count 和 Slot 的 previous/next/linked 连接 |

例如先借出 A、再借出 B，应用先发布 B、再发布 A，历史必须按 B 的发布序列、A 的发布序列遍历。不能因 reservation 顺序先遇到 A 就阻塞已经成功发布的 B，也不能为了排列历史而移动载荷。环链接和固定大小控制元数据可以改变，槽位载荷的 backing extent 保持不变。

published history 是闭合双向环。非空时 tail.next=head、head.previous=tail；单节点的前后链接均指向自身。空环 head=tail=NO_SLOT、count=0。正常读取遍历恰好 count 个节点后回到 head，并验证最后节点为 tail；非空环不以 NO_SLOT 终止。

## 写侧事务与发布线性化

结构变更由 Publication 的 try-CAS lane 串行化；竞争方及时失败或按既有有限等待策略重试，不因管理 mutex 的持有者暂停而等待它释放。该 lane 只约束结构修改，不能据此宣称整个 DDS write 路径形式化 lock-free。

history_epoch 为 64 位结构版本：偶数表示可供读者验证的稳定快照，奇数表示一次 append/unlink 事务进行中。写侧取得 Publication lane 后把 epoch 变为奇数，更新相关原子链接、head/tail/count、节点阶段和发布水位，再以 release 操作提交新的偶数 epoch。Reader 用 acquire 读取 epoch，在遍历前后核对同一个偶数值。

需要区分数据发布与稳定环快照提交：先完成载荷/metadata、pending 和 PUBLISHED 阶段，再更新 last_sequence 发布水位，随后才提交偶数 epoch。Reader 只选择不超过该水位的已发布节点；last_sequence 更新是对 Reader 可见的数据发布点，偶数 epoch 是稳定环结构的提交点。通知用于提示重检谓词，不充当早于载荷完成的发布点。先看到奇数 epoch 的读者可能在安全 fallback 中取得已独立发布的 pending 节点；因此不能把“遍历时遇到奇数”误判成段已损坏。

回收已链接槽位时，先持有 Publication lane 并进入奇数 epoch，再取得槽位冻结权并复查全部 pin；成功 CAS 推进 reservation 尾位置后执行 unlink，随后才重置可重用的控制字段。稳定偶数快照不能合法包含一个已链接的 FROZEN 节点。正常失败路径必须恢复完整状态，不能留下丢失链接或已被错误消费的 reservation；若修改者异常死亡在奇数事务中，按后述故障路径处理，不能伪造一次正常偶数提交。

## 必须始终成立的不变量

1. 每个已链接节点只出现一次，count 不超过容量；previous/next 双向一致，首尾闭合。空环状态三字段一致。
2. 从 head 开始按 count 个节点观察到严格递增的 DDS publish sequence；跨越 tail 回到 head 才允许序列从最新回到最旧。Reader 的 after-sequence 与 TRANSIENT_LOCAL 回放使用这条历史。
3. 未发布的借出不在 published history 中。discard_loan 不发布数据；取消、失败和重试不制造虚假历史项或破坏其他已发布节点。
4. Writer 引用、未读 pending、Reader Loan pin 及短操作 gate 共同约束复用。仍有任一保活引用时不能覆盖载荷；节点移出历史也不自动取消独立应用 Loan 的保活。
5. token 仍绑定 session、slot 和 generation。旧 token 在回绕后不能释放、发布或访问新一代样本；generation、绝对 reservation 位置与 epoch 达到边界时拒绝继续变更，不允许 64 位回绕隐藏 ABA。
6. 稳定偶数 epoch 下发现越界、重复、断链、反向链接不一致、count/首尾不符或非递增序列，报告 CORRUPT，不用扫描结果把结构损坏伪装为正常环。
7. 发生竞争时可以重试或明确返回耗尽/超时/关闭；不能通过复制载荷、覆盖慢 Reader 或先丢失原 history 再尝试分配来制造成功。

## 正常遍历、回放与故障 fallback

正常 Reader 与晚加入回放沿 published history 环遍历；扫描所有槽位、选取最小 sequence 只保留为 epoch 不稳定、修改者暂停或死亡时的安全 fallback。不能使 fallback 成为稳定正常路径，再把未使用的环字段作为“环形历史已实现”的证据。

读者在结构读取前后确认 epoch；发生变化或观察到奇数时，丢弃该次结构快照。fallback 仍须重新校验 slot gate、session/generation、PUBLISHED、pending/loan 状态与读取水位，只借用已经完成发布的节点，不访问正在构造的载荷。管理者死亡或元数据无法安全归属时延续 CORRUPT/新会话恢复规则，不能自行猜测并清除未知 pin。

回放快照只做原子元数据遍历和 epoch 验证，不给整条链建立 lease；注册先选择水位内仍由 Writer 保留的候选，再建立 SlotLease，复查代次/阶段/水位及 retained 状态后 pin。忙 retained 节点保持 ready=0，不能跳过保护；已消费且无 Writer/pending/Loan 引用的链上空闲节点不因无关回放 lease 阻塞零超时 loan。ready 前完成历史引用，独立应用 Loan、Writer 关闭和 Reader 删除的映射保活规则不变；多个 Writer 不要求全局 FIFO。

## “只写一次”的软件命题与独立证据

可由 DDS 实现保证并验证的是：借出开始对象寿命而不初始化载荷；应用直接在最终共享槽位填写样本；随后 publish、通知、history 准入、read/take 与 return 不再写入该载荷，也不另建完整样本副本或进行 CDR 序列化/反序列化。应用可选择重复写字段，库提供“一次直接填写即可发布”的通路；受控验收样例使用单遍填写。

Writer 与 Reader 的虚拟地址可以不同。独立 exec 进程必须从各自实际 Loan 地址定位 `/proc/self/maps` 的完整区间，以 `VMA.file_offset + (loan_address - VMA.start)` 得到文件偏移，核对 device major/minor、inode、偏移、长度及 MAP_SHARED。路径名、相同 checksum 或进程内指针相等不足以替代这一证据。

新共享布局 v7 将载荷区起点向上按宿主页大小对齐，使控制元数据不与首个载荷页重叠；仅该页对齐 padding 最多增加 page_size-1 字节，新增环字段另按实际 Header/Slot 大小计入预算。两者都必须纳入 segment_bytes 和 max_segment_bytes 的溢出/预算检查。槽位 stride 仍按类型对齐取整，其他槽位可能共享一个载荷页；因此生产保护覆盖整个载荷 span，而非单个槽位页。

Reader 侧 SharedPayloadPool::open 保留一个 Boost mapped_region，在完成布局、文件和 header 校验后，对从载荷起点到载荷区末尾的全部页设置 PROT_READ；控制元数据页仍为 RW，供注册、pending 和 Loan pin 使用。Writer 侧 create 的映射仍为 RW，供应用直接填写 Loan。Reader 载荷保护失败必须拒绝该共享映射，不能悄悄留下可写载荷。发现能力同步隔离新旧布局，具体版本与结果在冻结清单中核对。

独立保护工具在上述生产只读保护之上使用整页隔离样本：无键样本在框架执行窗口对整个载荷设置 PROT_NONE；带键样本只开放单独 key 页为只读，其余 body 页为 PROT_NONE。应用读写通过受控窗口完成；常规 mprotect、文件映射和 mremap 受 seccomp TSYNC 限制，另以拒绝探针证明封锁生效。所有同 backing extent 的别名映射均须纳入保护，不能只保护返回给应用的一个虚拟地址。PROT_NONE 与受控窗口属于测试取证机制，生产 Reader 的保护是 PROT_READ，生产 Writer 的映射不是只读。

PROT_NONE 除了阻止写入，也阻止框架从该载荷读取后复制到另一个用户态缓冲。原始 XML、日志中的逐例 proof 记录、映射信息、保护状态、seccomp 拒绝探针和 TypeSupport 回调计数须一起审计；仅有一个最终 passed 字段不足。保护窗口和允许的应用窗口必须与实际 publish/read/return 阶段对应，带键 body 不能借 key 页开放而被读取。

已有文件描述符的 read/pread/write/pwrite 等数据 IO 可以绕过用户映射权限。因此还需审计所选生产稳态路径不存在通过该类 IO 搬运载荷，明确测试不是针对任意恶意代码的内存安全沙箱。固定大小的键、通知、索引和控制元数据可以复制，应列出范围，不扩大为“任何字节均不复制”。

这组不变量落实原文避免用户缓冲、CDR 和接收输出之间完整载荷搬运的目标。它不等价于 DRAM 总线恰好一次写事务：内核页清零、缓存写回、页迁移、ECC 及编译器 store 合并均不由 DDS API 控制。验收结论应使用“单次应用载荷初始化、零用户态载荷搬运”，同时保留原文引文和上述实际证据，不把硬件事务计数写成已测结果。

## 当前验证与边界

两个永久回归 ReplaySkipsUnretainedBusyHistoryButProtectsRetainedPins、ReplayDefersBusyRetainedNodeWithoutAddingAnUnprotectedPin 已进入本次共享池程序。旧 followups/replay/ 的确定性红/绿调度只证明 c78607d 的根因和最小修复；新最终身份的结果仍为 通过（限本页所述范围）。

本轮实际证据：[冻结输入](../../../artifacts/datasharing-final-20261101/round2/source-manifest.json)、[最终审计](../../../artifacts/datasharing-final-20261101/round2/audit/final-audit.json)、[statistics 原始结果](../../../artifacts/datasharing-final-20261101/statistics/attempt2/statistics-result/results.json)、[费用/用量记录](../../../artifacts/datasharing-final-20261101/usage-ledger.json)。受测实现 f32da8390b0af07500f67eb1d0a1279a9c968f4f，源码身份 65a3cde4be51c42897dfd974a2eea661e4c74b06abd1d2a5d40a18810da82861（5609 项输入）；plain/coverage 各 555 = 259 模块 + 49 共享池 + 247 core，ASan/UBSan 与 TSan 各 308（不含 core），9 次有限示例，新增生产可执行行覆盖率 95.556%（3096/3240，144 个翻译单元）。 审计核对四模式真实 XML/log、用例身份、同一源码、私有库 ELF/插桩、原始载荷 proof、覆盖率及资源清理；此前 Modal 独立复验和 followups 仅保留其原有历史身份。

本次 plain/coverage/asan/tsan 四模式均在 Modal 从同一最终源码重新编译生产库和相应测试程序，不复用旧对象、旧二进制或旧覆盖计数；Ubuntu 24.04、g++ 13.3，固定 CPU=2、memory=4096 MiB。冻结清单包含 thirdparty、GTest zip、实际编译源码/头文件和构建脚本，逐文件 SHA 与最终代码身份一起绑定；清单数量以实际审计为准。

各模式模块 259 项在 Modal 执行；完整共享池 49 项在本机真实内核运行，plain/coverage 的 core 247 项也在本机运行。ASan/UBSan 与 TSan 不包含 core。按预先约定分工补齐 gVisor 的 futex 观测、IPv6 和网络环境限制，回传 ELF/私有库与 Modal 产物 SHA 完全相同，用例身份无遗漏；原始环境失败记录保留，不能把真实竞争归为环境限制。

每模式 Modal 模块内四项 NativePayloadWrites 均留原始 proof；本机使用相同 ELF/私有库独立补跑四项，由 tools/ospp/native_payload_writes.py 重解析 XML/log/proof。重复身份仅增加真实内核证据，不额外计数。Loan/keyed/mixed 各 3 次有限示例在本机 coverage 模式自然退出，共 9 次，实际收发计数通过核对。

覆盖率实测 95.556%（3096/3240，144 个翻译单元），最低要求 70%。使用 -fprofile-update=atomic，Modal/local 各从空的新计数目录采集，合并相同编译身份的新 gcda，全部实际链接 gcno（包括无 gcda 的零命中单元）进入统计。tools/ospp/coverage.py 按固定基线 489067108405b27afbd369245542f5e88d457f18 重算新增生产可执行行；原始生产负计数 0 个，负计数均按未覆盖处理，未复用旧计数。这不是分支覆盖率或全仓库文本行覆盖率。

statistics() 通过非阻塞 trylock 获取控制面快照，合法并发持锁可返回 TIMEOUT。原测试在子 Reader listener 仍运行时直接要求关闭快照返回 OK，没有建立必要的静止前提，属于测试握手缺口。本轮在真实 statistics Guard 已成功持有真实共享 mutex 后仅暂停执行：子工作线程的 PID/TID、mutex owner TID 和解析到 readerdatasharing.cpp 的调用栈均匹配。旧测试原 ASSERT 确定性失败，退出 1、TIMEOUT；增加 D/C/L 握手，等待 Reader 删除返回并完成 listener join，同时保留首条应用 Loan，原 OK、writer_alive=false、pending=0、loan_references=1 精确断言全部通过，退出 0。仅修测试的同步前提，未改产品统计语义、未新增 sleep/重试或放宽断言。这证明一种真实 listener 的同类合法交错可以产生该失败；历史偶发轮没有记录持锁者，不能据此声称已经识别那个历史 holder。原全量约 8 轮 1 次失败、单独 Modal 200 次/本机 40 次未复现均仍为历史现象。见 [before](../../../artifacts/datasharing-final-20261101/statistics/attempt2/statistics-result/before/summary.json)、[after](../../../artifacts/datasharing-final-20261101/statistics/attempt2/statistics-result/after/summary.json)。

旧首次 get_matched_publication_data 返回 3（BAD_PARAMETER）时没有记录所选 peer GUID，根因未证实。现测试采用独立 exec 对的 Topic，并断言精确父 Writer GUID/handle，lookup 原断言保留；后续通过和一次带 GUID 诊断均不能证明最初失败来自旧端点。原失败仍在 [tsan-final 原始日志](../../../artifacts/datasharing-followups-20261101/tsan-final/logs/test-loan_part_1.log)。

性能数据绑定 0bbeaed 的既有 v7/v5 测量。本轮最终提交 f32da8390b0af07500f67eb1d0a1279a9c968f4f 增加 ReaderProxy 控制面锁后未重新测量性能；下述 64 B RTPS 吞吐 -5.61%/p99 0.94 ms、64 KiB RTPS 约 71 samples/s/p95/p99 约 200 ms 均为旧版本的实际测量，不是最终提交的实测成绩。这些历史差异不能归因于本次新增锁，也不能据此证明最终实现性能不变；本任务不优化 RTPS 性能，仅如实保留历史观测和未归因限制。 RTPS 64 B 吞吐相对同日旧程序 -5.61%、p99 0.94 ms，退化尚未定量归因；64 KiB RTPS 约 71 samples/s、p95/p99 约 200 ms 属既有行为，本次不优化。性能来自 2 核共享 VM 三轮短测，不是最大可持续吞吐、专机因果归因或 SLA。软件层“只写一次”验证应用在最终共享槽位单遍填写及框架不搬运完整载荷，不证明 DRAM 写入次数；键和控制元数据仍可复制。“无锁”限于通知及共享数据面不获取跨进程管理 mutex，不证明整个 DDS write 形式化 lock-free/wait-free。本机 OFF/mixed/UDP 回退没有真实跨主机验证。ASan 关闭 LSan，未做泄漏扫描，不能写“无泄漏”。

本机 jobs=1、nice=19，未新增 swap/本机支出，不在远端存放凭据，未再连接不可用 ECS。Modal 固定 2 CPU/4096 MiB、Ubuntu 24.04/g++ 13.3、无 GPU；本任务请求资源时长与下载量估计 $0.086696，含预留的保守估计 $0.488881，低于 $1 授权上限，已结算记录不含待运行预留；不是账单。最终审计确认 Modal app 已停止、/dev/shm 无本任务新增残留、旧 strict/portable 保护路径核验通过。

以下保留 10/31 严格补全历史原始结果、资源与复现命令，所有数字仅属于旧冻结身份，不是本次新 pipeline 的命令或通过结果。两个旧保护目录只读，历史段原数不改。

首次诊断归档 [attempt1](../../../artifacts/datasharing-final-20261101/statistics/attempt1/statistics-result/results.json) 为配置失败：源码包遗漏 cmake/，configure-library 退出 1，未构建/执行任何测试；不得计为测试失败或通过。补齐源码包的 attempt2 保留独立输入/工具/程序 SHA 与原始 before/after 证据；旧归档不覆盖。

首轮受测 556c918d7e9659e42c266c5c524d8f2c038fb0c0 的 TSan 在 ReaderProxy::RequestedChangesSet 写请求 vector 与异步 RequestedChanges 复制读取之间报告真实数据竞争。模块退出 66，用时 12.542 s、限时 180 s，timed_out=false；尚有子进程 [2511]，清理记录 killed_pids=[1065, 2511]。这不是超时或 gVisor 环境失败。旧私有容器原始 shm_added 保留一个 DS 段并判失败，不能改写为旧轮无残留。后续已确认旧私有容器所属 app 停止且任务数为 0；停止记录不等于现场验证原段已删除，原始残留仍作为失败事实保留，与成功轮最终无新增残留分开。产品最小修复单独提交后，本轮在新的源码身份上从零重跑全部四模式、示例、覆盖率及 proof；旧 plain/coverage/ASan 的成功和旧失败 TSan 都不能替代新提交的验收。详见 [原始 race 分析](../../../artifacts/datasharing-final-20261101/race/README.md)与[首次 TSan 原始结果](../../../artifacts/datasharing-final-20261101/remote/tsan/results.json)。

## 10/31 历史验收清单

| 范围 | 必需证据 | 状态 |
|---|---|---|
| 真发布历史环 | 共享布局与实际 Reader/replay 路径静态复核；结构快照、空/单节点、多次 wrap、乱序借出/发布 | 静态复核、普通版及两个检查版均通过 |
| pin/cancel/回收 | 慢 Reader、乱序归还、取消、KEEP_LAST/KEEP_ALL、晚加入回放及旧 token | 普通版及两个检查版通过 |
| 竞争与故障 | 稳定坏环拒绝、奇数 epoch/暂停 fallback、代次与 epoch 边界、关闭/死亡保活 | 普通版及两个检查版通过 |
| 载荷访问保护 | NativePayloadWrites 逐例 XML/log、同 backing、PROT_NONE/key-RO、seccomp 拒绝探针、回调计数 | 普通版及两个检查版通过 |
| 正式交付 | 新冻结身份下的普通回归、独立检查器、示例、覆盖率、完整审计与便携包 | 542/99/99、9 示例及 94.132%；精确身份已绑定 |

保护测试为 `test/unit_payloadpool/test_native_payload_writes.cpp`，原始记录核验工具为 `tools/ospp/native_payload_writes.py`。本轮外部审计位于 `artifacts/strict-ring-20261031/final-audit.py`；三轮均通过后生成的 final-bindings.json 绑定精确 542/99/99 数量、同一源码和元数据 SHA-256；草稿绑定不能产生最终通过结论。

用户授权本轮继续在当前 VM 串行推进：jobs=1、nice=19、ionice best-effort 7、编译子进程地址空间 768 MiB、无新增 swap 或支出；资源计划保留于同证据根 resource-plan.json。磁盘允许时保留对象以便增量构建，本轮不默认启用 --trim-build-cache。正式命令和实际资源退出记录随最终冻结结果保存。

## 10/31 历史原始结果与复现

原始结果分别为 [coverage-atomic-final](../../../artifacts/strict-ring-20261031/coverage-atomic-final/results.json)、[asan-atomic-final](../../../artifacts/strict-ring-20261031/asan-atomic-final/results.json) 和 [tsan-atomic-final](../../../artifacts/strict-ring-20261031/tsan-atomic-final/results.json)。每轮包含四个 NativePayloadWrites 的实际 XML、日志和严格核验摘要；final audit 重新解析原始 proof，重建 focused driver，逐项核对私有库/安装头文件/检查器符号，从原始 gcov JSON 重新合并覆盖率，并核对九个独立进程示例的完整计数。五个伪造或不完整证据负控均被核验工具拒绝，记录位于 parser-negative-controls/。

在匹配源码根目录执行，使用全新的空证据与构建目录；三个命令必须依次运行。每个编译子进程限制为 768 MiB，运行检查器不受编译地址空间限制：

```bash
DS_EVIDENCE=/path/to/new-evidence
ionice -c 2 -n 7 nice -n 19 python3 tools/ospp/production_lockfree_verify.py \
  --output "$DS_EVIDENCE/coverage-atomic-final" --build-dir "$DS_EVIDENCE/build-coverage" \
  --mode coverage --core --examples 3 --compiler-memory-mib 768

ionice -c 2 -n 7 nice -n 19 python3 tools/ospp/production_lockfree_verify.py \
  --output "$DS_EVIDENCE/asan-atomic-final" --build-dir "$DS_EVIDENCE/build-asan" \
  --mode asan --scope focused --compiler-memory-mib 768 \
  --focus-test KeyedNativeCaptureFailurePreservesLoanAndRollsBackRegistration \
  --focus-test KeyedNativeReaderCaptureFailureDoesNotConsumeInstanceCapacityOrRetainSource \
  --focus-test NativeReaderIntegration.KeyedNativeKeySnapshotSurvivesSingleSlotReuseAndWriterDeletion

ionice -c 2 -n 7 nice -n 19 python3 tools/ospp/production_lockfree_verify.py \
  --output "$DS_EVIDENCE/tsan-atomic-final" --build-dir "$DS_EVIDENCE/build-tsan" \
  --mode tsan --scope focused --compiler-memory-mib 768 \
  --focus-test KeyedNativeCaptureFailurePreservesLoanAndRollsBackRegistration \
  --focus-test KeyedNativeReaderCaptureFailureDoesNotConsumeInstanceCapacityOrRetainSource \
  --focus-test NativeReaderIntegration.KeyedNativeKeySnapshotSurvivesSingleSlotReuseAndWriterDeletion
```

上述检查器范围为完整共享池、无初始化/key-admission/Reader 普通池和四项真实 DDS focused，并非全部 542 项均运行于检查器。ASan/UBSan 使用 detect_leaks=0，未执行 LSan。首轮三个 harness 的功能检查通过，但首次综合审计发现非原子计数产生的三条负计数被旧收集器误算覆盖，因此该报告不作为最终覆盖率。修复正计数判定并启用 -fprofile-update=atomic 后，143 个旧 gcda 连同 notes/二进制/flags 已归档，计数清零后全量重建并重新采集；新数据中全部生产代码负计数为 0，三个新运行再以同一冻结身份通过。旧统计 94.101%、失败审计及完整诊断保留于 audit-attempt1-diagnostic/ 与 gcov-negative-attempt1/，不冒充当前结果。便携记录及源码包见 [交付索引](../../../artifacts/portable-evidence-20261031/native-v5-delivery.md)；便携校验验证记录哈希与源码一致性，不会重新执行测试或原机完整审计。
