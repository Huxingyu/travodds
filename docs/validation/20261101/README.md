# 2026-11-01 验收摘要与编排快照

本目录保存现有证据的原字节副本，供代码审阅和结果定位，不代表重新执行。受测实现为 `f32da8390b0af07500f67eb1d0a1279a9c968f4f`，冻结规范身份为 `65a3cde4be51c42897dfd974a2eea661e4c74b06abd1d2a5d40a18810da82861`。当前入口见[验收矩阵](../../DATASHARING-ACCEPTANCE.md)和[测试说明](../../DATASHARING-TESTING.md)。

**原始日志不入库，只存在本机 `artifacts/`。** 下表原路径均相对仓库根目录；表内 SHA-256 同时适用于源文件和本目录副本。源 `artifacts/` 全部只读，复制时未改内容。副本内保留的绝对路径、相对路径、主机名和 PID 是历史执行记录，并非可直接在其他机器运行的配置；其中引用的日志、二进制和其余证据不随本目录分发。应从本页定位已入库文件，不能把副本报告里的每个历史链接视作仓库链接。

22 份新增副本逐个检查了私钥块、常见供应商令牌、HTTP 身份凭据、带口令 URL、JWT 和敏感字段赋值；对关键词命中另核上下文。pipeline 中的 `token` 是解析/校验变量，冻结清单中的相关词是依赖文件名，未发现凭据、密钥或账号令牌；本轮没有因此拒纳文件。单个文件均不超过 1 MiB，最大的是 `final/round2/source-manifest.json`（817892 字节）。

## 新增副本的来源和 SHA-256

`pipeline/` 是最终四模式证据编排代码；`final/` 是该轮审计、输入清单、费用估计与 statistics 分类摘要；`performance/attempt2/` 是后来独立完成的双基线性能重测；`crosshost/` 保存跨主机编排及结果的指定快照。它们保持各自执行身份，不能合并成一次新验收。

| 本目录文件 | 原路径 | SHA-256 |
|---|---|---|
| [pipeline/budget.py](pipeline/budget.py) | `artifacts/datasharing-final-20261101/pipeline/budget.py` | `e308d09c488e663b731dd02974eda36447f3684271c482e6f3a3ec70e5e100b4` |
| [pipeline/cleanup.py](pipeline/cleanup.py) | `artifacts/datasharing-final-20261101/pipeline/cleanup.py` | `b714b24e8d552227be139af56c8ea6259ae6c54abcb3aa302f5483d7eb212589` |
| [pipeline/coverage_stage.py](pipeline/coverage_stage.py) | `artifacts/datasharing-final-20261101/pipeline/coverage_stage.py` | `982bd5fa60059158af7d6d241a61497c9f64310a5ce7a1bf1bec0ac1d1a35281` |
| [pipeline/final_audit.py](pipeline/final_audit.py) | `artifacts/datasharing-final-20261101/pipeline/final_audit.py` | `166ec83dfb29ed2018973478d1099a1c684da3b5cd96a721caf4dbd2af8ed826` |
| [pipeline/freeze.py](pipeline/freeze.py) | `artifacts/datasharing-final-20261101/pipeline/freeze.py` | `b892a3ad20d0c02a836577cf7eb6ec0bbef4a96d98dae976ffac3df4cd8eef1c` |
| [pipeline/local_stage.py](pipeline/local_stage.py) | `artifacts/datasharing-final-20261101/pipeline/local_stage.py` | `5ea945fdd61648c565660980025ddffc954c4439958f7fd3b4c511b37d5793e3` |
| [pipeline/modal_worker.py](pipeline/modal_worker.py) | `artifacts/datasharing-final-20261101/pipeline/modal_worker.py` | `1b400e550eafa1a196f0a8544424f02cf13f164da6aae4b0c3b07ba156aeda0b` |
| [pipeline/worker.py](pipeline/worker.py) | `artifacts/datasharing-final-20261101/pipeline/worker.py` | `586e950b22c1ed0d432e2ac31855c1c8d9ade5cc2f82784cf5e74a12f9ab5066` |
| [final/round2/audit/final-audit.json](final/round2/audit/final-audit.json) | `artifacts/datasharing-final-20261101/round2/audit/final-audit.json` | `7f070f09fe0f251c634dc84633810a10b6c8f91f1abc209541506a4a21799efa` |
| [final/round2/source-manifest.json](final/round2/source-manifest.json) | `artifacts/datasharing-final-20261101/round2/source-manifest.json` | `f3557acd449183e880b10d8b5cc74f23fafd11be3f5ef35a2d6b33d33d967348` |
| [final/usage-ledger.json](final/usage-ledger.json) | `artifacts/datasharing-final-20261101/usage-ledger.json` | `07a9de10be9919afed993a228486d3656d8358259c01d4564bc0df551edae8be` |
| [final/statistics/attempt2/statistics-result/results.json](final/statistics/attempt2/statistics-result/results.json) | `artifacts/datasharing-final-20261101/statistics/attempt2/statistics-result/results.json` | `7502cbc3313e4f4f4596949161e9362961e5b3d5566be3a9ae38162bce3fc6f5` |
| [performance/attempt2/REPORT.md](performance/attempt2/REPORT.md) | `artifacts/datasharing-perf-f32da83/attempt2/REPORT.md` | `e4d26c79939f85c683f17ca8209e0093455700cc4b1cc926ad0182f78204414a` |
| [performance/attempt2/comparison-three-groups.json](performance/attempt2/comparison-three-groups.json) | `artifacts/datasharing-perf-f32da83/attempt2/comparison-three-groups.json` | `f5da0ff7d063222de5cc923e0c684dcd9804ace0807ae07d5f7f075e32f8f29b` |
| [crosshost/xhost.cpp](crosshost/xhost.cpp) | `artifacts/crosshost-20261101/xhost.cpp` | `68527ec87669ca191d1cb8c2ac9a96427f459c1f970c22ba65b508160ac2cd7f` |
| [crosshost/tapbridge.py](crosshost/tapbridge.py) | `artifacts/crosshost-20261101/tapbridge.py` | `66df266ef70d5d5e435cf44053d962e5935b32b089838e5eef748cdfa7d92380` |
| [crosshost/eval4090/REPORT.md](crosshost/eval4090/REPORT.md) | `artifacts/crosshost-20261101/eval4090/REPORT.md` | `7218884bb6d3aef7f72126452bfb822fbe3b745e9784a4a8211a1b41788e9f28` |
| [crosshost/eval4090/results.json](crosshost/eval4090/results.json) | `artifacts/crosshost-20261101/eval4090/results.json` | `3c64d6fb1aef1fefe9459fe92b901b37f9d010c63ad7174ce5e770784d42c4a8` |
| [crosshost/eval4090/results-independent-review.json](crosshost/eval4090/results-independent-review.json) | `artifacts/crosshost-20261101/eval4090/results-independent-review.json` | `383c56fb37c8f10b3773b3352410200d9f427f8315483d6f47f7dec85da91d40` |
| [crosshost/eval4090/cleanup.json](crosshost/eval4090/cleanup.json) | `artifacts/crosshost-20261101/eval4090/cleanup.json` | `9cf382b4e8df42d725fbca4a632cbc76cabf650ec9ca8ed9b77a0e4b0ab87183` |
| [crosshost/eval4090/ssh_l2_bridge.py](crosshost/eval4090/ssh_l2_bridge.py) | `artifacts/crosshost-20261101/eval4090/ssh_l2_bridge.py` | `fbfe10f0b6fb9d4a5efd9bb0019d5df8fbd229a58126959a3dd875be99a4599e` |
| [crosshost/eval4090/run_cases.py](crosshost/eval4090/run_cases.py) | `artifacts/crosshost-20261101/eval4090/run_cases.py` | `2a0754995a1f453e104ff9f8fdc9d95101715d2790d7ca6fa8ea0bc530fe4c2e` |

