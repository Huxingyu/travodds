# DataSharing 严格原生准入与验证（2026-10-30）

本轮将带键 key-only 从应用可选优化改为 native 准入条件：缺失或非法键元数据的类型不再进入 DataSharing，原生键快照的全样本 memcpy/copy_data 回退移除。共享布局保持 **v6**，发现能力升为 **v3**，防止仍允许旧带键全样本快照的 v2 对端加入严格 DS。对所有符合类型契约并实际选择 native 的端点对，库在 Writer Loan → Reader Loan 分支中都不复制完整用户载荷；键字段、索引、序列地址和控制元数据仍可复制，不作“任何字节都不复制”的承诺。

本报告对应工作树 `/home/ubuntu/project/travodds/.worktrees/datasharing-progress`、分支 `work/datasharing-progress`。新证据根为 `/home/ubuntu/project/travodds/artifacts/strict-native-20261030/`。首轮 `coverage-final` 因核心测试挂起失败，第二轮 `coverage-verified` 因既有 Ownership 用例的等待缺陷失败；最终当前入口 `coverage-complete/results.json` 已通过：**500/500 回归、三个示例各 3 轮共 9 次执行、新增生产可执行行覆盖率 93.920%（2765/2944，140 个翻译单元）**。本次 ASan/UBSan 与 TSan 独立完整插桩的 focused 检查版**各 57/57 通过**，最终汇总审计通过。此前 [可选 key-only 报告](DATASHARING-COMPLETION-20261030.md) 的 484 项、93.839%、ASan/UBSan 与 TSan 各 41 项及其汇总审计仅证明 discovery v2 冻结版本，不作为当前通过证据。官方八项对应关系见 [验收矩阵](DATASHARING-ACCEPTANCE.md)。

## 强制类型契约

TypeSupport 的 `supports_native_data_sharing(info)` 统一判断端点原生资格：NativeTypeInfo 必须满足固定、无指针、plain+bounded 的布局契约；带键类型还必须提供 `NativeKeyInfo::valid_for(info)`。Writer 与 Reader 均在 enable 时执行这一门控，具体键表示仍是各自的本地元数据，不需要共享同一快照布局。

| 类型或运行情况 | ON | AUTO |
|---|---|---|
| 无键类型，原生布局合法 | 可准备 DS | 尝试准备 DS |
| 带键类型，原生布局和 NativeKeyInfo 均合法 | 可准备 DS，应用承诺实现 capture/restore | 尝试准备 DS，承诺相同 |
| 带键类型，NativeKeyInfo 缺失或非法 | Writer/Reader enable 返回 UNSUPPORTED | enable 保留 CDR/RTPS，不广告有效 DS 能力；普通 write 使用 CDR，loan_sample 返回 UNSUPPORTED |
| 声明合法元数据，但 capture/restore 返回 false 或抛标准异常 | 实际捕获/查询失败，禁止原生全样本回退 | 已进入 native 的操作同样失败关闭，禁止原生全样本回退 |

NativeKeyInfo.size 必须非零且不超过原生样本大小，alignment 必须非零、为二次幂且不超过原生样本对齐。enable 仅校验声明，不用虚构样本试调用钩子；类型提供者必须保证声明和实现一致。capture 只读键，restore 只改目标的键，非键字段保留调用前值。快照是独立、无指针、无需构造/析构的键字节，大小与表示在 TypeSupport 生命周期内稳定。

原生捕获/查询若遇缺失或非法元数据返回 BAD_PARAMETER，capture/restore false 或标准异常返回 ERROR，分配失败返回 OUT_OF_RESOURCES；register_instance 只返回句柄，失败表现为 HANDLE_NIL。不再存在原生全样本键快照 memcpy 或查询 copy_data 兜底。普通 CDR 注册/查询仍使用原有序列化快照，这是未选择 native 时的回退行为。

