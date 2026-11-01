# DataSharing 测试指南

当前验收绑定受测代码 `f32da8390b0af07500f67eb1d0a1279a9c968f4f`、共享布局 v7 / discovery v5。数量与结论来自 [最终审计](validation/20261101/final/round2/audit/final-audit.json)，范围以 [八项验收矩阵](DATASHARING-ACCEPTANCE.md) 为准；本页命令是后续新构建的入口，本次文档修改没有重新运行测试。

## 程序与计数

| 源目录 / 程序 | 11/1 唯一用例数 | 覆盖内容及官方交付 |
|---|---:|---|
| `test/unit_inside` → `unit_test` | 247 | 既有 DDS core、QoS、发现、收发与条件等回归，检查新路径对现有行为的影响 |
| `test/unit_payloadpool` → `payloadpool_test` | 259 | PayloadPool、Loan、native 准入/载荷保护、Reader 接入、通知进展、发现/QoS/RTPS 回退及并发回归，对应第 1、3–6 项 |
| 同目录 → `sharedpayloadpool_test` | 49 | 共享段、发布历史环、回放/租约、回收、关闭和跨进程通知，对应第 2–3 项 |
| `example/cpp/data_sharing` → `datasharing_loan`、`datasharing_keyed`、`datasharing_mixed` | 9 次有限运行 | 每个程序 3 次，校验完整收发、CDR/Loan 计数和资源生命周期，对应第 4–7 项 |
| `test/unit_extra` → 其独立构建目录中的 `unit_test` | 不计入本次验收 | 上游 xml、cdr、xtypes、security、common_log 共 5 个模块；其原 README 保留上游范围和路径 |

259 项全部由一个 `payloadpool_test` 聚合程序执行。Linux CMake 还生成 `native_initialization_test`、`native_payload_writes_test`、`reader_payloadpool_test`、`reader_binding_test`、`native_key_admission_test`，它们复用聚合程序中的测试源，不能再加入 259 的唯一身份数。CMake 的 CTest 列表可能同时包含这些重复子集；总验收数按指定聚合程序和 XML 用例身份计算。测试与覆盖率证据支持第 7 项，设计和本页等使用文档支持第 8 项。

## 构建和普通运行

在仓库根目录执行下列命令，并把路径改为新的私有输出目录。需要 Linux（futex、`/proc` 和可写 `/dev/shm`）、支持 C++17 的编译器、CMake/CTest ≥3.20（项目最低为 CMake 3.16，本页命令使用 `ctest --test-dir`）、make、Python ≥3.9、Git、GNU `timeout`；GCC 覆盖率另需同版本 `gcov`，TSan 包装另需 `setarch`。11/1 证据使用 Ubuntu 24.04、g++ 13.3。Boost 1.88、Asio 等头文件和 `test/googletest-1.17.0.zip` 已随仓库提供，GTest 由 CMake 从该本地 ZIP 构建。

本机只有少量 CPU/内存，构建和测试均串行、nice 19；多程序并发还会相互干扰发现、共享内存及时间敏感用例。不同插桩模式使用不同目录，库、测试、示例和 TypeSupport 一起重编，不混用旧库或旧计数。

```bash
DS_RUN=/absolute/new-private-run
BUILD_JOBS=1 nice -n 19 bash tools/ospp/bootstrap.sh "$DS_RUN"

nice -n 19 ctest --test-dir "$DS_RUN/build-unit_inside" \
  --parallel 1 --output-on-failure
nice -n 19 "$DS_RUN/build-unit_payloadpool/payloadpool_test"
nice -n 19 "$DS_RUN/build-unit_payloadpool/sharedpayloadpool_test"
```

`bootstrap.sh` 构建库、私有安装前缀、示例和两个测试目录，不执行完整测试。`unit_inside` 是目录名，可执行文件名是 `unit_test`。用 GoogleTest filter 单独检查一个用例，或用 CTest 选择同一组：

```bash
nice -n 19 "$DS_RUN/build-unit_payloadpool/payloadpool_test" \
  --gtest_filter=PayloadPool.LegacyPoolReusesReturnedStorageThroughInterface
nice -n 19 ctest --test-dir "$DS_RUN/build-unit_payloadpool" \
  --parallel 1 --output-on-failure -R '^ReaderProxyRequests\.'
```

