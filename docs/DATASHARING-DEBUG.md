# DataSharing 调试与验证

本手册针对 Linux 同主机固定布局 DataSharing。API 见 [DATASHARING-API.md](DATASHARING-API.md)，配置见 [DATASHARING-QOS.md](DATASHARING-QOS.md)，有限示例见 [README](../example/cpp/data_sharing/README.md)。当前验收结果和限制见 [本轮报告](DATASHARING-COMPLETION-20261028.md) 及 [状态](DATASHARING-PLAN.md)。

## 从可复现示例开始

```bash
BUILD_JOBS=1 bash tools/ospp/bootstrap.sh "$PWD/artifacts/local-loan"
python3 tools/ospp/datasharing_examples.py \
  --build artifacts/local-loan/build-data_sharing \
  --output artifacts/local-loan/results/my-finite-run --rounds 1
```

结果目录必须是新目录。每个示例应自然返回 0，输出两个不同 PID 的 `DS_RESULT`。先查看失败用例的 stderr 和 result.json：进程被超时工具终止、只收到一端的结果、计数不符或目录未删除都不是成功。原有 hello_world/transmit_shm 示例会无限运行，smoke.py 主动停止它们，不能用其退出状态证明有限生命周期正确。

## 判断实际数据路径

1. 确认两端 domain、topic、类型名、可靠性和其他匹配 QoS 相容。`get_matched_subscriptions`/`get_matched_publications` 只能证明 DDS 匹配。
2. 确认类型声明 bounded/plain、非零大小、合法对齐和相同版本化布局 ID。布局内不得含跨进程无效的指针或动态容器。
3. 确认两端处于相同主机、PID namespace 和可访问的共享文件系统。容器间仅使用相同目录字符串不代表访问同一个共享段。
4. 确认 Reader 映射预算足够，目录可访问，Writer 的槽位和 Reader 注册容量未耗尽。
5. 在隔离的测试中检查原生借用及完整载荷序列化计数。有限示例在 OFF Reader 创建之前禁用完整载荷 CDR，并验证一次真实 Reader Loan，以确认共享路径已经建立。

共享路径仍然使用发现、心跳、ACK，以及带键生命周期的 RTPS 控制消息。看到 UDP 流量不能直接判定载荷没有共享。反之，启用旧 SHM transport 也不能证明绕过序列化。示例关闭旧 SHM transport，并在混合模式精确验证 CDR 调用数。

## 常见结果

| 现象 | 检查方向 |
|---|---|
| ON Writer 创建返回空 | 关闭自动 enable 后显式 enable 获取错误码；检查类型、目录、预算和资源配置 |
| `loan_sample` 返回 UNSUPPORTED | OFF、类型不支持、平台不支持，或 AUTO 本地准备失败；检查 enable 日志 |
| OUT_OF_RESOURCES | 应用未提交/取消的 Writer Loan、Writer history、未读 Reader 投递和应用 Reader Loan 分别可能持有槽位；不能只数 history |
| 匹配成功但仍走 CDR | 对端 OFF/类型不相容、共享组不交集、目录/预算/注册失败或内容过滤；检查实际映射注册 |
| Writer write 失败后 Loan 仍有效 | 失败不消费应用 Loan；修正原因后重试或 discard，不得同时把它交给另一个 Writer |
| 旧句柄 BAD_PARAMETER | unregister 后不再属于当前注册实例；重新注册同键再写入 |
| return_loan 返回 PRECONDITION_NOT_MET | 检查是否混用了 Reader、数据/信息序列或借用批次；双方均已无借用时再次归还返回 OK |
| 新加入的可靠 Reader 卡在第一条数据 | 检查首次 HEARTBEAT 的 firstSN、已收缓存及 GAP；发现公告也可能因旧缺口未提交。本补丁有 WriterProxyGap 回归及后加入 OFF Reader 的有限示例 |
| 未写入样本就期待 deadline 通知 | 先建立实际实例/样本；空 history 的定时唤醒不能注册 NIL 实例。使用真实 write/receive 后再验证 WaitSet 状态 |
| unregister 后句柄再次可查 | 检查迟到 deadline 回调是否插入实例；SetNextDeadline 仅更新已有注册，相关回归同时验证容量可重新使用 |
| Reader 删除后旧 Loan 仍占内存 | Loan 有独立映射所有权，直到应用归还/释放才结束；共享文件 unlink 与映射释放是两件事 |

当前 ON 只强制本地准备；对端不支持或映射失败仍保留 RTPS。兼容会话映射/注册暂时失败后自动重试，25ms至1s退避。日志区分fallback/retry/success/non-retryable；CORRUPT或不兼容需新会话，不能恢复损坏段。