Writer 捕获失败保留应用 Loan，不留下已注册实例。Reader 首次捕获失败在排序、过滤、淘汰和所有权状态变更前拒绝准入，回滚新建空实例，不占 max_instances；已有实例、历史和独立键快照不因失败删除。键快照不保留共享槽位引用，归还 Loan 后槽位可以复用，实例键仍可查询。调用细节见 [API 手册](DATASHARING-API.md#带键实例与本地快照)。

TypeSupport 虚函数扩展已改变二进制 ABI，库、应用及 TypeSupport 子类必须一起重编译。默认钩子保留旧代码的 CDR/RTPS 源码兼容，不保留旧带键 native 资格。

## 发现与共享协议

discovery v3 表示端点采用上述严格原生契约；v1/v2、未知版本和缺少能力扩展的对端不能选择 native，保留 RTPS。具体键快照大小和表示不新增到发现正文，协议版本用来隔离不同准入语义。共享布局仍为 v6，槽位 gate、64 位代次、64 Reader pin、重放 ready、水位与 futex 协议沿用；前阶段同步依据见 [历史生产协议报告](DATASHARING-PRODUCTION-20261030.md)。

实际 native 目标仍要求兼容的发现能力、文件/会话校验、真实 Reader 注册和重放 ready；普通 DDS matched 不代表共享通路已建立。旧协议、缺失键能力和建立映射前失败可使用 CDR/RTPS；已进入 native 后钩子失败不会偷偷复制完整样本，损坏 session 仍需新会话重建。

## 首轮失败与 ThreadPool 修复

首轮 `coverage-final/results.json` 明确为 `passed=false`：共享池 37 项和 payloadpool/Loan 213 项完成通过，但核心套件在 `DomainParticipant_delete_contentfilteredtopic2` 中挂起，未生成完整核心、最终覆盖率或示例结果。完整线程栈显示主线程在 ThreadPool::shutdown 中 join 工作线程，该线程仍等待条件变量；另外读取的 shutdown 原子标志已为 1。捕获证据后按 `core-stop.json` 的记录发送 SIGTERM，core 命令退出 143，不能记为测试通过。

根因是既有 ThreadPool 在 worker 条件变量谓词检查与进入休眠之间，未用相同 mutex 保护关闭标志更新/任务入队，通知可能先于休眠而丢失；原子标志不能代替条件变量的谓词互斥。源码现将 shutdown 标志更新和 submit 入队均放在 worker 等待所用的 mutex 下，worker 等待/出队后释放该锁再执行 task，允许另一个 worker 执行嵌套任务。关闭后提交不入队，返回已就绪的 broken_promise future，不在接收回调线程同步抛异常；shutdown 等待活动任务结束，不新增排空排队任务的保证。

本轮保留 `core-hang-threads.txt`、`core-hang-shutdown-state.txt`、`core-stop.json` 和 `coverage-final/before-library/`，用于区分修复前的库及现场。修复位于 `src/common/threads/threadpool.{h,cpp}`，新增 `test/unit_payloadpool/test_threadpool_progress.cpp` 中的三项正式回归：`NestedTaskCanCompleteOnAnotherWorker`、`ImmediateAndIdleShutdownNeverLoseTheWakeup`（512 次创建/关闭交错）和 `SubmissionAfterShutdownReturnsReadyBrokenPromise`，均属于 `ThreadPoolProgress`。三项修复回归已包含在 `coverage-complete` 的 216 项模块测试中通过；首轮局部通过仍只保留失败尝试的历史含义。

## 第二轮失败与 Ownership 测试修正

ThreadPool 修复后的第二轮 `coverage-verified/results.json` 仍为 `passed=false`：共享池 37 项与 payloadpool/Loan 216 项通过，核心 247 项运行结束但 `QoSTest.QoS_OwnershipQosPolicy` 有 1 项失败。原始 `core.log` 中第二次 SHARED take 返回 NO_DATA，随后 sample.id 仍为 1 而期望 2；没有最终覆盖率或示例通过结果，不能把该轮当作完整成功。

问题在既有测试同步：BaseDataReaderListener 的 dataflag 首次置位后不复位，第二次 wait_data_available 可立即返回，测试还忽略其返回值，不能用它计数已到达的样本。修正仅限 `test/unit_inside/test_qos.cpp` 的该用例，以真实 take 谓词循环等待 NO_DATA，正向最多等 2 秒；SHARED 的 id 1/2、EXCLUSIVE 的 id 3 期望保持，额外样本的负向断言完整观察 1 秒，未放宽生产 Ownership 规则。

修正后的该单项已重复 20 轮通过，证据为 `ownership-repeat.log`、`ownership-repeat.xml` 和 `ownership-case-verification.json`；其 scope 明确为定向测试；随后 `coverage-complete` 也已通过完整核心 247 项，包含修正后的该用例。重跑前保留第二轮结果并归档/重置 135 个 gcda，见 `coverage-verified/ownership-reset.json`；当前完整构建、测试、示例和覆盖率均以已通过的 `coverage-complete` 为准；ASan/UBSan 与 TSan 的本次独立插桩 focused 检查各 57 项通过。

## 本轮证据状态

以下结果均来自当前严格源码的新构建，319 项源码/构建输入在 coverage 运行结束时复核未变。旧版本 XML、百分比和 sanitizer 退出不能代替。

| 验证项 | 当前状态 |
|---|---|
| PayloadPool、共享池、DDS/Loan 与核心回归 | **37 + 216 + 247 = 500/500 通过**，失败/错误/禁用均为 0；`coverage-complete/{sharedpayloadpool_test,payloadpool_test,core}.xml` 包含 ON/AUTO 门控、失败关闭、旧协议隔离及 ThreadPool 三项回归 |
| 三个有限 Loan/keyed/mixed 示例 | 各 3 轮，共 **9 次示例执行通过**；`coverage-complete/examples/summary.json` 验证条数、CDR、Loan 保活、自然退出和共享目录清理 |
| 新增生产可执行行覆盖率 ≥70% | **93.920%（2765/2944）**，140 个翻译单元，变更生产翻译单元无漏采集；`coverage-complete/coverage/summary.json`，固定基线仍为 `489067108405b27afbd369245542f5e88d457f18` |
| ASan/UBSan | 完整插桩生产库、GTest 和 focused 驱动，**37 项共享池 + 13 项准入 + 3 项 ThreadPool + 4 项 DDS/key-only = 57/57 通过**；失败/错误/禁用/跳过均为 0，`asan-final/results.json` 的全部 9 个命令退出 0 |
| TSan | 独立完整插桩生产库、GTest 和 focused 驱动，**37 + 13 + 3 + 4 = 57/57 通过**；失败/错误/禁用/跳过均为 0，`tsan-final/results.json` 的全部 9 个命令退出 0，三个实际测试命令均使用 `setarch x86_64 -R` |
| 源码和库身份 | coverage、ASan、TSan 各用独立安装前缀，319 项源码/构建输入身份相同且当前未变；实际生产库、68 个安装头文件及 12 个程序的私有链接由最终审计核验 |
| 最终汇总审计 | `final-verification.json` 为 passed=true；三模式 XML 逐项完成、测试身份一致、生产库及 GTest 插桩、覆盖率/示例原始记录均核验通过，审计时无遗留共享段或活动测试程序 |

当前模块测试包含 `NativeKeyAdmission` 的 5 项行为回归及 `MalformedLayouts/NativeKeyMalformedAdmission` 的 8 项参数组合：缺失/四种非法键元数据的 ON 拒绝、AUTO CDR、单边缺失能力时 RTPS、双方合规时真实原生 Loan 均通过；`DataSharingDiscovery.PreviousProtocolsNeverEnableStrictNativeDelivery` 验证旧协议隔离。

本次重跑已保留旧证据并归档/重置 135 个 gcda，避免把失败尝试及定向重复计数混入新覆盖率。`coverage-complete/results.json` 保存命令、退出、XML、319 项源码输入哈希、生产库哈希和运行资源，全部命令退出 0。覆盖率仅统计相对固定基线新增的生产可执行行，合并库、测试和示例翻译单元中的模板实例，排除测试、依赖和文档；它不是分支覆盖率或交付完成度。LSan 此前因已确认的 ptrace 环境限制无法运行，无泄漏结论仍为空缺；历史无栈 WaitSet 崩溃仍未归因。

本次 `coverage-complete/examples/summary.json` 生成于 `2026-10-30T13:46:20.356413Z`。逐文件输入哈希在结果的 source_sha256 对象中；该对象按键排序并以紧凑 JSON 编码后的 SHA-256 为 `6f1669dbaa1608ba96c58ebc39d48adc417958b644667b0de68c1094535670b9`。

ASan/UBSan 本次 focused XML 记录时间为 `2026-10-30T21:55:15.875` 至 `21:55:19.560`（运行主机本地时间 UTC+8）。逐项 XML 均为 run/completed，没有失败或跳过；共享池 37 项、准入/ThreadPool 16 项及 DDS/key-only 4 项均实际完成。检查版生产库和测试私有链接对应 `build-asan/install/`，未使用 coverage 库。为继续 TSan 构建节省磁盘，仅删除可重建 ASan 对象缓存和与安装库哈希相同、inode 不同的构建副本；已保留安装库、测试二进制、静态 GTest archives、日志及全部结果，记录为 `asan-cache-cleanup.json`。

TSan 本次 focused XML 记录时间为 `2026-10-30T21:59:19.491` 至 `21:59:22.424`（UTC+8），同样实际完成 37 + 16 + 4 项；提取的 DDS/key-only 驱动与 ASan 哈希相同。两版共享池测试身份与 coverage 一致，准入/ThreadPool 16 项及 DDS/key-only 4 项均属于当前完整模块回归的子集。

[最终汇总审计](/home/ubuntu/project/travodds/artifacts/strict-native-20261030/final-verification.json) 生成于 `2026-10-30T13:59:31.206143Z`，其 [审计脚本](/home/ubuntu/project/travodds/artifacts/strict-native-20261030/final-audit.py) 无需重建或重跑测试：独立核对三模式相同的 319 项输入、68 个安装头文件/模式、500/57/57 个逐项完成测试，以及共 12 个程序链接各自私有库；同时检查生产库与静态 GTest archives 的 sanitizer 符号、覆盖率汇总/源码绑定及 9 次示例独立结果/原始 DS_RESULT 绑定。审计时 git diff --check 通过、共享段及活动测试程序均为空。该汇总仅证明记录的范围，LSan 未执行与历史 WaitSet 未归因限制保留。

| 当前产物（相对本报告证据根） | SHA-256 |
|---|---|
| `coverage-complete/results.json` | `556680bcd422e4afd932ed91f955327ff476278902f2ef37b16f02aab3408998` |
| `coverage-complete/coverage/summary.json` | `c4d95a102dfbcd4ca9eec397366d31e82af1e407e477f7ba0c86dd42ba96f6a9` |
| `coverage-complete/examples/summary.json` | `bd3749e523af4859b2f3cb04b3c5843df152cb81774d83b36e79cc5a24dd7560` |
| `build-coverage/install/lib/libtravodds.so.1.1.0` | `85261c45b3ac93c47b39faec2b1945d8e1b9f6721f4422c08e04ee357c77298a` |
| `asan-final/results.json` | `2b9be65997284a542992b847057cfd0db26d081be1082695f85f11c330713937` |
| `build-asan/install/lib/libtravodds.so.1.1.0` | `78fcca91006051df7d0505aabdab162920aac65572a83f85af6d9be467cbde64` |
| `asan-final/native-dds-progress.cpp` | `2e837e48a670867180c86023b5978b98e9860941c1277eb8a5a338e4ca53804d` |
| `asan-cache-cleanup.json` | `5501d6d53cd3390f467188855ea00a728231d1414e218bb9aa5c9adc4177c663` |
| `tsan-final/results.json` | `0c1706ef809ba4f984298f1cfdea366f7c2c35bf7ffa44d33849bb029936f3b9` |
| `build-tsan/install/lib/libtravodds.so.1.1.0` | `2bb35880144f62617610fc9e276f3ea9120f6e6c2b43b41b473854ae8ebb6783` |
| `tsan-final/native-dds-progress.cpp` | `2e837e48a670867180c86023b5978b98e9860941c1277eb8a5a338e4ca53804d` |
| `final-audit.py` | `241d37953c749c23a761b3d836c90a4aa0646f6e8620bb27bd45bac794dca569` |
| `final-verification.json` | `332eebdc40807039d394aec610fd71723137fa85172cfbef4b70535875c51d81` |

## 串行复现

使用当前严格源码、新的证据目录及私有构建目录；所有编译和检查串行执行：

```bash
python3 tools/ospp/production_lockfree_verify.py \
  --output /absolute/new-strict-evidence/coverage-complete \
  --build-dir /absolute/new-strict-evidence/build-coverage \
  --mode coverage --core --examples 3 \
  --baseline 489067108405b27afbd369245542f5e88d457f18
```

ASan/UBSan 和 TSan 使用各自的新 build/output，下面三个选择器与本次结果一致。focused 自动运行共享池 37 项、准入/ThreadPool 16 项，以及默认 1 项 DDS 控制锁暂停回归；三个选择器再加入 3 项 DDS/key-only，共 57 项：

```bash
python3 tools/ospp/production_lockfree_verify.py \
  --output /absolute/new-strict-evidence/asan-final \
  --build-dir /absolute/new-strict-evidence/build-asan \
  --mode asan --scope focused \
  --focus-test KeyedNativeCaptureFailurePreservesLoanAndRollsBackRegistration \
  --focus-test KeyedNativeReaderCaptureFailureDoesNotConsumeInstanceCapacityOrRetainSource \
  --focus-test NativeReaderIntegration.KeyedNativeKeySnapshotSurvivesSingleSlotReuseAndWriterDeletion

python3 tools/ospp/production_lockfree_verify.py \
  --output /absolute/new-strict-evidence/tsan-final \
  --build-dir /absolute/new-strict-evidence/build-tsan \
  --mode tsan --scope focused \
  --focus-test KeyedNativeCaptureFailurePreservesLoanAndRollsBackRegistration \
  --focus-test KeyedNativeReaderCaptureFailureDoesNotConsumeInstanceCapacityOrRetainSource \
  --focus-test NativeReaderIntegration.KeyedNativeKeySnapshotSurvivesSingleSlotReuseAndWriterDeletion
```

运行器在 TSan 测试命令中使用 `setarch x86_64 -R`，不修改系统 sysctl。build-dir 父目录中的 compiler.lock 仅约束同一父目录内的并发编译，本轮各构建放在同一 strict-native 根下串行安排。focused 结果只证明实际运行的用例，不代表全部 DDS 集成已在 sanitizer 下运行。

共享数据面不获取跨进程管理 mutex，不等于整个 DDS write 路径无锁：控制注册/维护、本地 Writer/history/发现锁及 futex 系统调用仍存在。普通裸指针 write、预分配输出、控制消息和非 native CDR 分支仍可复制。严格 native 保证限定于符合 TypeSupport 契约的 Writer Loan → Reader Loan 完整载荷路径；不包含任意有界容器、恶意类型实现或整个进程的零字节复制。
