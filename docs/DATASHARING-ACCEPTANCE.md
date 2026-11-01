# DataSharing 官方八项验收对照

本轮完整证据与最终审计已通过，范围与证明边界见下文。文档单独提交，不改变受测代码身份；之后源码/测试/构建工具再变化，受影响证据必须重跑。本报告不声称文档已提交或分支已推送；普通推送 origin 的最终 HEAD 以交付记录为准，不 force、不推 upstream/gitee、不提 PR。

本页按官方项目 **261760062** 的 programDesc/outputRequirement 核对：最终受测代码提交 `f32da8390b0af07500f67eb1d0a1279a9c968f4f`，共享布局 **v7 / discovery v5**；`5609` 项源码/测试/依赖/构建工具输入的同一规范身份为 `65a3cde4be51c42897dfd974a2eea661e4c74b06abd1d2a5d40a18810da82861`。plain、coverage 普通全量各 **555 = 259 模块 + 49 共享池 + 247 core**；ASan/UBSan、TSan 各 **308 = 259 + 49**（不含 core）；有限示例 **9 次**（Loan/keyed/mixed 各 3）。新增生产可执行行覆盖率为 **95.556%（3096/3240，144 个翻译单元）**。原始要求保留于 历史原文：`artifacts/strict-ring-20261031/requirements/acceptance-plan.json`（本机证据，不随仓库分发）。旧 ecb4b3c 的 542/99/99、9 示例、94.132%（2984/3170，143 TU）只证明当时冻结源码。

| 官方交付 | 当前实现及本次已核验的行为 | 当前状态 |
|---|---|---|
| 1. 实现 PayloadPool 接口 | IPayloadPool/ITopicPayloadPool、四种普通池、Topic 池共享、准入失败事务和分片生命周期；完整模块与 core 覆盖 | 通过（限本页所述范围） |
| 2. 实现 DataSharing 共享段管理 | Writer GUID/session 命名、Boost 映射、真实闭合发布历史环；按 retained 回放候选加 lease/pin，空闲节点不被无关回放占用；保活/关闭/代次与坏环拒绝 | 49 项共享池身份；通过（限本页所述范围） |
| 3. 实现跨进程通知 | 原子通知代次/futex、先采样再检查谓词、关闭后重扫；真实内核验证管理 mutex 暂停时共享进展 | 通过（限本页所述范围） |
| 4. 实现 DDS 层 Loan API | Writer loan/write/discard、Reader read/take/return、错误码和删除后的 Loan 映射保活；statistics 测试握手前提经确定性验证修复 | 通过（限本页所述范围） |
| 5. 零拷贝分支与 RTPS 回退 | no-init、native Loan、key-only、v5 协商与普通/混合回退；四项实际共享 backing/访问保护 proof，Modal 与本机分别留证；另有双主机 AUTO/ON × 两写法 × 三大小 × 两方向共 24 组 RTPS 回退通过 | 通过（限本页所述范围） |
| 6. DataSharingQosPolicy | AUTO/ON/OFF、目录/容量/预算、API/XML、类型与协议准入、不可变性和降级 | 通过（限本页所述范围） |
| 7. 单元测试 ≥70% 与 Loan/QoS 示例 | plain/coverage 各 555，ASan/UBSan 与 TSan 各 308，有限例 9；官方新增行覆盖率 95.556%（3096/3240，144 个翻译单元） | 通过（限本页所述范围） |
| 8. 设计与 API/QoS/调试手册 | [设计](DATASHARING-DESIGN.md)、[环](DATASHARING-RING.md)、[API](DATASHARING-API.md)、[QoS](DATASHARING-QOS.md)、[调试](DATASHARING-DEBUG.md)、[本次报告](DATASHARING-PROGRESS-20261101.md)及当前完整身份/交付绑定 | 通过（限本页所述范围） |

## 原文严格范围与证据边界