## 定位失败并保存证据

保留失败时的源码 SHA、完整 stdout/stderr、退出码、测试 XML、进程身份和运行库路径。先单独运行失败用例，再重复其前置场景；只重跑最后一个用例可能漏掉之前的生命周期影响。并发问题需要实际线程栈或确定性复现支持，不能把后来发现的其他缺陷当作既有崩溃的已证实原因。

```bash
ldd artifacts/local-loan/build-data_sharing/datasharing_mixed
artifacts/local-loan/build-unit_payloadpool/payloadpool_test \
  --gtest_filter='WriterProxyGap.*'

CORE_TESTS=1 COVERAGE=1 ASAN_DETECT_LEAKS=0 \
  bash tools/ospp/sanitize.sh "$PWD/artifacts/local-loan-sanitize"
ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 \
UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 \
  python3 tools/ospp/datasharing_examples.py \
    --build artifacts/local-loan-sanitize/build-data_sharing \
    --output artifacts/local-loan-sanitize/results/my-finite-run --rounds 1
```

检查版和普通版分开安装、分开运行。宿主限制 ptrace 时 LeakSanitizer 可能失败，关闭泄漏扫描不关闭 ASan/UBSan，但也不能得出无泄漏结论。完整测试比较耗时；本机所有编译保持单线程，DDS 测试串行运行。

## 共享资源清理

正常 Writer 关闭会 unlink 自己的会话段，已经持有的映射可以继续读。异常终止后的 orphan 必须先核对路径、Writer GUID、随机 session、创建 PID 及进程启动身份，再用内部 `SharedPayloadPool::removeOrphan` 的校验接口清理。它不是公开应用 API。不能仅凭 PID 不存在、文件名相似或文件较旧就删除，也不要对共享机器的 `/dev/shm` 通配清理。

有限示例每次使用独立目录并检查为空；异常退出可能保留目录供排查。目录中存在未知文件时示例不会递归删除。生产环境应预留可审计的运维流程，并分别监控段文件、映射和应用未归还借用。


## Deadline / lifespan 的针对性回归

```bash
artifacts/local-loan/build-unit_payloadpool/payloadpool_test \
  --gtest_filter='LoanIntegration.KeyedDeadline*:LoanIntegration.KeyedLifespan*:PayloadPoolHistory.Deadline*:NativeReaderIntegration.Deadline*:WriterProxyGap.*:MultiWriterDeadlineIntegration.*'
```

其中生命周期用例直接控制 history 的到期时间，验证不会凭空注册、复活注销句柄或提前报告；核心 StatusCondition 的两个 deadline 用例另通过实际 write/receive、真实定时器和 WaitSet 验证正反两条路径。不要将前者的强制到期当成真实时钟准确性测试。

首次 HEARTBEAT 可以宣告比 1 更大的 firstSN，例如旧 SEDP 公告与注销已从 Writer history 移除，只剩最新公告序号 3。Reader 必须先跳过尚未收到且已不可能获得的旧序号，才能提交已收到的最新缓存；已有的旧缓存仍应保留。只看到 matched handle 或缓存到达，不能证明公告已向应用发现层提交。


### 重启与通知排查

共享布局当前为 5；旧布局在执行共享锁操作前拒绝。通知使用 Linux futex，等待采用单调时钟期限。若锁持有者死亡，则 CORRUPT 是安全拒绝，应重建端点。不要将已死 DDS Reader 的 lease 保留误认为共享 Loan 未回收；共享池统计按真实进程身份回收 pin，RTPS 发现仍可暂时保留旧匹配。

新 Participant 可能复用旧 Reader entityId，定向回退数据必须同时带目标 Participant 的 INFO_DST。接收 DATA/FRAG 时丢弃外部前缀；UNKNOWN 允许广播。相关回归为 LoanIntegration.*DestinationPrefix*。

Reader max_instances 满时，默认无限 autopurge 保留的已知键仍占容量，即使样本已 take 或 Loan 已归还。按业务实例数配置预算，或删除重建 Reader；需要有限回收时显式配置reader_data_lifecycle；其状态转移计时、实例预算释放及Loan保活见QoS手册。


本轮最终验收：普通与ASan/UBSan各468/468，两版各55项×30轮及三个有限示例各30轮通过；新增生产行覆盖率94.118%（2432/2584）。独立TSan限定路径通过，LSan未运行、历史WaitSet根因未知。当前完整证据与限制以 [本轮报告](DATASHARING-COMPLETION-20261028.md) 为准，早期数字只保留历史含义。
