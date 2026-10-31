# DataSharing 生产后端接入与验证

> 历史快照：本页记录 key-only 补齐前的共享布局 v6 / discovery v2 同步协议和 234/38 项验证。其后的可选 key-only 阶段见 [同日历史报告](DATASHARING-COMPLETION-20261030.md)。当前 discovery v3 的强制 key-only 准入和新证据见 [严格准入报告](DATASHARING-STRICT-20261030.md)，旧计数和哈希不能作为当前通过证明。

实际工作树：`/home/ubuntu/project/travodds/.worktrees/datasharing-progress`，分支 `work/datasharing-progress`，基点 `3da8e077b9902142cdb4549eafa5b43114bdf936`。本报告记录在该基点上的未提交改动；上一轮实验见 [10 月 29 日记录](DATASHARING-PROGRESS-20261029.md)。本轮修改的是正式 `SharedPayloadPool` 和真实 DDS 接入路径，实验目录保留作对照。

## 已完成的接入

| 上轮缺口 | 本轮实现 |
|---|---|
| 实验只有 15 Reader、32 位槽位代次 | 生产 Layout v6 保留运行时槽数、载荷大小、最多 64 Reader 和完整 64 位槽位/注册代次。槽位达到 `UINT64_MAX` 后退休，不回绕。 |
| 实验只有注册请求值，没有 DDS 控制路径 | 复用已有的真实跨进程 `registerReader`、发现和 GUID 目标选择。注册管理保留 robust mutex；发布、借用、归还及注册快照不获取这把锁。 |
| 实验拒绝历史重放 | 生产后端保留 TRANSIENT_LOCAL 重放：登记先 OPEN，重放准备完成后才向 DDS 宣告可绕过 RTPS。过渡期共享发布可建立 pending，历史扫描与发布水位配合。 |
| 实验原始令牌没有 DDS Loan 所有权 | 复用正式 `SampleLoan::State → pool`、Writer history 的 alias shared owner，以及 Reader 的 `CacheChange → SharedReadLoan → registration → pool` 所有权链。Reader/Writer 对象删除不撤销已有借出映射。 |
| 死 Reader 管理与发布混在一起 | Writer 每 100 ms 用弱 pool 引用做维护，Reader 空等待也做维护；`statistics()` try-lock 忙返回 TIMEOUT。写入目标快照和提交不做 `/proc` 探测。 |
| 旧布局兼容性不明确 | 共享布局版本为 6，发现能力版本为 2。旧 v1 和未知发现版本不能启用 native 路径，继续使用 RTPS；映射仍严格校验布局、尺寸、session 与命名空间。 |

无锁目标在这里具体指：**已有 Reader 即使持有控制元数据 mutex 并被暂停，也不能阻止 Writer 向其他可用槽位提交、Reader 取样与归还。**完整 DDS `write()` 仍有进程本地 Writer/history/发现表锁，futex 唤醒仍是系统调用；本轮没有把它们宣称为形式化的全路径 lock-free 或 wait-free。

## 协议与安全依据

每个槽位的原子 gate 保存 phase 和短操作计数；64 位 generation 独立保存，但必须在取得 gate lease 后验证。reserve 先预检 persistent pins，再 CAS 将零操作计数的槽位冻结，复查全部 pins 后才改变 generation 和载荷。这避免“先检查旧代次、恢复后误清新代次”的窗口，也避免冻结仍有未读样本的槽位而导致扫描跳过更早序号。

Reader 从 pending 转移到 loaned 时先建立 loaned pin，再清 pending，中间不会出现无引用窗口。slot lease 在这组操作完成前阻止复用；lease 的 release 与下次 reserve 的 acquire 连接上一批读取和下一次覆写。Reader 实际使用载荷时由持久 loaned pin 保护，不长期占用短操作计数。

同一个进程本地 `SharedPoolReader` 的 take/discard 由 atomic try-consumer 串行化，竞争时立即返回 TIMEOUT/false，已有 Loan 的归还独立进行。该门控不在共享布局中，也不会阻止其他 Reader 或 Writer；不同 Reader 可并发消费。多个直接 publish 调用竞争同一发布 lane 时立即返回 EXHAUSTED；DDS 本身已有串行 Writer lane。