“按 Writer GUID 命名的共享环形历史段”由按 publish sequence 连接的共享闭合双链实现，reservation 顺序不等于 publish 顺序。正常 Reader 与 TRANSIENT_LOCAL 回放沿环，epoch 不稳定时有安全 fallback；稳定坏环返回 CORRUPT。本次继承 c78607d：确定性暂停真实回放 lease 证明旧版短暂占住已消费空闲节点，零超时 loan 误报 EXHAUSTED；修复先选择水位内 retained 节点再 lease/复查/pin，忙 retained 节点延后 ready，不削弱回放保护。红/绿证据位于旧 followups/replay/，两个永久回归已进入本次新测试程序。

原文“物理内存仅写入 1 次、无任何用户态拷贝”结合第 5 项“零拷贝分支及 RTPS 回退”验收兼容端点实际选择的 native Writer Loan → Reader Loan：应用单遍填写最终共享槽位；device/inode/file offset/MAP_SHARED、PROT_NONE/key-RO、所有别名保护、故障负控、seccomp 拒绝探针和完整载荷回调共同验证框架窗口。普通 write、预分配输出和 RTPS 保留复制语义；证据不构成恶意进程安全沙箱，也不免除 TypeSupport 类型契约。Ubuntu/openEuler 为原文的“或”，自动 IDL native 生成器不作为额外要求。

旧首次 get_matched_publication_data 返回 3（BAD_PARAMETER）时没有记录所选 peer GUID，根因未证实。现测试采用独立 exec 对的 Topic，并断言精确父 Writer GUID/handle，lookup 原断言保留；后续通过和一次带 GUID 诊断均不能证明最初失败来自旧端点。原失败仍在 tsan-final 原始日志：`artifacts/datasharing-followups-20261101/tsan-final/logs/test-loan_part_1.log`（本机证据，不随仓库分发）。

最终代码 `f32da8390b0af07500f67eb1d0a1279a9c968f4f` 已按原方法在本机完成连续 `0bbeaed → f32da83 → 0bbeaed` 性能重测：三组各 27/27，共 81 次自然退出，载荷/CDR 与原始日志核验通过，无失败或环境重跑。64 B RTPS 吞吐 28292.30 samples/s（相对基线1/2 -2.47% / +1.07%）及 p99 271.738 μs 均位于双基线中位区间内，未超出本次噪声；p50/p95 有改善观测。4 KiB ordinary 吞吐 -7.13% / -11.49%、p99 +141.02% / +204.07%，超出双基线差幅，必须保留退化观测，具体原因尚未查实。36 个指标有 7 个改善、2 个退化观测超出预设噪声规则，但组内范围交叠且宿主 CPU/I/O/换页不同，不能将差异归因新增 ReaderProxy 锁或声称统计显著、整体没有影响。64 KiB RTPS 目标约 70.97 samples/s、p95/p99 约 198/200 ms，与既有行为相符；两基线吞吐本身差幅 60.36%。[完整性能报告](DATASHARING-PERFORMANCE.md)和[本次三组证据](validation/20261101/performance/attempt2/REPORT.md)包含逐组数据、噪声规则、RSS及宿主条件。前次监控误杀的 9/27 部分结果仅作旧现象诊断，未用于本轮比较，原证据和 `553ac93` 保留。旧轮 64 B RTPS 吞吐 -5.61%/p99 0.94 ms 仍是历史未归因观测，不改写为当前成绩；本次不做性能优化。软件层“只写一次”不证明 DRAM 写入次数；键和控制元数据仍可复制。“无锁”限于通知及共享数据面，不证明整个 DDS write 形式化 lock-free/wait-free。AUTO/ON 同 kind 的真实跨主机 RTPS/UDP 回退已完成下述 24 组验证；跨主机 OFF/mixed 组合仍未实测；ASan 关闭 LSan，未做泄漏扫描。性能为两核共享 VM 短测，不是 SLA。

## 跨主机 RTPS 回退补充（2026-11-01）