`crosshost/xhost.cpp`、`crosshost/tapbridge.py` 是初始方案快照，原 Mac 路线未完成测试；不能将它们冒称为 eval4090 通过时的最终驱动源码或桥。实际通过的 SSH 二层桥和运行器是 `crosshost/eval4090/ssh_l2_bridge.py`、`run_cases.py`；该目录报告记录受测库 SHA，JSON 保留计数、退出及日志审核摘要。最终驱动源和逐端运行时身份记录 `artifacts/crosshost-20261101/eval4090/{xhost.cpp,runtime-provenance.json,driver-build.json,driver-audit.json}` 未在本轮指定纳入清单中，仍属本机证据，不随仓库分发。

## 已有性能摘要（原字节保留）

以下两个 JSON 在本次前已入库，本轮未修改；已与对应本机原文件重新核对 SHA。它们属于较早的 `0caef22` / `0bbeaed` 性能记录，不是 f32da83 的重测成绩。

| 本目录文件 | 原路径 | SHA-256 |
|---|---|---|
| [performance-today.json](performance-today.json) | `artifacts/datasharing-followups-20261101/performance-verified/comparison-today/comparison.json` | `3171b940917dc2233e4bb8ad878a9e5498813bb0aaf2f48606cbf8374d8b04f7` |
| [performance-historical.json](performance-historical.json) | `artifacts/datasharing-followups-20261101/performance-verified/comparison-historical/comparison.json` | `c149a8b57e710f2a0d83fb0cc194c9683d583cbc96b804ac95c816559df5125f` |

## 示例 README 的输入身份例外

原 `final/round2/source-manifest.json` 记录 5609 项输入，虽然策略说明排除文档，`git archive` 仍带入 `example/cpp/data_sharing/README.md`。该文件不参与编译；本轮只更新示例运行状态，没有改变生产源码、测试或构建工具。

| 文件状态 | SHA-256 |
|---|---|
| f32da83 受测文件及冻结清单中的旧值 | `42f8e0426decac83ad928e99dd09c22b3f856dd0a9372c623370017659a79480` |
| 本轮文档修改后的文件 | `482b50cc6f7dd90d4a412b2cefc18ed871754d27f17c98bdffea75582fd2511f` |

本目录清单及规范身份仍描述 f32da83 当时的输入，保持原字节、原数值；不会为本次文档变更改写清单或重算历史身份。新 HEAD 的该 README SHA 已不同，不能声称新 HEAD 与清单的全部 5609 项逐字节相同。命令和覆盖范围见[测试说明](../../DATASHARING-TESTING.md)。
