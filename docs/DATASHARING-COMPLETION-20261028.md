# TravoDDS 本轮实现与交付报告（2026-10-28）

当前状态：实现、两版完整验收、重点重复、示例、覆盖率、性能与精确资源清理已完成，最终补丁基线可应用性与交付审计通过。没有提交、推送或创建PR。正式交付结果由 `artifacts/completion-20261028/verification.json` 审计。

基线为 `489067108405b27afbd369245542f5e88d457f18`，工作分支为 `work/ospp-datasharing-bootstrap`。实现沿用 Boost.Interprocess 布局v5，保留全部先前工作区成果。构建/测试/工具输入清单SHA256为 `953cfc4395a9273ef38aa0e01463f46a83de6dfc0f04b7a17be369e2f2b78361`；文档结果不参与构建身份，交付时另做哈希。所有构建使用 `--parallel 1`，普通、检查版、覆盖率、性能在本机串行运行。

## 官方八项交付

各项的实现入口、正式测试及证据对应关系见 [验收矩阵](DATASHARING-ACCEPTANCE.md)。

| 交付项 | 当前实现 | 最终验收 |
|---|---|---|
| 1. PayloadPool | 接口、Topic注册、四策略、归属与分片回滚 | 完成，见下表 |
| 2. 共享段 | Boost创建/打开/映射、固定槽位/代次、引用、死亡与孤儿检查 | 完成 |
| 3. 跨进程通知 | 原子通知代次/futex、可取消listener | 完成 |
| 4. Loan API | Writer loan/write/discard、Reader read/take/return及变体 | 完成 |
| 5. 旁路与回退 | 固定布局原生载荷、混合OFF Reader、可靠/尽力、持久性、恢复 | 完成 |
| 6. QoS | AUTO/ON/OFF、XML、预算、匹配、映射/注册自动重试 | 完成 |
| 7. 覆盖率与示例 | 覆盖率统计工具、三个独立exec有限示例 | 完成，见下表 |
| 8. 文档与交付 | API/QoS/设计/错误/调试、可复现脚本及补丁审计 | 完成，见下表 |

## 本轮修复与补齐

三个review问题已转为正式断言：旧库运行3项、3项均失败（`review-before.xml/log`）；修复后纳入普通/ASan完整集，不以旧探针退出0当通过。过期样本在创建实例或淘汰历史之前被拒绝；不同Writer相同序号按缓存身份移除；错误read/take先预览并完成输出，再提交样本状态/移除，失败可重新读取。KEEP_LAST容量耗尽时只退役可安全替换的历史引用，未读/应用Loan仍独立保活，不增加配置外槽位。

共享映射和Reader注册的暂时失败由弱引用worker以25ms至1s退避重试；完成实际注册后Writer才选择共享。取消/销毁不在匹配表、ingress或连接表锁内等待worker。CORRUPT/关闭/布局不兼容是终止条件，需要新会话。

有限ReaderDataLifecycle分别处理disposed与最后Writer离开；单调时间到期时在history锁内重检当前状态，新ALIVE取消旧到期。回收实例索引、键快照、history及max_instances配额；已交给应用的Loan独立保活，旧句柄查询失效。默认无限语义保持不变。

CDR/native统一保留源时间；无合法INFO_TS时明确使用接收时间。lifespan准入、后续过期和time_based_filter延迟提交统一检查。Timer运行状态以独立共享对象、互斥/CV和修订号同步，callback在Timer锁外；端点QoS、deadline活动实例和销毁顺序同步。deadline/status与TBF应用listener在内部调度锁外执行，允许查询状态和set_qos重入。

## 失败证据与处理

