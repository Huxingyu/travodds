# 需求、实现与验收映射

本页按官方项目 261760062 的八项交付对应当前严格生产实现：共享布局 v6、发现能力 v3，带键 native 强制有效 key-only 元数据，并移除原生全样本键快照回退。新增生产行覆盖率固定比较基线为 `489067108405b27afbd369245542f5e88d457f18`，最低 70%。验证平台为 Ubuntu **或** openEuler，本轮采用 Ubuntu。当前 `artifacts/strict-native-20261030/coverage-complete/` 已验证 500 项回归、9 次示例执行和 93.920% 新增行覆盖率；本轮独立完整插桩 ASan/UBSan 与 TSan focused 各 57 项通过，最终汇总审计通过。

官方描述的“DataSharingNotification（无锁通知）”及第 3 项“实现跨进程通知机制”没有要求整个 DDS write 路径无锁。共享数据面的预留、发布、借用/归还和 ready 注册快照不获取跨进程管理 mutex；注册/维护、DDS 本地 Writer/history/发现表锁和 futex 系统调用仍存在，不宣称全路径形式化 lock-free 或 wait-free。

| 官方交付 | 当前实现入口与范围 | 当前严格版本验收状态 |
|---|---|---|
| 1. 实现 PayloadPool 接口 | `payloadpool*`、`topicpayloadpool*`、`payloadbuffer*`：接口、四策略、Topic 注册、原池归属、分片失败回滚 | 当前 216 项 payloadpool/Loan 模块测试通过，包含 `TopicPoolPolicyTest.*`、`TopicPoolRegistry.*`、`PoolIntegration.*`、`TakePoolIntegration.*` |
| 2. 实现 DataSharing 共享段管理 | `sharedpayloadpool*`：Boost 映射、布局 v6、运行时预算、64 Reader、64 位代次、pin、关闭/死亡及孤儿检查 | 本轮共享池 37/37 通过，见 `coverage-complete/sharedpayloadpool_test.xml` |
| 3. 实现跨进程通知机制 | 32 位原子代次/futex、Reader listener；先采样代次再检查谓词，关闭后重扫最终水位 | 本轮共享池及真实 DDS 控制锁暂停回归通过；不要求整个 DDS write 无锁 |
| 4. 实现 DDS 层 Loan API | Writer loan/write/discard、Reader read/take/return、独立映射所有权与删除保活 | 本轮 216 项模块测试验证容量、失败保留、配对归还和删除后的 Loan |
| 5. Read/Write 旁路及 RTPS 降级 | TypeSupport 统一原生准入，带键必须有效 NativeKeyInfo；discovery v3 排除 v1/v2；原生捕获/查询无全样本回退 | 13 项准入回归及旧协议隔离、失败关闭、纯共享/混合 RTPS 回归通过；native 端点对遵循完整载荷无复制契约 |
| 6. 新增 DataSharingQosPolicy | AUTO/ON/OFF、目录/预算、API/XML、映射/注册重试；缺失/非法键能力时 ON 拒绝，AUTO 不广告 DS 并保留 CDR/RTPS | 本轮缺失/非法键能力的 Writer/Reader ON/AUTO 门控及原有 QoS/XML 回归通过 |
| 7. 新代码覆盖率 ≥70% 与 Loan/QoS 示例 | `coverage.py` 合并新增生产可执行行和模板；三个有限独立 exec 示例 | **93.920%（2765/2944）≥70%，140 个翻译单元；三个示例各 3 轮，共 9 次执行通过。** 见 `coverage-complete/{coverage,examples}/summary.json` |
| 8. 设计及 API/QoS/错误/调试文档 | 当前手册及 README 按 v6/v3、强制键契约和明确失败边界更新 | [严格准入报告](DATASHARING-STRICT-20261030.md) 记录当前结果、319 输入身份及库哈希；历史报告单独标记 |

严格零载荷复制是指：对符合类型契约、实际选中 native 的端点对，Writer Loan → Reader Loan 分支不复制完整用户载荷；键、索引、地址数组和控制元数据仍可复制，不承诺任何字节都不复制。普通裸指针 write、预分配输出和对其他 RTPS 端点的序列化不属于这个原生借用分支。“plain/bounded”要求固定无指针布局，不等于任意有界 C++ 容器均可共享。

Writer/Reader 各自通过 `supports_native_data_sharing(info)` 校验原生资格；带键类型只有 NativeKeyInfo 合法才可广告 DS。元数据声明是实现 capture/restore 的契约，不在 enable 用假样本探测；实际钩子 false/标准异常失败关闭，不退回原生全样本 memcpy/copy_data。v3 隔离仍容许旧全样本快照的 v2 对端。TypeSupport 二进制 ABI 已变，库、应用及子类必须一起重编译。

## 当前证据状态与复现

当前证据根为 `/home/ubuntu/project/travodds/artifacts/strict-native-20261030/`。`coverage-complete/results.json` 的 passed=true：共享池 37 + payloadpool/Loan 216 + 核心 247 = **500/500 通过，失败/错误/禁用均为 0**；三个示例各 3 轮，共 9 次执行通过；新增生产可执行行覆盖率 **93.920%（2765/2944，140 个翻译单元）**。本次 `asan-final/results.json` 与 `tsan-final/results.json` 均为 passed=true：各自完整插桩生产库、GTest、测试驱动，**每版 37 项共享池 + 13 项准入 + 3 项 ThreadPool + 4 项 DDS/key-only = 57/57 通过**，失败/错误/禁用/跳过均为 0。三模式 319 项源码/构建输入相同，具体库与结果 SHA-256 见 [严格报告](DATASHARING-STRICT-20261030.md)。

[最终汇总审计](/home/ubuntu/project/travodds/artifacts/strict-native-20261030/final-verification.json) passed=true，SHA-256 为 `332eebdc40807039d394aec610fd71723137fa85172cfbef4b70535875c51d81`。审计核验源输入、三模式逐项测试、每模式 68 个安装头文件、共 12 个程序的私有库链接、生产库/GTest 插桩及覆盖率/示例原始绑定；审计时无共享段或活动测试程序残留。

首轮 `coverage-final` 的 ThreadPool 关闭丢唤醒及第二轮 `coverage-verified` 的 Ownership 测试等待缺陷均保留失败证据；修复后先归档/重置 135 个 gcda，再生成当前完整结果。ThreadPool 三项回归和修正后的 Ownership 用例均已包含在当前通过版本中；定向重复 20 轮只是附加证据，不算新增测试数量。

```bash
python3 tools/ospp/production_lockfree_verify.py \
  --output /absolute/new-strict-evidence \
  --build-dir /absolute/strict-build \
  --mode coverage --core --examples 3 \
  --baseline 489067108405b27afbd369245542f5e88d457f18
```

ASan/UBSan 与 TSan 分别使用新 build/output，选择 `--mode asan --scope focused`、`--mode tsan --scope focused`，通过 `--focus-test` 显式加入本轮门控用例。TSan 实际运行需记录 `setarch x86_64 -R`；focused 只证明实际执行的范围，不代表全部 DDS 集成均在检查器下通过。LSan 无泄漏结论仍为空缺。

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
