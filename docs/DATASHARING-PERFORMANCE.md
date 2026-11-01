# DataSharing 性能（f32da83 重测未完成，2026-11-01）

**本次未取得有效的版本性能对照。** 按原方法准备了 `0bbeaed` 与 `f32da8390b0af07500f67eb1d0a1279a9c968f4f` 的库和同一 benchmark driver，但新增外部进程监控器误判了本任务刚正常退出、等待回收的 zombie Writer，主动中止了基准组。基准仅完成第一轮 **9/27**，第 10 项被中止；f32da83 组 **0/27**，没有执行。按 TASK 的“失败不重试”约束未重跑，不能给出两组三轮中位数、九组版本差值或新增锁影响结论，也不能写成“没有影响”。详见 [本次未完成报告](../../../artifacts/datasharing-perf-f32da83/REPORT.md)、[原始测量记录](../../../artifacts/datasharing-perf-f32da83/measurement.json)及[独立中止诊断](../../../artifacts/datasharing-perf-f32da83/measurement-failure-audit.json)。

构建与 ABI 核验已通过：本机 jobs=1、nice=19、CPU 亲和 `[0,1]`、编译地址空间上限 512 MiB，沿用 `-O3 -g0 -DNDEBUG` 和原优化/GC 参数。SHA 核验复用旧对象，重新链接的基准库与上一轮库逐字节一致（SHA `fdf017da754b7be7db5440f2dfc87f6aaea6cd2c6a679f630676c1551118df88`）；只重编 ReaderProxy（2.937 s）后生成目标库（SHA `7ed3c24c732b236a4a89ec0aa44dfae0778487f3a5795e1dfbab1f57fc44dda6`）。两版公共头文件、5444 个导出符号的名称/类型相同，类布局未改；driver 两组计划使用相同字节，SHA `9ec972c7ee39250d5b23297287ac9dc7cae388cc1038c1a7b16e076e3b33693d`。私库 ldd/ELF 核验均通过，已运行基准进程的实际映射、nice/亲和也吻合。旧依赖历史缺失的 thirdparty/系统 SHA 无法补证，未冒称历史冻结完整。[构建](../../../artifacts/datasharing-perf-f32da83/build-records.json)与[依赖审计](../../../artifacts/datasharing-perf-f32da83/dependency-reuse-audit.json)分别保存依据。

原 runner `tools/ospp/benchmark.py`、driver、生产代码和测试均未修改；RELIABLE、KEEP_LAST16、64 槽、batch=8、warmup=32、计量 256/256/128 条和三轮模式轮换不变。实测在本机真实内核执行，监控采样没有发现编译或其他 DDS 测试；触发中止的 PID 经祖先链及原始结果确认就是本任务的 Writer。9 项已完成结果的原始载荷/CDR 检查通过，第 10 项没有自然退出结果，不能推断后续或目标组会通过。错误在新增证据脚本 `measure.py`，没有发现可据此确认的产品失败，也未修改产品或测试。

原始中止记录保留退出 `-9`、非超时，以及两个新增共享目录；目录内容先归档到本次 `aborted-shm/`，确认记录的任务进程退出后仅清理这两个本任务目录。清理后 `/dev/shm` 相对初始清单无新增，三个保护目录共 9685 项路径及 lstat 元数据核验不变，见 [清理记录](../../../artifacts/datasharing-perf-f32da83/cleanup.json)。未使用 Modal，费用 **$0**，无本任务 Modal app；未新增 swap。本次文档提交不改变既有完整验收证据；下文保留上一轮与 10/28 的完整历史数据，不能当作 f32da83 的性能成绩。

## 上一轮已完成测量（2026-11-01，布局 v7 / discovery v5）

当前受测实现为 `0caef22`，包含 `0ce433d`、`a8ba0ef`、回放修复 `c78607d`，以及 Reader 发布状态和 Writer 过滤器快照的竞态修复。本轮没有为改善数字调整性能参数。新原始证据位于 `artifacts/datasharing-followups-20261101/performance-verified/`；首轮 `performance/` 结果仅作历史补充。