需要保存普通全量 XML/log 时，可替代上面的完整执行步骤，使用 `nice -n 19 python3 tools/ospp/accept.py --prefix "$DS_RUN" --output /absolute/new-normal-results --mode normal --stage full`；该输出目录必须尚不存在。

## Sanitizer、覆盖率与有限示例

下面也是后续执行入口，不是声称这些脚本会自动重建 11/1 的完整远端/本机流水线。

```bash
DS_ASAN=/absolute/new-private-asan
ASAN_DETECT_LEAKS=0 nice -n 19 bash tools/ospp/sanitize.sh "$DS_ASAN"

DS_TSAN=/absolute/new-private-tsan
TSAN_NO_ASLR=1 nice -n 19 bash tools/ospp/tsan.sh "$DS_TSAN"
```

`sanitize.sh` 用 ASan+UBSan 重编并运行聚合模块和共享池；默认不含 core。`CORE_TESTS=1` 可额外构建/运行 core，但 11/1 的 ASan/TSan 证据没有这 247 项，不能计为已通过。`ASAN_DETECT_LEAKS=0` 关闭 LSan，仍保留 ASan/UBSan 检查，不能称为泄漏扫描。

`tsan.sh` 重编库和模块，实际只执行脚本指定的 timer/logger 与 QoS focused 重复检查，不等于完整 308 项。若在支持 `setarch -R` 的真实内核上执行完整聚合程序，可继续串行运行：

```bash
TSAN_OPTIONS=halt_on_error=1:second_deadlock_stack=1 \
  nice -n 19 setarch "$(uname -m)" -R "$DS_TSAN/build-tests/payloadpool_test"
TSAN_OPTIONS=halt_on_error=1:second_deadlock_stack=1 \
  nice -n 19 setarch "$(uname -m)" -R "$DS_TSAN/build-tests/sharedpayloadpool_test"
```

`TSAN_NO_ASLR=0` 适用于不需要该地址布局规避的宿主；不要把包装器失败写成测试通过。它只更改当前进程，不改全局内核配置。

覆盖率必须在同一编译身份的新 `.gcno/.gcda` 上收集，使用与编译器匹配的 `gcov`。通用本机构建入口可用 `COVERAGE=1 ASAN_DETECT_LEAKS=0 CORE_TESTS=1 nice -n 19 bash tools/ospp/sanitize.sh /absolute/new-private-coverage`；它是 ASan+UBSan 加覆盖率构建，不能冒充 11/1 独立 coverage 模式。提取新增生产可执行行的命令参数为：

```bash
python3 tools/ospp/coverage.py --source "$PWD" \
  --build-root /absolute/new-private-coverage \
  --output /absolute/new-coverage-report --gcov gcov \
  --baseline 489067108405b27afbd369245542f5e88d457f18 --minimum 70

nice -n 19 python3 tools/ospp/datasharing_examples.py \
  --build "$DS_RUN/build-data_sharing" \
  --output /absolute/new-finite-examples-results --rounds 3
```

`coverage.py` 统计相对指定基线新增的 `src/`、`include/` 中 GCC 可执行行，未运行的翻译单元也进入分母，负计数按未覆盖处理；这不是分支覆盖率或全仓库覆盖率。示例检查器核对两进程自然退出、固定条数、CDR/Loan 和共享目录清理；`--rounds 3` 是三个程序各 3 次，共 9 次，输出目录必须尚不存在。

## 性能入口

性能测量与正确性验收分开。先以与生产库一致的配置构建 driver，再串行调用 runner：

```bash
cmake -S tools/ospp/benchmark -B "$DS_RUN/build-benchmark" \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$DS_RUN/install" \
  -DCMAKE_CXX_FLAGS_RELEASE='-O3 -g0 -DNDEBUG'
nice -n 19 cmake --build "$DS_RUN/build-benchmark" --parallel 1
taskset -c 0,1 nice -n 19 python3 tools/ospp/benchmark.py \
  --binary "$DS_RUN/build-benchmark/datasharing_benchmark" \
  --output /absolute/new-benchmark-results --rounds 3
```