在同一受测实现 `f32da8390b0af07500f67eb1d0a1279a9c968f4f` 上，VM（Linux 6.8）与 `openlet-eval4090` 的 Kubernetes 容器（另一主机的 Linux 5.15）完成双向实测，双方 boot_id 不同、均为 x86_64。两端使用同字节独立收发程序和私有库，库 SHA-256 为 `7ed3c24c732b236a4a89ec0aa44dfae0778487f3a5795e1dfbab1f57fc44dda6`。VM 的 TAP 独立网络命名空间与远端 `unshare -Urn` 内 veth/AF_PACKET 经 SSH stdin/stdout 传送以太网帧；这是 **SSH 承载的虚拟二层，不是物理局域网**。RTPS 仍使用 UDP，SPDP 仍通过 `239.255.0.1` 发现，未修改产品代码或添加静态对端；收发器显式移除传统 SHM transport，以隔离 RTPS/UDP 路径，DataSharing 仍设 AUTO/ON。先行单播及组播双向 echo 均通过；程序 peer IP 仅用于应用控制握手。

同机 AUTO/native 控制组为 64 条：Writer 序列化 0、Reader 反序列化 0、Reader Loan 64、完整验证 64。跨主机矩阵为两端同设 AUTO/ON × ordinary/Writer Loan × 64/4096/65536 B × 两种 Writer 方向，**24/24 组通过**，每组正式 64 条：序列化 64、反序列化 64、Reader Loan 0、完整载荷验证 64；共 1536 条跨主机样本。控制组及跨主机共 50 个进程均自然退出 0，没有组重跑或失败后重写。每组另有一条不计入正式计数的握手样本；所有 loan/write 均单次检查返回码，native 控制先只读观察精确 Reader GUID 注册就绪。独立审核逐一解析 50 份原始日志并重核矩阵、计数、boot_id、SHA 和退出码。

远端只用 nice 19、1 核，未安装软件或访问 GPU，未改 OpenLET 文件/配置、未向其 PID 发送信号。已记录 OpenLET 前后 PID：后测仍观察到原控制器，随后记录到新评测控制器/worker，不能声称所有旧 PID 全程不变，也不以进程记录证明评测成绩相同。SSH/bridge、VM 命名空间和 TAP、远端临时目录均已清理，两端 `/dev/shm` 无新增残留。完整证据见 [跨主机报告](validation/20261101/crosshost/eval4090/REPORT.md)、[逐组结果](validation/20261101/crosshost/eval4090/results.json)、[独立日志审核](validation/20261101/crosshost/eval4090/results-independent-review.json)和[清理](validation/20261101/crosshost/eval4090/cleanup.json)。本轮不作性能结论；未覆盖物理局域网、跨主机 OFF/mixed 组合、其他发行版或所有故障场景。该次实测未改生产代码、测试和性能文档；跨主机文档提交 `efe78ca` 已推送到 `origin/work/datasharing-progress`，不改变受测实现 `f32da83`。

## 新证据的执行分工与绑定

本轮实际证据：[冻结输入](validation/20261101/final/round2/source-manifest.json)、[最终审计](validation/20261101/final/round2/audit/final-audit.json)、[statistics 原始结果](validation/20261101/final/statistics/attempt2/statistics-result/results.json)、[费用/用量记录](validation/20261101/final/usage-ledger.json)。受测实现 f32da8390b0af07500f67eb1d0a1279a9c968f4f，源码身份 65a3cde4be51c42897dfd974a2eea661e4c74b06abd1d2a5d40a18810da82861（5609 项输入）；plain/coverage 各 555 = 259 模块 + 49 共享池 + 247 core，ASan/UBSan 与 TSan 各 308（不含 core），9 次有限示例，新增生产可执行行覆盖率 95.556%（3096/3240，144 个翻译单元）。 审计核对四模式真实 XML/log、用例身份、同一源码、私有库 ELF/插桩、原始载荷 proof、覆盖率及资源清理；此前 Modal 独立复验和 followups 仅保留其原有历史身份。

本次 plain/coverage/asan/tsan 四模式均在 Modal 从同一最终源码重新编译生产库和相应测试程序，不复用旧对象、旧二进制或旧覆盖计数；Ubuntu 24.04、g++ 13.3，固定 CPU=2、memory=4096 MiB。冻结清单包含 thirdparty、GTest zip、实际编译源码/头文件和构建脚本，逐文件 SHA 与最终代码身份一起绑定；清单数量以实际审计为准。