- `candidate1/`、`candidate2/`、`candidate3/`保存中间版本结果，不能作为最终通过。候选2在重点第18轮暴露迟加入混合测试把DDS匹配等同共享注册的问题；保留原失败。修正测试阶段屏障后，两可靠性专项100轮共200次父场景通过，仍保留Loan/顺序/载荷/CDR成功计数的断言。
- `lifecycle-first.log`与`retry-crash-signal.log`保存本轮定位的builtin Reader空history崩溃及栈，已加空history防护。`retry-crash-stack.log`中的gdb ptrace拒绝不算成功调试。
- 历史无栈WaitSet SIGSEGV的根因仍未知；上述builtin Reader修复不作为该旧异常的根因证明。条件多WaitSet分离及销毁路径已补回归并进入重复集。

日志异步线程在并发首次启动时可覆盖可join线程而触发terminate。独立探针栈落在LogInitial；正式LoggerSynchronization旧库失败，修复后通过。运行状态统一使用queueMutex，启停及线程所有权使用独立生命周期锁，join不持queueMutex，掩码使用原子。新会话测试新增Writer侧共享选择屏障，保留禁止CDR和单次投递断言。

TSan首次运行在SPDP首次发送与周期重发中发现真实缓冲竞争：发送线程读取CacheChange的报文头，另一个线程在同一headroom封装DATA。已将非native DeliveryData也纳入readerLocatorsMutex_，使首次发送/周期重发/发现重放的封装与发送串行；原始TSan报告保存在`tsan-attempt1/`。修复后完整生产库插桩，Timer/logger3项×30轮、QoS/deadline/TBF/有限回收7项×10轮通过，实际退出0，无race诊断。该结果只覆盖列出的路径，不代表全项目无数据竞争。

## 当前验证

当前冻结版本的所有必需执行阶段均已成功；每项命令有实际退出0、源码前后核对和原始日志。两版测试项相同，不能相加称为936个独立用例。

| 检查 | 最终真实结果 | 证据 |
|---|---|---|
| 普通完整 | 247核心+195集成/池+26共享池=468/468 | `normal-full/*.xml` |
| ASan/UBSan完整 | 同样468/468，无检查器报告，detect_leaks=0 | `sanitize-full/*.xml` |
| 重点稳定性 | 两版各55项×30轮=1650次父场景，均退出0 | `normal-repeat/`、`sanitize-repeat/` |
| 三个有限示例 | 两版各30轮×3=90次，自然退出0；载荷/Loan保活/CDR/清理均检查 | `normal-examples/`、`sanitize-examples/` |
| 既有RTPS回退 | default/SHM/UDP smoke通过；TCP/WaitSet5项×6轮通过 | `smoke/`、`tcp-waitset/` |
| 独立TSan | 完整生产库插桩，Timer/logger3×30、QoS/deadline/TBF/有限回收7×10通过 | `tsan/logs/`、`checker-environment.json` |
| 新增生产行覆盖率 | 2432/2584=94.118%，138个翻译单元，变更.cpp无漏收 | `coverage/summary.json`、gcov原报告 |
| 最终Boost性能 | 64/4096/65536B×三模式×三轮=27次，全自然退出0 | `benchmark/summary.json`、逐次日志 |
| 来源及运行身份 | 两版安装改动头一致；12测试/示例及benchmark链接私有前缀；检查版确有检查器 | `verification.json`、`runtime-audit.json`、`benchmark-runtime-audit.json` |

覆盖率固定比较基线4890671，统计GCC新增可执行生产行（src/include），合并模板和所有翻译单元，未执行gcno仍入分母；排除测试/依赖/文档，未收集变更.cpp会失败。152行未覆盖的逐文件行号和源哈希在summary中；这是行覆盖，不是分支覆盖。没有使用旧94.68%。

性能三轮中位吞吐64B Loan约8879/s、RTPS29379/s；4KiB Loan8415/s、RTPS21607/s；64KiB Loan4119/s、RTPS71/s。小消息共享路径的固定同步/通知/调度成本占主导，不能声称全面提速；完整表、延迟、RSS和局限见 [性能手册](DATASHARING-PERFORMANCE.md)。所有共享测量完整载荷CDR为0，RTPS计数精确匹配预热与计量量。