构建采用 `-O3 -g0 -DNDEBUG`。首轮已重新构建生产全库和 v7/v5 benchmark driver；本次逐文件 SHA 核对 303 项源码/头文件及构建输入，确认仅 `src/dcps/publish/datawriterimpl.cpp` 改变，按 SHA 验证复用 95 个对象，只重编该翻译单元（6.704 s）并重新链接生产库（0.222 s）。driver 源码和 ABI 未变，按 SHA 复制首轮新 driver，没有重新编译，也没有使用历史 ABI driver 加载新库。`build-records.json` 保存对象身份和命令；当前库 SHA 为 `fdf017da754b7be7db5440f2dfc87f6aaea6cd2c6a679f630676c1551118df88`，driver SHA 为 `9ec972c7ee39250d5b23297287ac9dc7cae388cc1038c1a7b16e076e3b33693d`。历史 driver/匹配库另行复制到新目录，SHA 与 10/28 记录一致。

本机为同一 2 vCPU / 1.9 GiB Ubuntu 共享 VM，jobs=1、nice=19、CPU 亲和性 `[0,1]`；编译地址空间上限 512 MiB，未新增 swap 或支出。计量未与本任务的编译或其他 DDS 测试重叠。北京时间 11/1 01:47:44–01:47:58 运行历史程序，01:47:58–01:48:11 运行当前程序；1 分钟系统负载依次为 0.33、0.52、0.49。`measurement.json` 补充 summary 未记录的 nice、负载、内存、私有动态库解析及前后 `/dev/shm` 清单；两组均无新增残留。10/28 的 nice 未记录，不能宣称跨日调度条件完全相同。

方法保持不变：独立 Writer/Reader exec，RELIABLE、KEEP_LAST16、64 槽、每 8 条确认；预热 32 条，64/4096 B 计量 256 条，65536 B 计量 128 条；三模式轮换三轮，每组 27 次。正式计量不重试失败的 loan/write。吞吐包含应用填充、完整校验、控制和调度；延迟从 write 前到 take 后、校验前，nearest-rank 分位数再取三轮中位；RSS 取三轮最大值。`current/summary.json` 与 `historical-today/summary.json` 均为 27/27 自然退出成功，载荷、CDR 计数、独立进程身份、配置、吞吐算术及全部 54 份原始日志逐条核对通过。

| 字节 | 模式 | 10/28 吞吐 | 历史程序同日吞吐 | 当前吞吐 samples/s | 相对同日变化 | 当前 p50 / p95 / p99 μs | Writer / Reader RSS KiB |
|---|---|---:|---:|---:|---:|---|---|
| 64 | rtps | 29378.70 | 26602.70 | 25109.20 | -5.61% | 117.55 / 222.52 / 939.64 | 18048 / 8320 |
| 64 | ordinary | 8930.65 | 9415.49 | 43088.20 | +357.63% | 97.58 / 155.78 / 176.04 | 18048 / 8448 |
| 64 | loan | 8878.88 | 9169.20 | 49146.50 | +436.00% | 94.29 / 110.78 / 253.05 | 18048 / 8448 |
| 4096 | rtps | 21607.40 | 22485.00 | 21521.80 | -4.28% | 170.40 / 273.08 / 353.01 | 18132 / 8448 |
| 4096 | ordinary | 8445.79 | 8583.53 | 26074.50 | +203.77% | 149.19 / 212.93 / 222.85 | 18132 / 8704 |
| 4096 | loan | 8415.22 | 8422.95 | 22919.30 | +172.11% | 152.14 / 219.58 / 567.10 | 18048 / 8704 |
| 65536 | rtps | 71.11 | 71.37 | 71.01 | -0.50% | 816.99 / 198074.00 / 199917.00 | 18132 / 9216 |
| 65536 | ordinary | 4024.97 | 3977.99 | 5157.76 | +29.66% | 496.58 / 1092.92 / 1212.91 | 18132 / 12544 |
| 65536 | loan | 4118.62 | 4096.78 | 5199.56 | +26.92% | 521.56 / 1102.98 / 1508.35 | 18132 / 12544 |