各模式模块 259 项在 Modal 执行；完整共享池 49 项在本机真实内核运行，plain/coverage 的 core 247 项也在本机运行。ASan/UBSan 与 TSan 不包含 core。按预先约定分工补齐 gVisor 的 futex 观测、IPv6 和网络环境限制，回传 ELF/私有库与 Modal 产物 SHA 完全相同，用例身份无遗漏；原始环境失败记录保留，不能把真实竞争归为环境限制。

每模式 Modal 模块内四项 NativePayloadWrites 均留原始 proof；本机使用相同 ELF/私有库独立补跑四项，由 tools/ospp/native_payload_writes.py 重解析 XML/log/proof。重复身份仅增加真实内核证据，不额外计数。Loan/keyed/mixed 各 3 次有限示例在本机 coverage 模式自然退出，共 9 次，实际收发计数通过核对。

覆盖率实测 95.556%（3096/3240，144 个翻译单元），最低要求 70%。使用 -fprofile-update=atomic，Modal/local 各从空的新计数目录采集，合并相同编译身份的新 gcda，全部实际链接 gcno（包括无 gcda 的零命中单元）进入统计。tools/ospp/coverage.py 按固定基线 489067108405b27afbd369245542f5e88d457f18 重算新增生产可执行行；原始生产负计数 0 个，负计数均按未覆盖处理，未复用旧计数。这不是分支覆盖率或全仓库文本行覆盖率。

statistics() 通过非阻塞 trylock 获取控制面快照，合法并发持锁可返回 TIMEOUT。原测试在子 Reader listener 仍运行时直接要求关闭快照返回 OK，没有建立必要的静止前提，属于测试握手缺口。本轮在真实 statistics Guard 已成功持有真实共享 mutex 后仅暂停执行：子工作线程的 PID/TID、mutex owner TID 和解析到 readerdatasharing.cpp 的调用栈均匹配。旧测试原 ASSERT 确定性失败，退出 1、TIMEOUT；增加 D/C/L 握手，等待 Reader 删除返回并完成 listener join，同时保留首条应用 Loan，原 OK、writer_alive=false、pending=0、loan_references=1 精确断言全部通过，退出 0。仅修测试的同步前提，未改产品统计语义、未新增 sleep/重试或放宽断言。这证明一种真实 listener 的同类合法交错可以产生该失败；历史偶发轮没有记录持锁者，不能据此声称已经识别那个历史 holder。原全量约 8 轮 1 次失败、单独 Modal 200 次/本机 40 次未复现均仍为历史现象。见 before：`artifacts/datasharing-final-20261101/statistics/attempt2/statistics-result/before/summary.json`（本机证据，不随仓库分发）、after：`artifacts/datasharing-final-20261101/statistics/attempt2/statistics-result/after/summary.json`（本机证据，不随仓库分发）。

本机 jobs=1、nice=19，未新增 swap/本机支出，不在远端存放凭据，未再连接不可用 ECS。Modal 固定 2 CPU/4096 MiB、Ubuntu 24.04/g++ 13.3、无 GPU；本任务请求资源时长与下载量估计 $0.086696，含预留的保守估计 $0.488881，低于 $1 授权上限，已结算记录不含待运行预留；不是账单。最终审计确认 Modal app 已停止、/dev/shm 无本任务新增残留、旧 strict/portable 保护路径核验通过。

`artifacts/strict-ring-20261031/` 与 `artifacts/portable-evidence-20261031/` 只读，现有文件内容与历史数字不修改。旧 0bbeaed 的 Modal 独立复验、followups 的首轮成功/失败/停止记录保持各自身份，不能合并冒充本次成功。真正竞争必须诊断，gVisor 限制不授权跳过用例。最终审计须把同一源码、用例身份、模式插桩及私有链接串起来；便携包哈希校验不等于重新执行测试。