候选3两版完整467项通过但ASan重点重复失败，保留于candidate3/，不作为最终验收。

## 资源与失败闭环

最终还查出一条10月27日历史测试孤儿（PID454596，PPID1，已在交接起点快照中，旧删除程序与历史失败日志匹配），保留证据后精确终止。没有本任务的编译/测试/示例/benchmark进程，无`travodds_ds_*.shm`残留。两个本轮诊断崩溃的确切旧文件分别通过生产removeOrphan和已核对inode/时间/无任何进程映射及fd的精确unlink处理，证据在`cleanup-result.json`及`root-cleanup-inspection.log`。普通扫描存在proc权限拒绝，故额外以sudo只读核查全进程后才删除空文件；没有通配清理。

其他应用和旧SHM transport的全局UUID/传输文件保留，列表见`shared-resource-inventory.json`。无限旧smoke由工具停止，可能留下旧transport命名资源，不将其混称为DataSharing泄漏，也不删除已有其他任务资源。ASan重复主进程RSS峰值约807MiB，已记录resource-observations，退出后可用内存恢复约1GiB；LSan未运行，不能据此宣称无泄漏。

失败材料均保留：review正式旧库3失败、logger正式旧库1失败、候选2迟加入屏障失败、候选3重复失败、TSan首次真实SPDP竞争、builtin空history崩溃及栈。后续最终完整和重复均在修复后的同冻结源码执行；历史无栈WaitSet仍未归因。证据根目录为 `artifacts/completion-20261028/`，`commands.jsonl`记录实际退出码、耗时、资源和冻结清单身份。

## 适用范围与限制

- Ubuntu、SECURITY=OFF。未验收Windows、独立openEuler内核或DDS Security。
- 零拷贝限于显式声明兼容固定布局的应用Writer Loan→Reader Loan载荷；普通write、预分配Reader输出和键快照仍会复制。通知代次可无锁发布，元数据使用robust mutex，不能称端到端全流程无锁。
- ON严格要求本地准备成功，对端不可用仍可回退RTPS；兼容暂时失败能自动重试。CORRUPT段必须新建会话，BEST_EFFORT不保证重传。
- Boost1.88自定义目录使用私有file_wrapper接口，升级需复核布局与行为。
- LSan探针被宿主ptrace限制拒绝；本轮ASan/UBSan显式关闭detect_leaks，不能宣称无泄漏。独立TSan的实际结果另见checker证据。
- 性能是2vCPU/约1.9GiB、带swap的共享VM短测，包含填充/验证/控制/调度成本，不能解释为最大可持续吞吐或SLA。

## 复查和复现

先看 [验收矩阵及串行命令](DATASHARING-ACCEPTANCE.md)、[性能方法](DATASHARING-PERFORMANCE.md)、[API](DATASHARING-API.md)、[调试](DATASHARING-DEBUG.md)。最终补丁为证据目录的 `final.patch`，包含新增源码/测试/工具/文档，排除构建和运行日志。`delivery-source.json`给出全部交付文件哈希；`deliver.py`同时校验冻结输入、安装头、库链接与补丁对基线的可应用性。生成patch不会创建Git提交。


## 最终包

`verification.json`记录冻结身份、每条实际命令/退出、两版计数/重复/示例、覆盖率、性能、检查器与库/头文件。`final.patch`包含118个变更文件（已有修改与新增源码、测试、工具、文档），排除日志/构建/凭据；`delivery-source.json`是逐文件SHA256。补丁已用基线4890671的原始文件执行`git apply --check`通过，`git diff --check`通过。未创建任何新commit，分支HEAD仍为基线。

精确清理、12程序链接补充、性能链接和最终进程/资源清单作为verification补充审计一同保存。原失败及候选记录保留于同一证据目录。复现执行工具都在`tools/ospp/`，具体串行步骤见验收矩阵。交付只覆盖报告所列Ubuntu范围和验证路径，不是全项目无竞态、无泄漏或生产可用保证。