关闭 Reader 先拒绝新操作。旧操作未退出时不清 pending、不复用身份；旧 Loan 始终保留 pin。最终回收者自己也持有一份注册 lease，扫描后通过 CAS 同时将自己的最后一份 lease 消耗为 RETIRED。否则，旧关闭者可能在暂停期间跨过“回收、重注册、新借用、再次关闭”，只凭相同 CLOSING phase 误回收新身份。

发布先完成槽位，再 release-store 全段序号水位；Reader 按一次水位快照选择最早 pending 序号。通知先采样 changed，再检查谓词，最后 futex 比较等待，保留原截止时间并周期重查。观察到 Writer 关闭必须重新扫描最终水位，确保排空最后已提交样本后才返回 CLOSED。

重放登记在 ready 之前不出现在 `registeredReaders()` 的 DDS 绕过快照中。这样，重放扫描尚未完成时，旧历史不能仅凭登记 OPEN 就停止发送 RTPS。共享 publish 的 OPEN 目标与扫描配合捕获过渡样本；可能出现的 RTPS/native 重复仍由原有序号逻辑处理。

## 故障契约

| 故障 | 行为 |
|---|---|
| Reader 慢、持有旧 Loan 或暂停 | 固定其涉及的槽位，其余空槽可继续提交；容量耗尽返回 EXHAUSTED/TIMEOUT，不覆写旧载荷。 |
| 控制 mutex 持有者仍存活但暂停 | 已有数据面继续；统计维护忙返回 TIMEOUT；新的注册管理仍可能等待控制锁。 |
| Reader 进程死亡，登记中没有短操作计数 | 控制维护核实 PID/start identity 后清除该身份的长期 Loan/未读引用。未知 `/proc` 结果不作为死亡证据。 |
| 已死亡登记仍有无法归属的短操作计数 | 不猜测、强减共享计数；将整个 session 标为 CORRUPT，拒绝新分配并使用新 session 重建。计数也可能来自并发的活发布者，因此某些并发死亡时序会保守走重建。已经借出的映射和字节继续有效。 |
| robust 管理锁持有者死亡 | 管理恢复将会话标为 CORRUPT，不修补不确定的半事务。 |
| Writer 死亡或关闭 | 保留已有 Loan 和已提交 pending 的可读性；关闭后不分配新样本。重启使用新的 session，孤儿清理严格核对旧 inode/session。 |

这里没有新增“任意运行时错误都透明回退 RTPS”的保证。旧协议协商、不兼容类型和建立映射前的失败仍按现有 AUTO/ON 契约处理；已经损坏的 native session 仍采用明确的端点重建边界。

## 验证

证据目录：`/home/ubuntu/project/travodds/artifacts/production-lockfree-20261030/`。每轮记录完整编译命令、退出码、XML、源码和库 SHA-256；所有 CMake 构建均 `--parallel 1`，编译进程 768 MiB 地址空间上限，低优先级。

| 验证 | 已确认结果 |
|---|---|
| `normal-final/results.json` | 新生产库重新构建；共享池 37/37、DDS/Loan 集成 197/197，共 234 项，全部通过。 |
| `normal-repeat.json` | 全部 37 项共享池回归重复 20 轮，退出 0；重复次数不当作新增测试数量。 |
| `asan-final/results.json` | 生产库、GTest、全部 37 项共享池测试及正式 DDS 控制锁占用用例完整插桩，38/38 通过。未完整编译/运行 ASan 下其他 196 项 DDS 集成测试；泄漏扫描因已确认的 ptrace 环境限制关闭。 |
| `tsan-final/results.json` | 生产库、GTest、全部 37 项共享池测试及正式 DDS 控制锁占用用例完整插桩，38/38 通过，未报告数据竞争；未运行 TSan 下其他 196 项 DDS 集成测试。 |
| `retirement-model.json` | 有限 SC 模型在 12 状态内找到旧 phase-only 回收的 ABA 反例；持回收 lease 的模型遍历 7 状态未找到该反例。不是完整 C++ 弱内存证明。 |