首次诊断归档 attempt1：`artifacts/datasharing-final-20261101/statistics/attempt1/statistics-result/results.json`（本机证据，不随仓库分发） 为配置失败：源码包遗漏 cmake/，configure-library 退出 1，未构建/执行任何测试；不得计为测试失败或通过。补齐源码包的 attempt2 保留独立输入/工具/程序 SHA 与原始 before/after 证据；旧归档不覆盖。

首轮受测 556c918d7e9659e42c266c5c524d8f2c038fb0c0 的 TSan 在 ReaderProxy::RequestedChangesSet 写请求 vector 与异步 RequestedChanges 复制读取之间报告真实数据竞争。模块退出 66，用时 12.542 s、限时 180 s，timed_out=false；尚有子进程 [2511]，清理记录 killed_pids=[1065, 2511]。这不是超时或 gVisor 环境失败。旧私有容器原始 shm_added 保留一个 DS 段并判失败，不能改写为旧轮无残留。后续已确认旧私有容器所属 app 停止且任务数为 0；停止记录不等于现场验证原段已删除，原始残留仍作为失败事实保留，与成功轮最终无新增残留分开。产品最小修复单独提交后，本轮在新的源码身份上从零重跑全部四模式、示例、覆盖率及 proof；旧 plain/coverage/ASan 的成功和旧失败 TSan 都不能替代新提交的验收。详见 原始 race 分析：`artifacts/datasharing-final-20261101/race/README.md`（本机证据，不随仓库分发）与首次 TSan 原始结果：`artifacts/datasharing-final-20261101/remote/tsan/results.json`（本机证据，不随仓库分发）。

## 历史验收记录（2026-10-28，布局 v5）

以下修复、完整阶段命令及 94.118% 覆盖率对应 `artifacts/completion-20261028/`，由其 `commands.jsonl`、`frozen-source.json` 和 `verification.json` 绑定。它们保留可复现的历史，不替代布局 v6 和 key-only 最终代码验证。

| 2026-10-28 历史正确性问题 | 回归与关键断言 |
|---|---|
| 过期准入 | `KeyedReviewExpiredAdmissionForNativeAndCdrBothReliabilities`：native/CDR×两种可靠性均不入history/不占实例。修复前失败XML保留。 |
| 错误read/take消费history | `ReviewRejectedSequencePreservesSampleAcrossAllVariants`覆盖10种序列变体；`ReviewCopyAllocationAndDeserializeFailuresPreserveUnreadHistory`保留未读样本。修复前失败XML保留。 |
| KEEP_LAST槽位耗尽后永久停写 | `ReviewKeepLastCapacityMustSustainWritesOrRejectAtCreation`覆盖1/2槽位反复写；`KeyedKeepLastSingleSlotSharesBudgetWithoutOverwritingWriterLoans`覆盖总预算、Writer Loan安全及归还恢复；`KeepAllSharedExhaustionPreservesLoansAndAutoFallbackRecovers`覆盖KEEP_ALL、ON/AUTO及两种可靠性。修复前失败XML保留。 |
| 自动恢复与取消 | 可靠/尽力分别测试映射与注册容量暂时不可用后同Reader恢复；`MappingRetryCancellationAndDuplicateDiscoveryLeaveNoRegistration`及`MappingNewSessionCancelsOldRetryAndDuplicateStartKeepsRegistration`覆盖删除、重复启动和新会话。 |
| 有限Reader回收 | `KeyedFiniteAutopurgeReleasesBudgetAndPreservesOutstandingLoans`在单用例内覆盖12种组合；`KeyedAutopurgeStateRecheckCancelsOldExpiryAndSharedWriterLoss`确定性验证新ALIVE/多Writer/同键重建；`KeyedFinitePurgeCannotReuseAnOutstandingSharedReaderLoanSlot`固定真实共享槽位。 |
| 时间戳、lifespan、延迟过滤 | `ReaderTimestamp.*`、`KeyedReaderLifespanRetiresHistoryByCacheIdentityAndKeepsLoanAlive`（不同Writer同SN）、`LifespanRecheckedBeforeTimeBasedFilterDelayedCommit`。 |
| 并发首次日志 | `LoggerSynchronization.ConcurrentFirstUseAndRestartAcrossExec`独立exec，以32线程同时首次调用及停止/重新启动；旧库正式失败，独立terminate栈落在LogInitial，修复后重复通过。 |
| Timer/QoS/状态重入 | `TimerSynchronization.*`，Reader/Writer并发QoS和手动超时压力；`TimeBasedFilterListenerAllowsAnotherThreadToChangeQos`验证定时提交的listener内等待另一线程set_qos不会死锁；`*DeadlineListenerCanQueryStatusAndChangeItsOwnQos`；多Writer注销、dispose、不存在实例、有限/无限变化回归。 |
| 条件/销毁与旧WaitSet异常 | `DestroyConditionAttachedToSeveralWaitSetsDetachesEveryEntry`、`ReaderDetach*`，两版30轮重点场景与TCP/WaitSet重复；历史无栈SIGSEGV根因仍未知。 |

