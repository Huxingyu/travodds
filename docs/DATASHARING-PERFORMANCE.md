# Boost DataSharing 历史性能（2026-10-28）

> 本页性能仅属于 10/28 布局 v5 冻结源码；不代表后续 v6/v4 当前版本。本轮尚未重新测量吞吐、延迟或 RSS，状态见 [10/31 报告](DATASHARING-PROGRESS-20261031.md)。

当日冻结输入SHA256：`953cfc4395a9273ef38aa0e01463f46a83de6dfc0f04b7a17be369e2f2b78361`。固定基线4890671。最后一轮测试与编译停止后，本机独立Writer/Reader、64/4096/65536B、RTPS普通write/共享普通write/Writer Loan，各三轮轮换顺序，共27次自然退出、退出0。所有载荷字段与CDR计数通过断言，原始指标在 `artifacts/completion-20261028/benchmark/summary.json`；每次日志含进程身份、预热、计数、耗时和RSS。

三轮中位数如下，延迟为每次分位数的中位数；RSS为三轮中各进程最大值，KiB。

| 样本 | 模式 | 吞吐 samples/s | p50 / p95 / p99 延迟 μs | Writer / Reader RSS KiB |
|---|---|---:|---|---|
| 64 B | rtps | 29378.7 | 105.4 / 153.7 / 205.9 | 17876 / 8576 |
| 64 B | ordinary | 8930.6 | 425.5 / 737.9 / 970.5 | 17876 / 8320 |
| 64 B | loan | 8878.9 | 373.9 / 665.3 / 1143.3 | 17876 / 8320 |
| 4096 B | rtps | 21607.4 | 157.9 / 222.3 / 316.0 | 18004 / 8448 |
| 4096 B | ordinary | 8445.8 | 417.8 / 694.5 / 1082.3 | 18004 / 8576 |
| 4096 B | loan | 8415.2 | 413.1 / 637.4 / 718.6 | 17876 / 8448 |
| 65536 B | rtps | 71.1 | 728.1 / 197495.0 / 199612.0 | 18004 / 9344 |
| 65536 B | ordinary | 4025.0 | 647.3 / 1204.7 / 1377.8 | 18004 / 12288 |
| 65536 B | loan | 4118.6 | 718.0 / 1078.7 / 1201.0 | 18004 / 12416 |

64KiB Loan/RTPS吞吐比约57.9倍；64B和4KiB共享路径分别约为RTPS的0.30、0.39倍，不能声称所有消息大小均提速。小消息的固定共享发布、引用、futex唤醒、Reader轮询与进程调度成本高于此本机UDP/CDR路径，省去64B/4KiB复制不足以抵消；普通共享write和Loan相近，也符合该固定开销占主导。64KiB RTPS分片、批次确认和重传/调度尾延迟在本短测占比大；不能把此比值外推为普遍吞吐收益。三轮短测未见相对于旧历史数据的数量级退化，但共享VM无法提供严格版本性能归因。

测量方法：RELIABLE、KEEP_LAST16、64槽位、每8条接收确认；预热32条，正式64B/4KiB各256条、64KiB128条，验证每个uint64字段。关闭旧SHM transport，rtps=OFF、ordinary/loan=ON。RTPS序列化与反序列化各精确等于预热+计量数，共享两模式完整载荷CDR各为0。吞吐包含填充、校验、控制和调度，延迟为写入前单调时钟到take后/校验前的nearest-rank分位数。

环境为2vCPU/约1.9GiB Ubuntu共享VM，带swap；没有编译或其他本任务DDS测试竞争。本结果不是最大可持续吞吐、跨主机网络RTT、硬实时或生产SLA。当日复现命令见 [验收矩阵的历史流程](DATASHARING-ACCEPTANCE.md#历史完整流程复现2026-10-28)，程序为 `tools/ospp/benchmark/benchmark.cpp`，运行器为 `tools/ospp/benchmark.py`。

## 历史POSIX后端（保留原记录）

以下只属于2026-10-27旧版本，不替代上述Boost结果。

> 下表为2026-10-27旧POSIX后端历史结果，不代表本轮Boost最终版本。10/28 Boost 历史结果入口：`artifacts/completion-20261028/benchmark/summary.json`，已列于文首。

测试机器为当前 2 vCPU、约 1.9 GiB RAM 的共享 Ubuntu VM。运行时暂停了编译和其他 DDS 测试。每次启动独立 Writer/Reader 进程，使用 RELIABLE、KEEP_LAST16、64 个共享槽位，Reader 每 8 条确认一批。先做实际收发探测，预热 32 条，正式计量 64/4096 B 各 256 条、65536 B 各 128 条；每个 uint64 字段都验证。三轮轮换模式顺序，共 27 次自然退出。

rtps 为 OFF 普通 write，经 UDP/RTPS 传递 CDR；ordinary 为 ON 普通 write，将应用对象复制到共享槽位；loan 为 ON Writer Loan，由应用直接填写共享槽位。三个模式都关闭了旧 SHM transport，避免与 DataSharing 混淆。ON 模式在探测之后禁止完整载荷序列化，正式阶段载荷 CDR 调用均为零；OFF 的序列化/反序列化调用各精确为预热量加计量量。

吞吐包含填充、校验、控制通道和调度成本；延迟从填写完样本、write 前的单调时钟到 Reader take 后、完整校验前计算，使用 nearest-rank 分位数。表中吞吐为三轮中位数，延迟为各分位数的三轮中位数，不是合并全部样本后的分位数。

| 样本大小 | 模式 | 吞吐 samples/s | p50 / p95 / p99 延迟（μs） |
|---|---|---:|---|
| 64 B | rtps | 29748.9 | 102.9 / 147.2 / 156.3 |
| 64 B | ordinary | 9344.0 | 396.2 / 692.9 / 869.0 |
| 64 B | loan | 8888.4 | 386.0 / 646.4 / 843.9 |
| 4096 B | rtps | 20188.6 | 161.5 / 284.5 / 343.0 |
| 4096 B | ordinary | 8421.7 | 374.9 / 649.7 / 748.2 |
| 4096 B | loan | 8572.8 | 399.3 / 631.0 / 1472.9 |
| 65536 B | rtps | 80.0 | 792.8 / 195639.0 / 199315.0 |
| 65536 B | ordinary | 3871.8 | 643.5 / 1199.0 / 1539.5 |
| 65536 B | loan | 3876.8 | 681.1 / 1633.7 / 1811.3 |

这组短时结果用于比较当前实现的路径成本。小样本、共享 VM、Reader 轮询、swap 和进程调度限制了推断；不能据此声称最大可持续吞吐、远程网络 RTT 或硬实时 SLA。Writer Loan 也不保证每个样本大小、每次运行都快于共享普通 write。

原始记录为 `artifacts/local-loan/results/benchmark-final/summary.json`，逐次日志在同一目录，含进程 PID、CDR 计数、耗时及各进程 RSS。程序为 `tools/ospp/benchmark/benchmark.cpp`，运行器为 `tools/ospp/benchmark.py`。

```bash
cmake -S tools/ospp/benchmark -B artifacts/local-loan/build-benchmark \
  -DCMAKE_PREFIX_PATH="$PWD/artifacts/local-loan/install"
cmake --build artifacts/local-loan/build-benchmark --parallel 1
python3 tools/ospp/benchmark.py \
  --binary artifacts/local-loan/build-benchmark/datasharing_benchmark \
  --output artifacts/local-loan/results/benchmark-new --rounds 3
```