完整差值（吞吐、三个分位、两个进程 RSS）见 [对历史记录](validation/20261101/performance-historical.json) 和 [对同日历史程序](validation/20261101/performance-today.json)，对应原始比较表为 `comparison-historical/comparison.json` 和 `comparison-today/comparison.json`。共享路径吞吐均提升；相对同日旧程序，64 B/4 KiB/64 KiB Loan 分别 +436.00%/+172.11%/+26.92%，相对 10/28 为 +453.52%/+172.36%/+26.25%。后续协议和数据面修改改变了旧管理锁、扫描短引用等固定成本，no-init 路径也避免完整载荷初始化；当前仍有完整历史链校验和多次原子元数据读取。本实验没有消融，不能把全部收益定量归给某次提交。

64 B RTPS 本次出现退化：吞吐相对同日 -5.61%、相对 10/28 -14.53%；p99 为 0.940 ms，分别 +255.92%/+356.40%。三轮 p99 为 0.940/1.726/0.246 ms，历史程序同日为 0.803/0.264/0.220 ms；当前计量耗时 14.033/10.196/9.321 ms，旧程序为 10.164/9.623/8.636 ms。每轮仅 256 条，p99 是第 254 个顺序统计量，既有 50 μs Reader 轮询、每批 8 条排队、完整校验及共享 VM 调度均参与计量。新增过滤器快照短锁可能影响普通 write/RTPS 的固定成本，但本轮没有消融或逐样本调度跟踪，不能把这些差异归因于该锁；退化已记录，具体占比尚未量化。

4 KiB RTPS 吞吐相对同日 -4.28%、相对 10/28 -0.40%；p99 为 353 μs，相对同日改善 43.11%，相对 10/28 增加 11.71%。当前三轮 p99 为 333/353/512 μs，同日旧程序为 661/621/218 μs。短测尾部波动使跨日和同日结论不同，不能只取改善的一组；缺少消融与调度时间线，尚不能区分这 4.28% 吞吐差异中的版本成本和宿主噪声。

64 KiB RTPS 为 71.01 samples/s，相对同日 -0.50%、相对 10/28 -0.15%；两组 p95/p99 仍约 198/200 ms。当前三轮计量耗时 2.002/1.803/0.818 s，同日旧程序为 1.794/2.197/1.202 s。源码 `Participant` 的实际心跳定时器每 200 ms 调用可靠 Writer 的 `WriteHeartbeat()`，丢失分片通过 heartbeat/ACKNACK/NACKFRAG 修复，与等待间隔吻合；没有单 flow 时间线，不能定量归因。首轮补充诊断 `performance/rtps-fragment-diagnostic/summary.json` 曾观察到历史/当时当前程序的 UDP `RcvbufErrors` 增加 10/9、耗时 1.600/1.784 s；这是旧轮网络命名空间总计数，不能代表本次或单 flow 的丢包/重传数量。

64 KiB Loan 的 p99 为 1.508 ms，相对同日改善 29.71%，相对 10/28 仍增加 25.59%；三轮为 1.188/2.529/1.508 ms。每轮 128 条的 p99 是第 127 个顺序统计量，包含批次、轮询与调度尾部成本；吞吐同日提升 26.92%并不意味着每个分位均提升。专机长测和逐样本调度归因尚未完成，本表仅描述本机三轮短测，不能作为最大可持续吞吐或生产 SLA。

首轮历史补充：`03fe395` 对应的 `performance/` 各组也为 27/27，其 64 KiB RTPS 为 63.98 samples/s（相对当轮同日旧程序 -29.95%），64 KiB Loan p99 为 2.049 ms（+17.33%）。该轮在最终过滤器竞态修复之前，保留原始证据供核查，当前结论采用上面的 `performance-verified/`，两轮差异本身也不能作为锁开销的因果证明。

以下保留 10/28 历史正文，便于复核，不替代上面的 v7/v5 当前测量。

# Boost DataSharing 历史性能（2026-10-28）

> 以下属于 10/28 布局 v5 冻结源码；当时的后续 v6/v4 轮未重新测量吞吐、延迟或 RSS，历史状态见 [10/31 报告](DATASHARING-PROGRESS-20261031.md)。当前 v7/v5 测量见上文。

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