本矩阵采用代表性组合，不宣称完整线程交错或分支穷尽。host/namespace不兼容由能力编解码与选择测试验证，实际跨进程回退还使用显式OFF Reader和纯UDP smoke；不将同一VM的DS通信称为真实跨主机验收。有限回收依据DDS ReaderDataLifecycle的disposed/no-writer独立状态语义，实例索引释放和旧句柄失效是本实现明确的本地行为。参照规范：<https://www.omg.org/spec/DDS/1.4>。

## 历史完整流程复现（2026-10-28）

在新的检出目录执行，保持`--parallel 1`。先阅读Boost版本、固定布局类型承诺和Ubuntu资源要求；无需全局安装。

```bash
python3 tools/ospp/freeze.py --manifest artifacts/accept/frozen-source.json
python3 tools/ospp/record.py --output artifacts/accept --manifest artifacts/accept/frozen-source.json \
  --label final-normal-build -- bash tools/ospp/bootstrap.sh artifacts/local-loan
python3 tools/ospp/record.py --output artifacts/accept --manifest artifacts/accept/frozen-source.json \
  --label final-normal-full -- python3 tools/ospp/accept.py --prefix artifacts/local-loan \
  --output artifacts/accept/normal-full --mode normal --stage full
```

同一运行器的`--stage repeat --rounds 30`覆盖重点场景；`--stage tcp-waitset --rounds 6`覆盖既有TCP/WaitSet。检查版使用`CORE_TESTS=1 COVERAGE=1 ASAN_DETECT_LEAKS=0 bash tools/ospp/sanitize.sh "$PWD/artifacts/local-loan-sanitize"`，然后`accept.py --mode sanitize`。两个版本不能同时构建或运行。三个有限示例用`datasharing_examples.py --rounds 30`。覆盖率使用`coverage.py --baseline 489067108405b27afbd369245542f5e88d457f18 --minimum 70`；性能配置和命令见性能手册。

最终`deliver.py --output artifacts/accept --normal artifacts/local-loan --sanitize artifacts/local-loan-sanitize`要求所有必需阶段实际成功、源码清单未变、安装头与源码一致、程序链接各自私有库；失败时不会生成成功报告。命令标签与目录约定见脚本。LSan/TSan若宿主无法运行须单独保留探针退出和输出，不能写成通过或“无泄漏”。

独立TSan构建可使用同一CMake工程，另设前缀：Debug flags为`-O1 -g1 -fsanitize=thread -fno-omit-frame-pointer`，共享链接flags为`-fsanitize=thread`，测试flags另加`-fno-pie`、测试链接`-fsanitize=thread -no-pie`。本机默认映射布局使TSan探针退出66；`setarch x86_64 -R`探针通过，因此构建时的gtest discovery与实际运行也使用该进程级设置（不更改宿主sysctl）。实际相关路径的结果在本轮checker证据中记录，不以小探针成功替代生产路径检查。


## 历史完整阶段命令（2026-10-28）

