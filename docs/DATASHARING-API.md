# DataSharing / Loan API（当前实现）

> 当前后端为 Boost.Interprocess、布局v5；2026-10-28修复与验收见 [本轮报告](DATASHARING-COMPLETION-20261028.md) 与 [需求矩阵](DATASHARING-ACCEPTANCE.md)。本页明确标注2026-10-27的数字仅保留历史含义。

当前实现已接通 Linux 上固定布局类型的自动共享发布/接收。共享能力与实际映射注册兼容的 Reader 从共享段收样；混合匹配中的其他 Reader 继续使用定向 RTPS。纯共享路径的双进程测试将两端序列化/反序列化器设为失败，Writer Loan 与普通写入均通过。带键类型也可使用原生共享路径；存在 SQL Reader 过滤器时继续走 CDR。键哈希仍通过 TypeSupport 的键操作计算；本轮Ubuntu验收范围和限制见当前报告。配置见 [QoS 手册](DATASHARING-QOS.md)。

## 类型声明

旧 TypeSupport 不必修改，默认不声明原生布局能力。新固定布局类型可从 `NativeTypeSupport<T>` 继承，像已有 TypeSupport 一样实现序列化、复制、类型名和键操作，并向基类构造函数提供版本化布局 ID。

T 必须是标准布局、可平凡复制和析构、默认构造 noexcept 的类型，且所有字段都不依赖进程内地址。指针类型也可能满足 C++ trivial 属性，因此应用必须另外保证字段不含指针。布局 ID 应包含协议版本与 ABI 约束，改变成员顺序、大小、对齐或含义时必须更换。`std::string`、`std::vector` 等类型不能放入共享样本。

`get_native_type_info()` 返回 bounded/plain、size、alignment 和 layout_id；bounded 与 plain 分别声明，只有两者成立且布局信息有效时才允许原生 Loan。

## Writer

```cpp
SampleLoan loan;
auto rc = writer->loan_sample(loan);
if (rc == RETCODE_OK) {
    auto* value = static_cast<MyFixedSample*>(loan.data());
    value->index = 1;
    rc = writer->write(loan, HANDLE_NIL);
    if (rc != RETCODE_OK) {
        writer->discard_loan(loan); // 也可以保留 loan 重试
    }
}
```

- `loan_sample(SampleLoan&)`：仅支持显式声明的原生布局，返回已初始化的对象。Linux Writer 在 enable 时准备共享池，默认 64 个槽位，目录 `/dev/shm`；通过 `qos.data_sharing` 配置。OFF 或 AUTO 在 enable 时未能准备共享段的端点返回 UNSUPPORTED。
- `write(SampleLoan&, handle)`：句柄必须属于此 Writer。成功后句柄为空，原地址不得继续访问；失败时句柄及数据保持可用。有已注册的兼容 Reader 时直接发布借出的槽位；其他匹配 Reader 获取一次生成的 CDR。TRANSIENT_LOCAL Writer 尚无 Reader 时也保留原生历史；VOLATILE 且没有共享目标时沿用普通 RTPS。
- `discard_loan(SampleLoan&)`：归还未提交的句柄并清空。错误 Writer、重复操作和空句柄返回 PRECONDITION_NOT_MET。不修改其他 Writer 的借用。
- `SampleLoan` 只能移动，移动后源句柄为空。析构或 `reset()` 自动取消。Writer 删除后句柄仍保活映射，应用可取消或析构，不能再调用已删除的 Writer。
- `write(loan.data(), handle)` 是普通裸指针写入，不消费句柄；要使用借用的所有权语义，应使用句柄重载。

| 情况 | 结果 |
|---|---|
| Writer 未 enable | NOT_ENABLED |
| 类型未声明兼容布局或非 Linux 实现 | UNSUPPORTED |
| 输出句柄已有借用 | PRECONDITION_NOT_MET |
| 池耗尽或本地分配失败 | OUT_OF_RESOURCES |
| enable 时共享池布局/预算不合法（ON） | INCONSISTENT_POLICY |
| 原生构造失败或系统调用失败 | ERROR |
| 普通 RTPS 写入失败 | 沿用原写入错误码，保留句柄 |

发布前的序列化器若抛异常，沿用已有 API 的异常行为，句柄不会被消耗。共享发布是不可撤销的接受点；发布后 RTPS 发送或 QoS 定时器失败会记录日志，仍返回 OK 并消费句柄，禁止应用重新修改已交给 Reader 的样本。可靠模式由现有 history/heartbeat/ACK 管理重传；这一规则不承诺所有回退 Reader 当场收到样本。共享池准备阶段的 `invalid_argument` 转换为 INCONSISTENT_POLICY，分配失败转换为 OUT_OF_RESOURCES，其余标准异常转换为 ERROR；AUTO 在准备失败时保留普通 RTPS。

