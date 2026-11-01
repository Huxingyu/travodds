# TravoDDS DataSharing 补齐与验收报告（2026-10-30）

> 历史快照：本报告对应 discovery v2、带键 key-only 钩子可选且仍容许旧原生全样本快照的阶段。484 项、93.839%、两版各 41 项及其哈希只证明该冻结版本。当前严格版本为共享布局 v6 / discovery v3，带键 native 强制有效 key-only 元数据，已删除原生全样本快照回退；实现与新验证见 [严格准入报告](DATASHARING-STRICT-20261030.md) 和 [当前验收矩阵](DATASHARING-ACCEPTANCE.md)。

本轮补齐带键原生样本的 key-only 本地快照、Reader 首次快照失败回滚，并重新验证当前生产代码。Ubuntu 上共享池 37 项、payloadpool/Loan 200 项、核心 247 项，共 **484/484 通过**，失败、错误和禁用均为 0；三个有限示例各 3 轮，共 9 次示例执行通过。新增生产可执行行覆盖率为 **93.839%（2757/2938）**，达到官方 ≥70% 要求。本次 ASan/UBSan 与 TSan 分别完整插桩生产库、GTest、共享池及 DDS/key-only 重点回归，**各 41/41 通过**。

实际工作树为 `/home/ubuntu/project/travodds/.worktrees/datasharing-progress`，分支 `work/datasharing-progress`，基点 `3da8e077b9902142cdb4549eafa5b43114bdf936`；新增行覆盖率比较基线仍为 `489067108405b27afbd369245542f5e88d457f18`。本报告对应当前未提交改动，没有提交、推送或新 PR。官方八项交付的实现与验证对应关系见 [验收矩阵](DATASHARING-ACCEPTANCE.md)。共享布局 v6、发现能力 v2 的协议依据及前阶段回归见 [生产后端报告](DATASHARING-PRODUCTION-20261030.md)，该报告旧计数不能代替本次结果。

## 本轮实现

TypeSupport 新增可选的 `get_native_key_info()`、`capture_native_key(snapshot, sample)` 和 `restore_native_key(sample, snapshot)`。`NativeKeyInfo` 声明独立的键存储大小和对齐；Writer 注册及 Reader 首次接受 ALIVE 时只分配并保存键字节，不再为了键查询复制完整原生样本。示例和正式测试类型将 64 字节样本的快照缩为 8 字节键。快照不保留源样本或共享槽位引用，归还 Loan 后槽位可以复用，实例键仍可查询。