每组 3 大小 × 3 模式 × 3 轮共 27 次；参数、样本数、轮换顺序和 CDR 校验由 runner 固定。11/1 的最终比较实际采用 `0bbeaed → f32da83 → 0bbeaed` 三组、81/81 自然退出，复用经 SHA 核对的同一 driver 与两私库，完整条件及 4 KiB ordinary 退化未归因结论见 [性能报告](DATASHARING-PERFORMANCE.md)。不要根据此处通用构建示例宣称已复现该对照，也不并行测量或为了数字重跑失败组。

## 11/1 最终证据与边界

| 模式 | Modal 执行 | 同字节产物在本机真实内核执行 | 唯一用例数 |
|---|---|---|---:|
| plain | 聚合模块 259 | 共享池 49 + core 247 | 555 |
| coverage | 聚合模块 259 | 共享池 49 + core 247，另有有限示例 9 次 | 555 |
| ASan+UBSan | 聚合模块 259 | 共享池 49 | 308 |
| TSan | 聚合模块 259 | 共享池 49 | 308 |

四模式从同一 `f32da83` 冻结输入独立构建，Modal 为 2 CPU/4096 MiB、Ubuntu 24.04/g++ 13.3；本机补跑使用相同 ELF/私有库 SHA。四项 NativePayloadWrites proof 在 Modal 和本机各执行，重复身份不加到用例总数。覆盖率为 **95.556%（3096/3240，144 个生产翻译单元）**。ASan/TSan 的预设范围是模块/共享池集成，core 只在 plain/coverage 验证；没有据此证明 core 的 sanitizer 结果。

gVisor 缺少部分 `/proc/.../syscall`、`wchan` 观测和 IPv6，且网络时序不同，所以完整共享池和 core 被分配到本机；真实竞争不能归为环境限制。Modal 的 TSan 实际没有使用 `setarch -R`，本机 TSan 使用该进程级包装；LSan 关闭。完整构建、回传、用例身份和覆盖率合并规则见 [最终报告](DATASHARING-PROGRESS-20261101.md#四模式全量证据实际完成)及[归档流水线](validation/20261101/README.md)。这些流水线副本保留原本机路径，原始日志不入库，仅存在本机 `artifacts/`。

跨主机补充使用 VM TAP 与另一 Linux 主机的 veth/AF_PACKET，通过 SSH 承载虚拟二层，先验证单播/组播，再执行同机 native 控制与双向 AUTO/ON × ordinary/Writer Loan × 3 大小共 24 组，均通过。它验证不同 boot_id 下的 RTPS/UDP 回退，不是物理局域网、OFF/mixed 全组合或性能结论。见 [跨主机证据](validation/20261101/crosshost/eval4090/REPORT.md)和[范围说明](DATASHARING-ACCEPTANCE.md#跨主机-rtps-回退补充2026-11-01)。

已知未归因现象、软件载荷保护、无锁范围、DRAM 写入次数和类型契约边界直接见 [未解决现象与证明范围](DATASHARING-PROGRESS-20261101.md#未解决现象与证明范围)与[原文严格范围](DATASHARING-ACCEPTANCE.md#原文严格范围与证据边界)，本页不扩大原结论。

## 文档修改与冻结输入身份

受测规范身份仍为 `65a3cde4be51c42897dfd974a2eea661e4c74b06abd1d2a5d40a18810da82861`，共 5609 项，详见 [原始输入清单](validation/20261101/final/round2/source-manifest.json)。清单策略写为排除文档，但 `git archive` 实际带入了不参与编译的 `example/cpp/data_sharing/README.md`。本次仅修正其中示例验收说明，因此 HEAD 上该文档 SHA 与历史清单不同：

| 文件 | SHA-256 |
|---|---|
| `f32da83` 清单中的示例 README | `42f8e0426decac83ad928e99dd09c22b3f856dd0a9372c623370017659a79480` |
| 本次文档修改后的示例 README | `482b50cc6f7dd90d4a412b2cefc18ed871754d27f17c98bdffea75582fd2511f` |

清单和受测身份保持原样，未用新文档 SHA 覆盖历史输入；它不表示 HEAD 全部 5609 项仍与旧清单逐字节一致。生产代码、测试、构建脚本和示例代码均未因此改变。