## Reader 与归还

```cpp
SequenceData<MyFixedSample> values; // maximum()==0，请求可用的原生 Loan
SampleInfoSeq infos;
auto rc = reader->take(values, infos, LENGTH_UNLIMITED,
    ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);
if (rc == RETCODE_OK) {
    for (uint32_t i = 0; i < values.length(); ++i) {
        if (infos[i].valid_data) {
            // 读取 values[i]；借出的样本必须视为只读
        }
    }
    reader->return_loan(values, infos);
}
```

当批次全部为布局兼容的有效原生缓存时，空输出序列借用其真实映射地址；数据与 SampleInfo 绑定同一个批次 pin。普通 RTPS 数据、混合批次和无有效数据的控制样本返回自有对象。`SequenceData<T>(capacity)` 预分配输出会请求复制；单样本 read/take 也复制，原生样本复制不经过反序列化。

Reader 在发现兼容 Writer 后，以本地目录配置及对端 GUID/会话推导文件名，验证共享段并注册自身 GUID。独立监听线程通过原子代次与 Linux futex 等待共享通知，生成持有 SharedReadLoan 的原生缓存，再交给现有可靠性代理与 history。可靠和尽力模式、同参与者内的 ON/OFF Reader 混用均有独立 exec 进程测试。

- `has_loan()` 判断是否有活跃借用；`has_ownership()` 判断序列是否拥有样本存储。空序列和借用序列均不拥有样本，借用序列 maximum 为 0、length 为实际样本数。
- `return_loan(values, infos)` 必须使用相同 Reader 与同批配对序列。错误 Reader、错配批次或数据/元信息角色错误返回 PRECONDITION_NOT_MET，已有借用保持有效。
- 成功归还后双方 length/maximum 为 0，不再有借用。双方均无借用时返回 OK，保留自有数据；归还后的空配对重复调用同样无副作用。
- 未归还时再次把相同输出用于 read/take，返回 PRECONDITION_NOT_MET。无数据返回 NO_DATA；非法样本数、空/其他 Reader 的 condition、把同一序列同时作为数据和元信息返回 BAD_PARAMETER。
- 借用时禁止 push_back 和 length 修改。复制会生成独立自有样本，移动转移借用。序列析构自动释放本方 pin；另一方仍存活时继续保活样本。
- read 保留 Reader history，take 仅移除 history 引用。最后一个 history/应用引用消失后才归还共享槽位。Reader 删除不会使已经借出的样本失效；删除后通过序列析构/unloan 释放，不调用已删除 Reader。

所有这些序列操作都要求应用避免并发修改同一个序列对象。Writer Loan 并发借出/取消已有测试；原生跨进程存储的崩溃/代次协议见设计文档。

此次扩展改变了 TypeSupport/DataWriter 的虚函数接口及序列对象布局，使用方需要整体重新编译，不能仅替换动态库后继续使用旧二进制。

## 本机验证与覆盖率

在仓库根目录执行，所有编译保持单线程：

```bash
BUILD_JOBS=1 bash tools/ospp/bootstrap.sh "$PWD/artifacts/local-loan"

# 当前机器 ptrace 限制不支持 LeakSanitizer，因此明确关闭泄漏扫描。
CORE_TESTS=1 COVERAGE=1 ASAN_DETECT_LEAKS=0 bash tools/ospp/sanitize.sh \
  "$PWD/artifacts/local-loan-sanitize"

python3 tools/ospp/coverage.py --source . \
  --baseline 489067108405b27afbd369245542f5e88d457f18 \
  --build-root artifacts/local-loan-sanitize \
  --output artifacts/local-loan-sanitize/coverage --minimum 70
```

`CORE_TESTS=1` 额外构建并运行 247 项核心测试；不设置时只运行池/Loan 集成与共享池测试。所有构建均为单线程。

覆盖率统计 GCC 标记的可执行生产代码行，取相对指定固定基线的新增行，合并库与测试翻译单元中的模板实例；不把测试、依赖和文档计入分母。QoS 阶段的历史快照为 1166/1225（95.18%），本轮测量以当前报告为准；这不是分支覆盖率，也不证明未实现的功能已完成。完整运行记录见 `OSPP-261760062.md`。


### 自动收发阶段验证快照（2026-10-27）