capture 只能读取键字段，restore 只能修改输出的键字段，非键字段保持调用前值。快照必须独立、无指针且无需构造/析构；元数据在 TypeSupport 生命周期内稳定。大小/对齐非法时拒绝，钩子 false 或标准异常按失败处理，显式启用后不回退完整样本复制。默认钩子保留旧 TypeSupport 的全样本快照源码兼容行为，此兼容分支不属于严格带键零载荷复制范围。严格端到端范围要求 Writer 与 Reader 两端的 TypeSupport 都实现钩子；这项本地元数据不参与发现协商，Writer 启用不能证明匹配 Reader 也启用。具体参数和错误码见 [API 手册](DATASHARING-API.md#带键实例与本地快照)。

Writer 捕获失败不留下已注册实例，不消费应用 Writer Loan，修正后可以重试。Reader 首次捕获失败在排序、过滤、淘汰和所有权状态变更前拒绝本条准入，回滚刚创建的空实例，不占 `max_instances`；已有实例与历史不因失败删除。已有键快照还原失败保留快照，修正后可再次查询。

新增钩子改变 TypeSupport 二进制 ABI，**库、应用及自定义 TypeSupport 子类必须一起重新编译**，不能只替换动态库。NativeKeyInfo 属于进程本地元数据，不再改变共享布局或发现能力版本。

三项新增正式回归已包含在当前 200 项 payloadpool/Loan 测试中：

| 回归 | 核心断言 |
|---|---|
| `LoanIntegration.KeyedNativeCaptureFailurePreservesLoanAndRollsBackRegistration` | 四种非法元数据、捕获 false/标准异常均保留 Loan 且不注册；成功后只还原键；完整样本构造、copy_data 和载荷 CDR 不参与键快照 |
| `LoanIntegration.KeyedNativeReaderCaptureFailureDoesNotConsumeInstanceCapacityOrRetainSource` | `max_instances=1` 时连续失败不留下已知键、不保留源样本，随后有效实例仍可准入 |
| `NativeReaderIntegration.KeyedNativeKeySnapshotSurvivesSingleSlotReuseAndWriterDeletion` | 唯一共享槽位归还后可覆写，键查询仍返回旧实例键且不修改非键字段；删除 Writer 后快照仍可查询 |

## 当前验证与证据

证据根目录为 `/home/ubuntu/project/travodds/artifacts/completion-20261030/`，以下路径相对于此目录。运行器记录命令、退出、资源、XML、317 个源码/构建输入 SHA-256 与实际动态库 SHA-256，并在结束时复核输入和库未变；文档和 README 不参与构建身份。全部编译串行执行，编译进程地址空间上限 768 MiB。当前汇总审计入口为 final-verification.json：`/home/ubuntu/project/travodds/artifacts/completion-20261030/final-verification.json`（本机证据，不随仓库分发），`passed=true`。该快照生成于 `2026-10-30T12:24:53.152984Z`：三套 317 项冻结输入一致，每套 68 个纳入清单的安装公共头与源码一致，10 个二进制均链接各自的私有库；当时 `/dev/shm/travodds*` 为空，证据根目录下没有仍在执行的测试程序。审计逻辑见 final-audit.py：`/home/ubuntu/project/travodds/artifacts/completion-20261030/final-audit.py`（本机证据，不随仓库分发），只核对现有产物，不重编或重跑测试，并拒绝覆盖已有汇总。

| 验证 | 当前结果 | 证据 |
|---|---|---|
| 共享池 | 37/37 通过 | `coverage-final/sharedpayloadpool_test.xml` |
| 普通池及 DDS/Loan | 200/200 通过，含三项新增 key-only 回归及真实 DDS 控制锁占用回归 | `coverage-final/payloadpool_test.xml` |
| 既有核心 | 247/247 通过 | `coverage-final/core.xml` |
| Loan/keyed/mixed 有限示例 | 各 3 轮，共 9 次示例执行；独立进程自然退出、载荷/CDR/Loan 保活及共享目录清理检查通过 | `coverage-final/examples/summary.json`，生成于 `2026-10-30T12:12:31Z` |
| 新增生产行覆盖率 | 2757/2938 = 93.839%，138 个翻译单元，变更生产翻译单元无漏采集 | `coverage-final/coverage/summary.json` |
| 本次 ASan/UBSan | 生产库、GTest、测试驱动完整插桩；37 项共享池 + 4 项实际 DDS/key-only 重点回归，共 41/41 通过，失败/错误/禁用均为 0 | `asan-final/results.json`、`asan-final/sharedpayloadpool_test.xml`、`asan-final/native_dds_progress.xml`；LSan 关闭 |
| 本次 TSan | 生产库、GTest、测试驱动完整插桩；37 项共享池 + 4 项实际 DDS/key-only 重点回归，共 41/41 通过，失败/错误/禁用均为 0，未报告数据竞争 | `tsan-final/results.json`、`tsan-final/sharedpayloadpool_test.xml`、`tsan-final/native_dds_progress.xml`；实际运行使用 `setarch x86_64 -R` |

常规回归与覆盖率来自同一份 coverage 插桩构建，不是使用旧库复跑。ASan/UBSan 与 TSan 新构建分别使用独立安装前缀，二者的 317 项源码/构建输入哈希均与 coverage 版本完全一致，提取的 4 项 DDS/key-only 驱动哈希也相同；生产库、GTest 及重点测试驱动均插桩，未复用旧未插桩库。覆盖率合并生产库、测试和示例翻译单元中的模板实例，分母为相对固定基线新增的 GCC 可执行生产代码行，排除测试、依赖和文档；它不是分支覆盖率或完成度。检查版 focused 范围通过，不等于在 sanitizer 下执行全部 484 项；LSan 因此前已独立确认的 ptrace 环境限制关闭，不能据 ASan/UBSan 声称无泄漏。

| 本次产物 | SHA-256 |
|---|---|
| `coverage-final/results.json` | `478b7711f1bd070260f5a748c8496ea0b1cea2135573ccf98ca7f28a295c2614` |
| `coverage-final/coverage/summary.json` | `548c1171af7f5341c359156d00d95daf26e659b89d39a2eaa9ff3edfa4b826e4` |
| `coverage-final/examples/summary.json` | `245a84be478fb715a9b4c3fc1187ee1f592aa6c49250457b05c324c8f71cb5e0` |
| `build-coverage/install/lib/libtravodds.so.1.1.0` | `36ac47b50acfa30d7369d7d63eb85fee4b6030787ac5ffaf99fcf61bfa025a12` |
| `asan-final/results.json` | `cbef8e6ae36db324755a1c8a344275d8ebd06fbd888197f8a47fa45b5ffe43e1` |
| `build-asan/install/lib/libtravodds.so.1.1.0` | `acd7e4d9e5f8f8bbf69ed958e8d58a6675593057529b021bbbc770d6b104b0f1` |
| `asan-final/native-dds-progress.cpp`、`tsan-final/native-dds-progress.cpp` | `2e837e48a670867180c86023b5978b98e9860941c1277eb8a5a338e4ca53804d` |
| `tsan-final/results.json` | `5251d229bc0692d22c742863af5195c16190675fe1f5791d00317462f4694e6b` |
| `final-verification.json` | `292062039d6f43dedb6c81b6e6162a374df1556f34e4286c2ee6ebbd17475806` |
| `final-audit.py` | `8d2664500256f5d7fc0b683456200328b09b9656b6dc4be2e480c15c851a06b1` |
| `build-tsan/install/lib/libtravodds.so.1.1.0` | `6e14beeb29e986b3c739111cf808feb42f22f091daeb0fd16d9c46a20c7a4db4` |

逐文件源码哈希保存在 `coverage-final/results.json` 的 `source_sha256` 中；该对象按键排序并以紧凑 JSON 编码后的 SHA-256 为 `a361eaeddeb8a10b7b39f6bbfffcca39e088d5c18f4820d720291746fbfd492b`。2026-10-28 的 94.118% 仅对应旧布局 v5，本次覆盖率从当前代码重新采集。

## 复现

在同一源码版本上使用新的证据目录与私有构建目录，串行执行：

```bash
python3 tools/ospp/production_lockfree_verify.py \
  --output /absolute/new-evidence \
  --build-dir /absolute/private-build \
  --mode coverage --core --examples 3 \
  --baseline 489067108405b27afbd369245542f5e88d457f18
```

本次检查版分别使用 `--mode asan --scope focused` 和 `--mode tsan --scope focused`，额外指定以下重点用例；真实 DDS 控制锁占用用例由 focused 模式默认加入：

```bash
--focus-test KeyedNativeCaptureFailurePreservesLoanAndRollsBackRegistration \
--focus-test KeyedNativeReaderCaptureFailureDoesNotConsumeInstanceCapacityOrRetainSource \
--focus-test NativeReaderIntegration.KeyedNativeKeySnapshotSurvivesSingleSlotReuseAndWriterDeletion
```

每种模式使用独立的新 output/build-dir。本轮所有 build-dir 位于同一个 completion 根目录，按顺序编译和运行。脚本拒绝覆盖已有证据；锁文件为 build-dir 父目录下的 compiler.lock，仅防止同一父目录内的构建同时编译，不是跨目录的全局锁。TSan 使用 `setarch x86_64 -R`，不修改系统 sysctl。失败日志和退出码保留，不能将尚未运行的阶段或旧阶段结果记成本次通过。

## 验收范围

官方要求跨进程通知，并在描述中提出无锁通知；没有要求整个 DDS 写入路径无锁。当前共享数据面的预留、发布、借用/归还及 ready 快照不获取跨进程管理 mutex，控制注册/维护仍保留 robust mutex，本地 Writer/history/发现锁和 futex 系统调用仍存在。控制锁被活进程占用不阻止已有数据面，不构成整个 DDS write 形式化 lock-free 或 wait-free 证明。

严格零载荷复制范围为显式兼容的无指针固定 plain+bounded 类型的 Writer Loan → Reader Loan；带键类型另需 Writer 与 Reader 两端的 TypeSupport 各自显式实现 key-only 钩子。普通裸指针 write、预分配输出、控制样本复制及旧类型全样本键快照不在此范围，不承诺任意有界 C++ 容器可直接共享。

旧发现版本、不兼容类型与建立映射前的失败保留 RTPS，暂时映射/注册失败可重试；已经损坏的 native session 仍需新会话重建，不能概括为任意运行时故障透明回退。死亡短操作计数无法安全归属时保守标记 CORRUPT，旧 Loan 保留映射及字节。Ubuntu 满足官方“Ubuntu 或 openEuler”的平台要求；长期压力、穷尽线程/丢包矩阵、跨平台部署及同条件性能比较不由这次功能验收替代。历史无栈 WaitSet 崩溃仍未归因。