新增生产回归验证：控制锁持有者 SIGSTOP 时 P/Z/T/W/R/N 操作继续；64 位 generation 跨过 `UINT32_MAX`；全部 64 Reader 位的目标选择和最高位重放；跨线程归还后身份复用；死亡短 lease 的保守会话损坏；同一 Reader 两个线程准确消费 2,000 个样本且无残余 pin；128 轮旧关闭者与新身份/Loan 并发交错；重放 ready 之前不允许 native bypass；槽位冻结复查窗口不得跳过更早序号，也不得越过零超时截止。

真实 DDS 新用例在控制 mutex 始终被占用时，使用 3 个槽位连续完成 8 次 `loan_sample → write → take → return_loan`，并一直保留首个 Loan 验证字节未变；序列化、反序列化和 copy_data 计数均为零。它验证的是固定原生类型的借用路径，不将普通 write 的 memcpy 或 keyed 首次实例快照复制排除在外。

首轮普通回归实际发现一个失败：Writer 完成最后发布并关闭时，Reader 使用较早水位扫描后可能过早返回 CLOSED，留下末尾 pending。现已通过“观察关闭后的最终水位重扫”修正，并通过整轮重新构建验证。首轮失败日志保留在 `normal-attempt1/`。独立回收模型初稿漏掉了“先前确实观察到零 Loan”的前提，修正模型后保留了实际旧协议反例；这不计作生产代码失败。

ASan 首次测试构建失败于 GoogleTest 自动枚举继承了编译器地址空间限制，现使用 PRE_TEST 枚举，实际测试运行不带该限制。巨大的 `test_loan.cpp` 即使 O0 插桩仍超过本机 768 MiB 编译上限，因此检查版改用 `--scope focused`：从同一份已哈希源码中精确提取既有 fixture 和 DDS 关键用例，生成、保存并完整插桩小型测试驱动；完整生产库仍全部插桩，不使用旧库。失败日志保留在 `asan-attempt1/` 至 `asan-attempt3/`。不能将这 38 项结果表述为 ASan 下 234 项集成全部通过。

磁盘紧张时仅删除本轮 ASan 生产库的 96 个生成对象缓存，约 153.5 MiB，保留源码、库、可执行测试、哈希及完整日志。清理前后库 SHA-256 一致，记录为 `asan-object-cache-cleanup.json`。

原有协作代理被 `401 USER_INACTIVE` 中断。另按 [claude 技能](/home/ubuntu/.codex/skills/claude/SKILL.md) 请求只读独立审查，但账号返回 `You've hit your weekly limit`，未产生专家审查结论。问题和诊断保留在 `/home/ubuntu/Library/Application Support/AI Expert Reviews/claude-20261030-002947.z9cmmpxt/`；本轮不宣称获得了最终独立专家批准，重要交错由主线程复查并以回归验证。

## 重现

```sh
python3 tools/ospp/production_lockfree_verify.py \
  --output /tmp/travodds-production-normal-evidence \
  --build-dir /tmp/travodds-production-normal-build \
  --mode normal
```

检查版分别选择 `--mode asan --scope focused` 或 `--mode tsan --scope focused`，使用各自不同的新 build-dir 和证据 output。脚本拒绝覆盖旧证据，build-dir 父目录中的 compiler.lock 防止同一构建根下同时编译；不同父目录之间仍需调用者串行安排。TSan 使用 `setarch x86_64 -R`，不更改内核设置。普通版构建生产库和该模块两个完整测试目标，检查版构建完整插桩生产库、共享池回归及提取的原生 DDS 关键用例，不等于执行仓库全部测试。

目前没有提交、推送或创建新的 PR；已有日志器 PR 不受本轮改造影响。Linux 上的源码、可复现实验和验证产物保存在上述工作树。跨平台部署、长期压力和与 Fast DDS 的同条件性能比较不由本轮功能检查替代。