无键固定布局类型已在可靠/尽力 × 纯共享/同参与者混合 Reader 的独立 exec 场景验证。普通版 247+104+22 项通过，独立 ASan/UBSan 104+22 项通过（无泄漏扫描），新增生产代码行覆盖率 1504/1617（93.012%）。最终证据与剩余边界见 `DATASHARING-PLAN.md`，旧阶段覆盖率数字仅保留其历史含义。


### 普通池公开配置阶段验证快照（2026-10-27）

`DataWriterQos::payload_pool` 与 XML 的 `<payload_pool>` 可选四种策略、容量和对齐，详细块计数与预留空间说明见 [普通池配置手册](PAYLOADPOOL-QOS.md)。当前普通测试 247+113+22=382 项，独立 ASan/UBSan 113+22=135 项通过；两版各 30 轮独立进程重复验证通过，新增生产代码行覆盖率 1559/1682（92.687%），无泄漏扫描结论。完整验收边界继续以实施清单为准。


### 带键实例与本地快照

带键原生类型与无键类型采用相同的布局约束。TypeSupport 的 `MakeKey` 必须只根据键字段稳定生成实例句柄，成功返回 0；键字段可以使用键序列化与 MD5，而纯共享的 `serialize_data` / `deserialize_data` 调用次数为 0。dispose/unregister 通过 RTPS inline key/status 通知，不能据此声称不发送网络控制消息。

- Writer 注册实例时保存首个样本的独立本地快照，不占共享槽位。相同键再次注册返回原句柄，`get_key_value` 可重复查询；返回对象的非键字段来自首次快照，不代表最新样本。
- Native 快照按声明的对齐分配，构造后复制固定布局字节；普通 CDR 注册保留序列化快照并通过原池回收。注册序列化失败不会留下已注册实例。
- Reader 第一次接受某实例的 ALIVE 样本时保存独立快照。take/return_loan 后仍可查询键，且快照不会继续钉住共享槽位。CDR Reader 保存独立缓冲链，每次查询使用独立的全部分片游标。
- `lookup_instance` 对未知键、空数据或键计算失败返回 HANDLE_NIL。注册的空数据、负时间戳和资源耗尽也返回 HANDLE_NIL；隐式注册的 write 在实例数耗尽时返回 OUT_OF_RESOURCES。
- write 使用未注册的显式句柄返回 BAD_PARAMETER；样本键与已注册句柄不一致返回 PRECONDITION_NOT_MET。dispose/unregister 可以用空数据加有效句柄，空数据与 HANDLE_NIL 的组合返回 BAD_PARAMETER。
- 注销成功后 Writer 的句柄不再可查，键快照释放；相同键可重新注册。Reader 保留其已知实例身份，并通过 dispose、无 Writer、重新 ALIVE 的通知跟踪实例状态与代数。

每个已知实例会额外保留一份本地快照，其成本不包含在 `data_sharing.max_samples` 内。原生快照约为 `native size × 已知实例数`；普通快照按序列化长度计。Writer 可通过 `resource_limits.max_instances` 限制注册数；Reader按有限生命周期回收已知实例；默认无限时仍保留预算占用，具体语义见QoS手册。

本阶段覆盖带键类型的基本注册/查询、ALIVE/dispose/unregister/重新注册、可靠/尽力和混合 Reader 场景。deadline/lifespan与注销及read/take_instance组合已有本轮回归，有限Reader生命周期回收与多Writer语义见当前QoS手册，验收以本轮矩阵为准。


### 带键阶段最终验证（2026-10-27）

普通与独立 ASan/UBSan 版分别通过 247+122+22=391 项测试；两版各 30 轮 × 9 个独立 exec 场景通过。新增生产代码行覆盖率 1727/1831（94.32%），不含泄漏扫描结论。完整核心检查可用上面的 `CORE_TESTS=1` 命令复现。

此前一次普通完整核心测试在 WaitSet 用例段错误，未取得栈；后续诊断、重复与最终两版完整回归通过，但根因尚未确定。本轮证据见当前报告；该历史异常仍保留“根因未知”，后续通过不是归因证明。


### Deadline 与样本生命周期

成功写入 ALIVE 样本后，Writer 依据当前已注册实例安排 deadline；Reader 只在样本通过接收准入和 lifespan 检查后启动或刷新该实例的 deadline。dispose 暂停该实例的监视，后续成功写入/接收 ALIVE 再恢复。Writer unregister 成功后删除注册；迟到的定时回调不得重建旧句柄，也不得占用 `max_instances`。这里的调度时间使用单调时钟，与样本的源时间戳分别处理。

