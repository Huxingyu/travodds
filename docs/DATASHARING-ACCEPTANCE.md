# 需求、实现与验收映射

固定比较基线：`489067108405b27afbd369245542f5e88d457f18`。本轮证据为 `artifacts/completion-20261028/`；`commands.jsonl`记录实际时间/退出与资源，`frozen-source.json`绑定代码、构建、测试、工具输入。文档在结果产生后更新，不改变构建身份。完整结果由`verification.json`审计，不能用旧版本测试替代。

| 官方交付 | 实现入口 | 代表性正式测试 | 本轮证据 |
|---|---|---|---|
| PayloadPool接口、四策略、Topic注册 | `src/common/memory/{payloadpool,topicpayloadpool,payloadbuffer}*`、Writer创建与分片 | `TopicPoolPolicyTest.*`、`TopicPoolRegistry.*`、`PoolIntegration.*`、`TakePoolIntegration.*` | 两版`*-full/payloadpool.xml` |
| 命名共享环形段、引用与异常管理 | `sharedpayloadpool*`，Boost设备/mapped_region，布局v5 | `SharedPool.*`；含不同映射地址、旧token、异常持锁、预算、并发慢Reader | 两版`*-full/sharedpayloadpool.xml` |
| 跨进程通知与监听 | 原子通知代次/futex、`readerdatasharing.cpp`、WriterProxy | `*NativeLoanAcrossExec*`、`*MultiTopic*`、`*KilledReader*`、`WriterProxyGap.*` | 两版完整及30轮重点重复 |
| Writer/Reader Loan API | `sampleloan*`、sequence、DataWriter/DataReader实现 | `LoanSequence.*`、`*WriterLoanCapacity*`、`NativeReaderIntegration.*`、12种`NativeReadVariants` | 两版完整XML、有限Loan示例 |
| 读写旁路与RTPS回退 | Writer选择已注册Reader、按Participant/Reader定向CDR、两种Reader准入 | 可靠/尽力×keyed/unkeyed/晚加入/混合OFF的`*AcrossExec*`，`*DestinationPrefix*`、`DataSharingDiscovery.*` | 两版重复；普通default/SHM/UDP smoke、TCP回归 |
| DataSharing QoS及XML | `datasharingqospolicy.h`、发现扩展、QoS校验、XML | `DataSharingQos.*`、`DataSharingXml.*`、`*UnavailableDirectory*`、`*AutomaticMappingRetry*`、`*AutomaticRegistrationRetry*` | 两版完整/30轮；同Reader CDR→禁止CDR的DS恢复 |
| ≥70%新增生产行与三个完整示例 | `coverage.py`、`example/cpp/data_sharing` | GCC合并所有gcno及模板，变更.cpp缺收集即失败；3示例独立exec、自然退出、载荷/地址/CDR/清理检查 | `coverage/summary.json`；两版`*-examples/summary.json`各30轮 |
| 设计、API、QoS、错误与调试手册 | `docs/DATASHARING-*`、`PAYLOADPOOL-QOS.md`、可复现工具 | 交付清单、安装头/库链接、patch基线检查 | `verification.json`、`final.patch`、`delivery-source.json` |

| 本轮正确性问题 | 回归与关键断言 |
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

## 串行复现

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


## 完整阶段命令

以下是同一冻结版本的完整串行顺序。使用新的证据目录；不要覆盖已有失败或复用不同版本XML。三个review的修复前失败证据属于本轮历史，在新的已修复检出目录不会再次产生旧库失败。`deliver.py`审计本轮交付目录；新机器复现应保留原始修复前证据并另存本机结果。

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