以下是 2026-10-28 同一冻结版本的完整串行顺序；其中 XML、覆盖率和通过数字仅证明布局 v5，不能替代当前版本验收。使用新的证据目录；不要覆盖已有失败或复用不同版本XML。三个review的修复前失败证据属于本轮历史，在新的已修复检出目录不会再次产生旧库失败。`deliver.py`审计本轮交付目录；新机器复现应保留原始修复前证据并另存本机结果。

```bash
set -e
OUT=artifacts/completion-20261028
NORMAL=artifacts/local-loan
SANITIZE=artifacts/local-loan-sanitize
record() {
  python3 tools/ospp/record.py --output "$OUT" --manifest "$OUT/frozen-source.json" "$@"
}
# 首次建立清单；之后只能 --check，不能用重写清单掩盖执行中源码变化。
python3 tools/ospp/freeze.py --manifest "$OUT/frozen-source.json"
record --label final-normal-build -- bash tools/ospp/bootstrap.sh "$NORMAL"
record --label final-normal-full -- python3 tools/ospp/accept.py   --prefix "$NORMAL" --output "$OUT/normal-full" --mode normal --stage full
record --label final-normal-repeat -- python3 tools/ospp/accept.py   --prefix "$NORMAL" --output "$OUT/normal-repeat" --mode normal --stage repeat --rounds 30
record --label final-normal-examples -- python3 tools/ospp/datasharing_examples.py   --build "$NORMAL/build-data_sharing" --output "$OUT/normal-examples" --rounds 30
record --label final-smoke -- python3 tools/ospp/smoke.py   --build-root "$NORMAL" --output "$OUT/smoke"
record --label final-tcp-waitset -- python3 tools/ospp/accept.py   --prefix "$NORMAL" --output "$OUT/tcp-waitset" --mode normal --stage tcp-waitset --rounds 6
record --label final-sanitize-build-test -- env CORE_TESTS=1 COVERAGE=1 ASAN_DETECT_LEAKS=0   bash tools/ospp/sanitize.sh "$PWD/$SANITIZE"
# collect只采集刚执行的sanitize.sh XML/日志；不声称额外执行了一遍测试。
record --label final-sanitize-full -- python3 tools/ospp/accept.py   --prefix "$SANITIZE" --output "$OUT/sanitize-full" --mode sanitize --stage collect
record --label final-sanitize-repeat -- python3 tools/ospp/accept.py   --prefix "$SANITIZE" --output "$OUT/sanitize-repeat" --mode sanitize --stage repeat --rounds 30
record --label final-sanitize-examples -- env ASAN_OPTIONS=detect_leaks=0:halt_on_error=1   UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 python3 tools/ospp/datasharing_examples.py   --build "$SANITIZE/build-data_sharing" --output "$OUT/sanitize-examples" --rounds 30
record --label final-tsan -- bash tools/ospp/tsan.sh "$PWD/$OUT/tsan"
record --label final-coverage -- python3 tools/ospp/coverage.py --source "$PWD"   --build-root "$SANITIZE" --output "$OUT/coverage"   --baseline 489067108405b27afbd369245542f5e88d457f18 --minimum 70
cmake -S tools/ospp/benchmark -B "$NORMAL/build-benchmark" -DCMAKE_BUILD_TYPE=Release   -DCMAKE_PREFIX_PATH="$PWD/$NORMAL/install"
record --label final-benchmark-build -- cmake --build "$NORMAL/build-benchmark" --parallel 1
record --label final-benchmark -- python3 tools/ospp/benchmark.py   --binary "$NORMAL/build-benchmark/datasharing_benchmark" --output "$OUT/benchmark" --rounds 3
python3 tools/ospp/freeze.py --manifest "$OUT/frozen-source.json" --check
python3 tools/ospp/deliver.py --output "$OUT" --normal "$NORMAL" --sanitize "$SANITIZE"
```

TSan阶段若报宿主映射/权限问题，保存真实退出和原始输出后继续其他独立项；若报实际race，应分析和修复，不能写成环境限制。环境相关checker摘要、进程与共享文件清理证据需在最终报告中单独记录。普通无限smoke由运行器终止，不要求其自然退出；有限示例和benchmark必须自然退出。