尚未到当前到期时间的旧回调不增加 missed 计数；移除最早实例后，其他活动实例仍被监视。deadline listener 在调度锁释放后调用，可在回调中 unregister。空端点不因一次计时唤醒而凭空出现 NIL 实例；无键样本成功写入/接收后的实际超时仍以 HANDLE_NIL 报告。

Writer history 中的样本因 lifespan 过期或注销被移除，不会撤销应用已持有的 Reader Loan。应用继续持有的共享引用会阻止槽位复用，直到 return_loan 或最后一个引用释放。这项保证已用真实 Writer 过期处理及共享 Reader 借用验证，不表示完整 lifespan 时钟/源时间戳矩阵已验收。

本阶段完整普通/检查版结果以 `DATASHARING-PLAN.md` 的最新记录为准。并发set_qos、Timer内部同步、从未写入实例不启动deadline、有限Reader回收的当前实现与回归见本轮验收矩阵。


### 多 Writer 的实例监视

SHARED 所有权下，一个实例可同时由多个 Writer 保持存活。一个 Writer unregister 后，如果 history 中该实例仍为 ALIVE，Reader 保留其原 deadline；控制消息不会刷新期限。最后一个 Writer 注销或从 history 的活动 Writer 集合移除后暂停监视。后续被接受的 ALIVE 样本重新启动。dispose 使实例进入 disposed 状态并暂停监视。

本轮对可靠/尽力接收逻辑的验证结果见实施清单，独立进程故障/发现租约与完整并发矩阵仍未完成。


### 已排队的 requested deadline 回调

接收生命周期通知或最后一个 Writer 消失，可以与已开始的超时回调交错。超时推进在 history 锁内重新确认实例仍为 ALIVE；否则停止该次监视并跳过 missed 通知，再扫描其他活动实例。这里保留已知实例身份和键快照，不删除实例。该确定性交错已有回归；并发set_qos与Timer内部同步现已实现，并有有界压力与回调重入用例，完整结果见本轮验收矩阵。


### 晚加入、容量与故障恢复

双方 TRANSIENT_LOCAL 支持 Writer 预发布后 Reader 独立进程加入并借用保留样本；VOLATILE Reader 只收新数据。晚加入 OFF Reader 使用按需 CDR。BEST_EFFORT 历史在匹配时发送一次，不承诺任意发现顺序或丢包下都送达。

ON 强制本地准备成功，对端兼容但共享映射失败仍使用 RTPS。兼容Reader的映射/注册暂时失败会自动退避重试，条件恢复后同一个Reader转到DS。Reader 默认无限 autopurge 时，已知键占用 max_instances 直到端点删除，take 和 return_loan 都不会释放实例预算。

重启期间旧端点的 DDS lease 可能尚未到期，Writer 可为这些旧目标准备 CDR；新 native Reader 仍走共享收样。应用已持有的 Reader Loan 可跨 Writer 死亡、段精确 unlink、Writer 重启及 Reader 删除保持可读，直至主动归还/释放。若持元数据锁的进程死亡导致段 CORRUPT，应重建端点，不能继续借出该段。

## 当前失败原子性与回收语义（2026-10-28）

native类型输出的大小/对齐不兼容返回BAD_PARAMETER，纠正参数后仍可读取同一样本。十种序列read/take及instance/condition/next变体统一preview/materialize/commit；输出分配、复制、CDR解码失败不消费history或将其提前标记已读。旧动态TypeSupport没有尺寸元数据，仍依赖其原有解码约定。

Reader有限autopurge删除实例身份后，旧句柄的get_key_value返回BAD_PARAMETER，lookup_instance返回HANDLE_NIL；同键可重新创建，按新的状态转移重新计时。已交付的Loan和SampleInfo引用共同保活原始缓存；必须分别return/unloan，两者最后引用释放前共享槽位不会复用。默认无限配置保持已知键，take/return_loan不会隐式删除实例。

调度及Timer同步、deadline listener查询状态/修改QoS/注销的当前回归已加入验收。上文“尚未验证”段落保留为历史范围描述，当前结果见DATASHARING-ACCEPTANCE.md；未运行的检查器不会由ASan代替。


本轮最终验收：普通与ASan/UBSan各468/468，两版各55项×30轮及三个有限示例各30轮通过；新增生产行覆盖率94.118%（2432/2584）。独立TSan限定路径通过，LSan未运行、历史WaitSet根因未知。当前完整证据与限制以 [本轮报告](DATASHARING-COMPLETION-20261028.md) 为准，早期数字只保留历史含义。
