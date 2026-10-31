# DataSharing 官方八项验收对照

本页按官方项目 **261760062** 的原始 programDesc/outputRequirement 核对当前 **共享布局 v7 / discovery v5**。原文、逐文件哈希和验收计划保存在 [原始要求](../../../artifacts/strict-ring-20261031/requirements/acceptance-plan.json)。本轮已补齐真实发布历史环及原生载荷直接访问证据；完整普通回归 **542/542**、有限示例 **9/9**、新增生产可执行行覆盖率 **94.132%（2984/3170）**，独立 **ASan/UBSan 99/99、TSan 99/99** 通过。八项实现与当前运行证据已齐全，最终交付身份核验见 [综合审计原始记录](../../../artifacts/strict-ring-20261031/final-verification.json)。

| 官方交付 | 当前实现及直接证据 | 本轮检查 |
|---|---|---|
| 1. 实现 PayloadPool 接口 | IPayloadPool/ITopicPayloadPool、四种普通池、按 Topic 的 Writer/Reader 共享、失败事务归还及分片生命周期 | 完整模块 249 项及核心 247 项通过；Reader 普通池检查版通过 |
| 2. 实现 DataSharing 共享段管理 | Boost 映射、Writer GUID/session 命名；真实闭合发布历史环，Reader/replay 沿环；64 位位置/代次/epoch、pin、关闭/死亡/孤儿处理 | 共享池 46 项包含新增 7 项环及 2 项只读保护检查；普通版、ASan/UBSan 与 TSan 通过 |
| 3. 实现跨进程通知 | 原子通知代次/futex、Reader listener；先采样再检查谓词；关闭后重扫最终水位 | 通知回绕、信号、进程死亡及真实 DDS 管理锁暂停进展检查通过 |
| 4. 实现 DDS 层 Loan API | Writer loan/write/discard、Reader read/take/return、错误返回及删除后的 Loan 保活 | 原生生命周期、错误分支和页保护集成检查通过 |
| 5. 零拷贝 Read/Write 旁路与 RTPS 回退 | no-init 构造、直接共享 Loan、key-only、v5 协商隔离 v1–v4；同 backing 的独立进程原生访问；RTPS/mixed 回退保留 | 4 项页面保护测试及原始 proof 核验通过；原生完整载荷序列化/反序列化/copy/create 回调为 0；混合示例通过 |
| 6. DataSharingQosPolicy | AUTO/ON/OFF、目录/容量/预算、API/XML、类型与协议准入、不可变配置及重试 | 配置、协商和降级的普通模块回归通过 |
| 7. 单元测试 ≥70% 及 Loan/QoS 示例 | 当前 Ubuntu 上重新构建全部生产库、完整普通回归；Loan、keyed、mixed 各 3 轮独立 exec；143 个翻译单元覆盖数据 | 542/542、9/9、94.132%；ASan/UBSan 与 TSan 各 99/99 |
| 8. 详细设计与 API/QoS/错误调试手册 | [设计](DATASHARING-DESIGN.md)、[环与载荷证据](DATASHARING-RING.md)、[API](DATASHARING-API.md)、[QoS](DATASHARING-QOS.md)、[调试](DATASHARING-DEBUG.md)和公开示例 | 手册已同步 v7/v5；[综合审计](../../../artifacts/strict-ring-20261031/final-verification.json)及[交付索引](../../../artifacts/portable-evidence-20261031/native-v5-delivery.md)记录最终证据 |

## 原文严格范围与证据边界

“按Writer GUID命名的共享环形历史段”由实际发布顺序的共享闭合双链实现；分配位置不是发布顺序，取消的未发布 Loan 不进入历史。正常 Reader 和晚加入回放沿历史环读取，仅 epoch 不稳定时保留安全扫描；稳定坏环返回 CORRUPT。实现、事务和九项新增测试见 [环说明](DATASHARING-RING.md)。

原文“同主机plain/bounded类型数据读写全程物理内存仅写入1次，无任何用户态拷贝”与第 5 项“零拷贝分支，同时保留 RTPS 降级回退”一同核对。工程上验证的是兼容端点实际选择的 native Writer Loan → Reader Loan：应用在最终共享槽位单遍填写，框架不初始化或搬运完整载荷。独立 exec 的实际 Loan 逐页 VMA 证明同 device/inode/file offset 和 MAP_SHARED；框架窗口为 PROT_NONE（key 页只读），应用仅有一次填写窗口。read/write 故障负控、所有别名保护、seccomp 拒绝探针、完整载荷回调和源码/启动路径审查共同提供证据。键、索引及控制元数据仍可复制；普通 write、预分配输出和 RTPS 保持兼容复制语义。

该证据不测量 DRAM 总线事务次数；内核清零、缓存写回、页迁移和 ECC 不受 DDS API 控制。没有把“仅拦截 CDR/copy 回调”冒充完整载荷访问证明，也没有声称任意恶意 TypeSupport 可以免除类型契约。

官方要求 DataSharingNotification 无锁通知，没有要求整个 DDS write 形式化 lock-free/wait-free。共享数据面不获取跨进程管理 mutex；注册/维护、DDS 本地 Writer/history/发现锁和 futex 系统调用仍存在。原文平台为 Ubuntu **或** openEuler，没有自动 IDL native 生成器这一额外硬性条件。

## 当前冻结与复核入口

本轮 **323** 项生产/测试/工具输入的规范身份为 `42ee01a89f3cba2e11df8147ece3ea73ba2906c0652bcbd0c71e460262b6b3ea`；[冻结清单](../../../artifacts/strict-ring-20261031/frozen-source-atomic-final.json)保存逐文件 SHA-256。当前普通结果位于 [coverage-atomic-final](../../../artifacts/strict-ring-20261031/coverage-atomic-final/results.json)，独立 [ASan/UBSan](../../../artifacts/strict-ring-20261031/asan-atomic-final/results.json) 和 [TSan](../../../artifacts/strict-ring-20261031/tsan-atomic-final/results.json)均通过。[精确绑定](../../../artifacts/strict-ring-20261031/final-bindings.json)和[最终审计](../../../artifacts/strict-ring-20261031/final-verification.json)核对三轮同一源码、私有链接二进制、原始 proof、覆盖率、示例及清理状态；[交付索引](../../../artifacts/portable-evidence-20261031/native-v5-delivery.md)保存源码与便携证据包。

资源边界为本机 jobs=1、nice=19、ionice best-effort 7、编译进程地址空间 768 MiB，无新增 swap 或支出。ASan/UBSan 不启用 LSan，不作泄漏扫描通过的声明。新源码不能借用前轮数字验收；v6/v4 的 529 项、93.936% 和检查版各 86 项仅保留于 [前轮报告](DATASHARING-PROGRESS-20261031.md)。

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
