# 需求、实现与验收映射

本页按官方项目 261760062 的八项交付核对 2026-10-31 验收状态。共享布局保持 v6，发现能力为 v4；Reader 普通载荷池与借出不初始化完整载荷两项缺口已补齐，并在本轮 **529/529 普通回归、9 次有限示例及 93.936% 新增生产可执行行覆盖率**中通过，独立 **ASan/UBSan 与 TSan 各 86/86 项 focused 通过**。最终审计与两份便携证据的搬迁复核通过，**八项交付按下表限定范围完成**。当前冻结身份和原始结果见 [10/31 报告](DATASHARING-PROGRESS-20261031.md)。

官方描述的“DataSharingNotification（无锁通知）”及第 3 项“实现跨进程通知机制”没有要求整个 DDS write 路径无锁。共享数据面的预留、发布、借用/归还和 ready 注册快照不获取跨进程管理 mutex；注册/维护、DDS 本地 Writer/history/发现表锁和 futex 系统调用仍存在，不宣称全路径形式化 lock-free 或 wait-free。

| 官方交付 | 当前实现入口与范围 | 本轮验收状态 |
|---|---|---|
| 1. 实现 PayloadPool 接口 | 四策略、Writer/ReaderQos/XML、普通 RTPS history 完整分片链事务性转入 Topic 池；native SharedReadLoan 保持旁路 | **通过。** 19 项 Reader 回归在普通、ASan/UBSan 及 TSan 中通过，包含真实接收/分片、共享/隔离/冲突、耗尽回滚、MultiTopic 及删除后缓存寿命；普通项均包含在正式模块 245 项内 |
| 2. 实现 DataSharing 共享段管理 | Boost 映射、布局 v6、固定槽位轮转扫描、64 Reader、64 位代次、pin、关闭/死亡及孤儿检查 | **通过。** 共享池 37 项、模块 245 项与核心 247 项普通回归通过，共享池 37 项在 ASan/UBSan 和 TSan 中通过。轮转扫描不是 FIFO 环形队列，也不覆盖被 pin 的槽位 |
| 3. 实现跨进程通知机制 | 32 位原子代次/futex、Reader listener；先采样代次再检查谓词，关闭后重扫最终水位 | **通过。** 共享池普通及两个检查版回归通过，1 项真实 DDS 控制锁暂停进展和 3 项键/失败回归在 ASan/UBSan 与 TSan 中通过；不要求整个 DDS write 无锁 |
| 4. 实现 DDS 层 Loan API | Writer loan/write/discard、Reader read/take/return、独立映射所有权与删除保活；借出按新的无初始化契约处理 | **通过。** 正式模块中的 Loan 行为通过，10 项 no-init 在普通及两个检查版中通过，包括逐字节哨兵与槽位复用。应用必须先填写所有将被读取的字段 |
| 5. Read/Write 旁路及 RTPS 降级 | 强制 key-only 门控保留；supports_native_uninitialized_sample 准入、placement new T、discovery v4 隔离 v1/v2/v3 | **通过。** 无初始化、类型门控、旧协议隔离和 RTPS 降级普通回归已运行，无初始化/key-admission/DDS focused 在两个检查版通过。没有 IDL 自动 native 适配生成器 |
| 6. 新增 DataSharingQosPolicy | AUTO/ON/OFF、目录/预算、API/XML、映射/注册重试；无 native 资格时 ON 拒绝，AUTO 保留 CDR/RTPS | **通过。** 配置/发现、v4/no-init 门控及 Reader payload_pool 普通回归通过，Reader/no-init focused 在两个检查版通过 |
| 7. 新代码覆盖率 ≥70% 与 Loan/QoS 示例 | 固定基线 489067108405b27afbd369245542f5e88d457f18；Ubuntu 或 openEuler，三个有限独立 exec 示例 | **本轮通过。** coverage-release2 覆盖率 2835/3018 = 93.936%，142 个翻译单元，变更生产翻译单元无漏采集；loan/keyed/mixed 各 3 轮，共 9 次通过 |
| 8. 设计及 API/QoS/错误/调试文档 | 手册按 v6/v4、新类型契约和实际边界更新；小型证据相对索引、打包工具及三轮完整复现命令 | **完成。** 最终审计 passed=true；当前主包 3 个成功 run/1 个源码身份，历史补充包 4 个失败 run/两套诊断/3 个旧身份，搬迁复核均通过。入口和包哈希见 [10/31 报告](DATASHARING-PROGRESS-20261031.md#便携证据与复现) |

零完整载荷复制限定符合类型契约且实际选中 native 的 Writer Loan → Reader Loan 分支；普通裸指针 write、预分配输出和 RTPS 序列化仍可复制。无初始化是另一个必须独立验证的条件：借出不保证零初值，应用必须填写所有将由 Reader、键钩子或回退序列化器读取的字段。键、索引、地址数组和控制元数据仍可复制，不承诺任何字节都不复制。

Writer/Reader 均通过统一 TypeSupport 门控校验布局、无初始化资格及带键 key-only 元数据。NativeTypeSupport 的默认无初始化资格依赖 trivially default constructible；不满足者仍可编译使用 CDR，ON 拒绝 native，AUTO 回退。手写 TypeSupport 可以显式声明符合契约的固定布局；仓库没有任意 IDL 到 native 的自动适配生成器，HelloWorld 无界 string 保留 CDR/RTPS。“plain/bounded”也不等于任意有界 C++ 容器均可共享。

## 当前证据与执行约束

本轮 321 项输入已冻结，canonical SHA-256 为 `6ef3367ca3902a456b5ea5a59881f7190582fedac7fc4be78fbb85bc0d81cead`。当前 coverage-release2/results.json 为 passed=true，14 个命令均退出 0，共享池 37 + 模块 245 + 核心 247 = **529/529 通过**；正式模块包含 Reader 19 项和无初始化 10 项。三个有限示例各 3 轮、共 9 次通过，新增生产可执行行覆盖率 2835/3018 = **93.936%**，142 个翻译单元，普通结果完成于 `2026-10-31T05:56:30.721745Z`。asan-final/results.json 和 tsan-final/results.json 均为 passed=true，各 11 个命令退出 0，独立 **ASan/UBSan 与 TSan 各 37 + 16 + 10 + 19 + 4 = 86/86 项 focused 通过**，分别完成于 `2026-10-31T06:05:25.171427Z` 与 `2026-10-31T06:10:00.062875Z`；本轮 TSan 无 race 报告。LSan 未启用（ASAN_OPTIONS=detect_leaks=0），没有泄漏结论。最终审计于 `2026-10-31T06:10:27.599189Z` 记录 passed=true，核对三模式同一源码、每模式 68 个安装头、16 个私有链接程序、插桩与资源记录、覆盖率和共享文件/程序清理。原始结果及十一项产物哈希见 [10/31 报告](DATASHARING-PROGRESS-20261031.md#当前证据与最终审计)；当前主包和历史附包已生成并搬迁复核。三轮包含 768 MiB 限额的完整命令见 [当前复现命令](DATASHARING-PROGRESS-20261031.md#当前三轮完整复现命令)。

当前 VM 按用户授权使用 nice 19、ionice best-effort 7、单线程及编译子进程 768 MiB 地址空间限制推进。三轮构建失败均保留：coverage-final 库构建超时 124，coverage-verified 测试编译触及 512 MiB 上限退出 2，coverage-final768 新增 Reader 测试 GetEndianess 未声明导致编译退出 2；三轮均未运行正式测试。随后两次诊断仅修测试，完整模块诊断为 243/244，修正契约后新增 28 项定向通过；不据此计为正式通过。首次归档/清空 111 个 gcda 后的正式 coverage-release 共享池 37、模块 244 通过，核心 246/247，仅 MultiTopic Reader 池身份兼容性失败，整轮失败已保留。最小生产修复并新增 1 项 Reader 回归后，再次归档/清空 137 个 gcda，使用新冻结身份运行上述 coverage-release2 成功结果；四次失败和两次诊断保留原始状态，单独进入历史补充包。临时 256 MiB swap 已在无编译时安全关闭并删除，原 /swap.img 保持，清理记录 passed=true。实际用例、精确选择器及成功/失败退出必须与本轮 XML 和原始记录绑定，focused 不代表完整 DDS 套件，未执行 LSan 就没有泄漏结论。

原始 JSON/XML/log 与 source_sha256 已用 [package_evidence.py](../tools/ospp/package_evidence.py) 打成便携小包，相对索引和独立复核不依赖原机器路径。[当前主包索引](../../../artifacts/portable-evidence-20261031/native-v4-final-review/README.md) 收集三个实际成功 run 和单独暂存的审计元数据，恰有 3 个 passed=true、1 个当前源码身份，打包与搬迁后复核均以 --require-source-match 通过；[历史补充包索引](../../../artifacts/portable-evidence-20261031/native-v4-history-review/README.md) 保留四次失败和两套诊断，三个旧身份如实报告 changed/missing。大库和构建缓存不包含在包中，重跑需从匹配源码重建；便携复核不会重新执行测试或原机器完整审计。命令与包内范围见 [10/31 报告](DATASHARING-PROGRESS-20261031.md#便携证据与复现)。

## 10/30 discovery v3 历史快照

[10/30 严格报告](DATASHARING-STRICT-20261030.md) 绑定 checkpoint 570b779 对应的 319 项冻结输入：共享池 37 + payloadpool/Loan 216 + 核心 247 = 500 项通过，三个有限示例共 9 次执行，新增行覆盖率 2765/2944（93.920%），独立 ASan/UBSan 与 TSan focused 各 57 项通过。它们只证明当时已运行的范围，没有证明本轮 Reader Topic 池或无初始化语义。最终历史审计 SHA-256 为 332eebdc40807039d394aec610fd71723137fa85172cfbef4b70535875c51d81；原始数据保持不改写。

首轮 ThreadPool 关闭丢唤醒与第二轮 Ownership 测试等待缺陷均保留失败记录；修复后旧完整回归通过，其定向 20 轮只是附加证据，不算新增独立测试。历史性能见 [性能手册](DATASHARING-PERFORMANCE.md)，不能外推到本轮源码。

## 同日 discovery v2 历史快照

[可选 key-only 报告](DATASHARING-COMPLETION-20261030.md) 记录 `artifacts/completion-20261030/` 的旧冻结结果：共享池 37 + payloadpool/Loan 200 + 核心 247 = 484 项通过，三个示例各 3 轮、共 9 次执行，覆盖率 93.839%（2757/2938，138 个翻译单元），ASan/UBSan 与 TSan 各 41 项通过。其 final-verification.json 只证明仍允许旧带键 native 全样本快照的 discovery v2 代码，不是当前严格版本验收。

更早的 [生产协议报告](DATASHARING-PRODUCTION-20261030.md) 保留 234 项、两版各 38 项和共享池重复 20 轮记录，有限 SC 模型只检查回收 ABA 的特定交错；这些也属于历史，不是完整 C++ 弱内存证明。旧 LSan 未运行和历史无栈 WaitSet 崩溃未归因的限制继续保留。

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
