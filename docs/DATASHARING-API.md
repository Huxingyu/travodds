# DataSharing / Loan API（当前实现）

> 最终受测代码提交 `f32da8390b0af07500f67eb1d0a1279a9c968f4f`，共享布局 **v7 / discovery v5**；`5609` 项源码/测试/依赖/构建工具输入的同一规范身份为 `65a3cde4be51c42897dfd974a2eea661e4c74b06abd1d2a5d40a18810da82861`。当前状态 **通过（限本页所述范围）**，实测 plain/coverage 各 555、ASan/UBSan 与 TSan 各 308、有限示例 9、覆盖率 95.556%（3096/3240，144 个翻译单元）。结果仅以[本次报告](DATASHARING-PROGRESS-20261101.md)和[需求矩阵](DATASHARING-ACCEPTANCE.md)的新证据为准；下文有日期的快照只证明各自历史。


当前实现已接通 Linux 上固定布局类型的自动共享发布/接收。共享能力与实际映射注册兼容的 Reader 从共享段收样；混合匹配中的其他 Reader 继续使用定向 RTPS。纯共享路径可跳过两端完整载荷序列化/反序列化。零载荷复制限定 Writer Loan → 空输出序列的 Reader Loan；普通裸指针 write 会复制样本，预分配输出也复制。带键类型只有在 Writer 与 Reader 两端的 TypeSupport 均提供有效 key-only 元数据并实现下述钩子时才可进入 native。原生路径不再保留完整样本键快照回退；不满足契约的类型保留 CDR/RTPS。键字段和控制元数据仍会复制，“零载荷复制”不表示系统中没有任何字节复制。存在 SQL Reader 过滤器时继续走 CDR。键哈希仍通过 TypeSupport 的键操作计算，Ubuntu 验证范围见需求矩阵。配置见 [QoS 手册](DATASHARING-QOS.md)。

## 类型声明

旧 TypeSupport 可以继续使用 CDR/RTPS，默认不声明原生布局或无初始化能力；已有原生类型必须补齐无初始化声明，带键类型还必须满足 key-only 契约。新固定布局类型可从 `NativeTypeSupport<T>` 继承，像已有 TypeSupport 一样实现序列化、复制、类型名和键操作，并向基类构造函数提供版本化布局 ID。仓库没有把任意 IDL 自动适配为 native 的生成器；示例的能力声明及键钩子为手写实现。HelloWorld 无界 string 正常保留 CDR/RTPS。

T 必须是标准布局、可平凡复制和析构的无指针类型；NativeTypeSupport 的默认 native 资格还要求 trivially default constructible。带默认成员初始化或用户默认构造函数的旧类型仍可编译并使用 CDR，但默认不允许 native。指针类型也可能满足 C++ trivial 属性，因此应用必须另外保证字段不含指针。布局 ID 应包含协议版本与 ABI 约束，改变成员顺序、大小、对齐或含义时必须更换。`std::string`、`std::vector` 等类型不能放入共享样本。

`get_native_type_info()` 返回 bounded/plain、size、alignment 和 layout_id；bounded 与 plain 分别声明。统一门控 `supports_native_data_sharing(info)` 同时要求有效布局和 `supports_native_uninitialized_sample()` 为 true，带键类型还要求 `NativeKeyInfo::valid_for(info)`。无初始化能力的保守默认值为 false；直接继承 TypeSupport 的实现需显式声明并遵守契约。NativeTypeSupport 使用 placement new(storage) T 开始对象寿命，不使用 T{}，借出不保证零初值。类型声明是应用契约，enable 不用假样本探测构造或键钩子；实际 capture/restore 失败关闭，不复制完整样本兜底。

## Writer

```cpp
SampleLoan loan;
auto rc = writer->loan_sample(loan);
if (rc == RETCODE_OK) {
    auto* value = static_cast<MyFixedSample*>(loan.data());
    // 发布前填写所有将被 Reader、键钩子或回退序列化器读取的字段。
    value->index = 1;
    rc = writer->write(loan, HANDLE_NIL);
    if (rc != RETCODE_OK) {
        writer->discard_loan(loan); // 也可以保留 loan 重试
    }
}
```

- `loan_sample(SampleLoan&)`：仅支持通过原生准入校验的类型，开始对象寿命但不初始化载荷，不保证字段为零；应用发布前必须填写所有将被读取的字段。缺失无初始化能力，或带键类型缺失/非法 NativeKeyInfo 时，ON enable 返回 UNSUPPORTED；AUTO 保留普通 CDR/RTPS 且不广告 DS，后续 loan_sample 返回 UNSUPPORTED。Linux Writer 在 enable 时准备共享池，默认 64 个槽位，目录 `/dev/shm`；通过 `qos.data_sharing` 配置。OFF 或 AUTO 在 enable 时未能准备共享段的端点返回 UNSUPPORTED。
- `write(SampleLoan&, handle)`：句柄必须属于此 Writer。成功后句柄为空，原地址不得继续访问；失败时句柄及数据保持可用。有已注册且重放准备 ready 的兼容 Reader 时直接发布借出的槽位；其他匹配 Reader 获取一次生成的 CDR。TRANSIENT_LOCAL Writer 尚无 Reader 时也保留原生历史；VOLATILE 且没有共享目标时沿用普通 RTPS。
- `discard_loan(SampleLoan&)`：归还未提交的句柄并清空。错误 Writer、重复操作和空句柄返回 PRECONDITION_NOT_MET。不修改其他 Writer 的借用。
- `SampleLoan` 只能移动，移动后源句柄为空。析构或 `reset()` 自动取消。Writer 删除后句柄仍保活映射，应用可取消或析构，不能再调用已删除的 Writer。
- `write(loan.data(), handle)` 是普通裸指针写入，不消费句柄；要使用借用的所有权语义，应使用句柄重载。

| 情况 | 结果 |
|---|---|
| Writer 未 enable | NOT_ENABLED |
| 类型未声明兼容布局/无初始化能力、带键类型缺失/非法 key-only 元数据，或非 Linux 实现 | UNSUPPORTED |
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

普通 RTPS Reader 的 payload_pool 配置控制 history 准入拷贝：完整 CDR 分片链事务性转入本地 Topic 池，native SharedReadLoan 保持旁路，transport 整包及独立 CDR 键快照仍归原池。五个普通池参数不可变，活动 domain/topic/type 池配置必须一致；前轮 19 项 Reader 回归覆盖真实接收、耗尽回滚与缓存保活，已纳入本次新源码完整模块并通过验证，详见 [普通池手册](PAYLOADPOOL-QOS.md)。

Reader 在发现兼容 Writer 后，以本地目录配置及对端 GUID/会话推导文件名，验证共享段并注册自身 GUID。独立监听线程通过原子代次与 Linux futex 等待共享通知，生成持有 SharedReadLoan 的原生缓存，再交给现有可靠性代理与 history。可靠和尽力模式、同参与者内的 ON/OFF Reader 混用均有独立 exec 进程测试。

- `has_loan()` 判断是否有活跃借用；`has_ownership()` 判断序列是否拥有样本存储。空序列和借用序列均不拥有样本，借用序列 maximum 为 0、length 为实际样本数。
- `return_loan(values, infos)` 必须使用相同 Reader 与同批配对序列。错误 Reader、错配批次或数据/元信息角色错误返回 PRECONDITION_NOT_MET，已有借用保持有效。
- 成功归还后双方 length/maximum 为 0，不再有借用。双方均无借用时返回 OK，保留自有数据；归还后的空配对重复调用同样无副作用。
- 未归还时再次把相同输出用于 read/take，返回 PRECONDITION_NOT_MET。无数据返回 NO_DATA；非法样本数、空/其他 Reader 的 condition、把同一序列同时作为数据和元信息返回 BAD_PARAMETER。
- 借用时禁止 push_back 和 length 修改。复制会生成独立自有样本，移动转移借用。序列析构自动释放本方 pin；另一方仍存活时继续保活样本。
- read 保留 Reader history，take 仅移除 history 引用。最后一个 history/应用引用消失后才归还共享槽位。Reader 删除不会使已经借出的样本失效；删除后通过序列析构/unloan 释放，不调用已删除 Reader。

所有这些序列操作都要求应用避免并发修改同一个序列对象。Writer Loan 并发借出/取消已有测试；原生跨进程存储的崩溃/代次协议见设计文档。

此次扩展改变了 TypeSupport/DataWriter 的虚函数接口及序列对象布局，无初始化及 key-only 钩子均影响 TypeSupport 二进制 ABI。库、应用及自定义 TypeSupport 子类必须一起重新编译，不能仅替换动态库后继续使用旧二进制；保守默认实现保留旧类型的 CDR/RTPS 源码兼容。NativeKeyInfo 的具体大小/对齐仍是本地元数据；当前 discovery v5 隔离旧 v1/v2/v3/v4 原生语义，共享布局 v7 包含真实发布历史环。

原生 Reader Loan 指向生产只读载荷页，修改样本会被内核拒绝；Reader 的注册和 pin 元数据仍可写。Writer Loan 保持可写，应用直接填写后发布。环按成功 publish 的 DDS 序列排序，与 loan 借出先后无关；取消未发布 Loan 不加入历史，仍被应用持有的 Reader Loan 不随历史淘汰失效。页对齐、预算和独立 PROT_NONE 取证范围见 [设计说明](DATASHARING-RING.md)。

## 验证与覆盖率

本轮完整证据与最终审计已通过，范围与证明边界见下文。文档单独提交，不改变受测代码身份；之后源码/测试/构建工具再变化，受影响证据必须重跑。本报告不声称文档已提交或分支已推送；普通推送 origin 的最终 HEAD 以交付记录为准，不 force、不推 upstream/gitee、不提 PR。

最终受测代码提交 `f32da8390b0af07500f67eb1d0a1279a9c968f4f`，共享布局 **v7 / discovery v5**；`5609` 项源码/测试/依赖/构建工具输入的同一规范身份为 `65a3cde4be51c42897dfd974a2eea661e4c74b06abd1d2a5d40a18810da82861`。plain、coverage 普通全量各 **555 = 259 模块 + 49 共享池 + 247 core**；ASan/UBSan、TSan 各 **308 = 259 + 49**（不含 core）；有限示例 **9 次**（Loan/keyed/mixed 各 3），新增生产可执行行覆盖率 **95.556%（3096/3240，144 个翻译单元）**。

本轮实际证据：[冻结输入](validation/20261101/final/round2/source-manifest.json)、[最终审计](validation/20261101/final/round2/audit/final-audit.json)、[statistics 原始结果](validation/20261101/final/statistics/attempt2/statistics-result/results.json)、[费用/用量记录](validation/20261101/final/usage-ledger.json)。受测实现 f32da8390b0af07500f67eb1d0a1279a9c968f4f，源码身份 65a3cde4be51c42897dfd974a2eea661e4c74b06abd1d2a5d40a18810da82861（5609 项输入）；plain/coverage 各 555 = 259 模块 + 49 共享池 + 247 core，ASan/UBSan 与 TSan 各 308（不含 core），9 次有限示例，新增生产可执行行覆盖率 95.556%（3096/3240，144 个翻译单元）。 审计核对四模式真实 XML/log、用例身份、同一源码、私有库 ELF/插桩、原始载荷 proof、覆盖率及资源清理；此前 Modal 独立复验和 followups 仅保留其原有历史身份。

本次 plain/coverage/asan/tsan 四模式均在 Modal 从同一最终源码重新编译生产库和相应测试程序，不复用旧对象、旧二进制或旧覆盖计数；Ubuntu 24.04、g++ 13.3，固定 CPU=2、memory=4096 MiB。冻结清单包含 thirdparty、GTest zip、实际编译源码/头文件和构建脚本，逐文件 SHA 与最终代码身份一起绑定；清单数量以实际审计为准。

各模式模块 259 项在 Modal 执行；完整共享池 49 项在本机真实内核运行，plain/coverage 的 core 247 项也在本机运行。ASan/UBSan 与 TSan 不包含 core。按预先约定分工补齐 gVisor 的 futex 观测、IPv6 和网络环境限制，回传 ELF/私有库与 Modal 产物 SHA 完全相同，用例身份无遗漏；原始环境失败记录保留，不能把真实竞争归为环境限制。

每模式 Modal 模块内四项 NativePayloadWrites 均留原始 proof；本机使用相同 ELF/私有库独立补跑四项，由 tools/ospp/native_payload_writes.py 重解析 XML/log/proof。重复身份仅增加真实内核证据，不额外计数。Loan/keyed/mixed 各 3 次有限示例在本机 coverage 模式自然退出，共 9 次，实际收发计数通过核对。

覆盖率实测 95.556%（3096/3240，144 个翻译单元），最低要求 70%。使用 -fprofile-update=atomic，Modal/local 各从空的新计数目录采集，合并相同编译身份的新 gcda，全部实际链接 gcno（包括无 gcda 的零命中单元）进入统计。tools/ospp/coverage.py 按固定基线 489067108405b27afbd369245542f5e88d457f18 重算新增生产可执行行；原始生产负计数 0 个，负计数均按未覆盖处理，未复用旧计数。这不是分支覆盖率或全仓库文本行覆盖率。

statistics() 通过非阻塞 trylock 获取控制面快照，合法并发持锁可返回 TIMEOUT。原测试在子 Reader listener 仍运行时直接要求关闭快照返回 OK，没有建立必要的静止前提，属于测试握手缺口。本轮在真实 statistics Guard 已成功持有真实共享 mutex 后仅暂停执行：子工作线程的 PID/TID、mutex owner TID 和解析到 readerdatasharing.cpp 的调用栈均匹配。旧测试原 ASSERT 确定性失败，退出 1、TIMEOUT；增加 D/C/L 握手，等待 Reader 删除返回并完成 listener join，同时保留首条应用 Loan，原 OK、writer_alive=false、pending=0、loan_references=1 精确断言全部通过，退出 0。仅修测试的同步前提，未改产品统计语义、未新增 sleep/重试或放宽断言。这证明一种真实 listener 的同类合法交错可以产生该失败；历史偶发轮没有记录持锁者，不能据此声称已经识别那个历史 holder。原全量约 8 轮 1 次失败、单独 Modal 200 次/本机 40 次未复现均仍为历史现象。见 before：`artifacts/datasharing-final-20261101/statistics/attempt2/statistics-result/before/summary.json`（本机证据，不随仓库分发）、after：`artifacts/datasharing-final-20261101/statistics/attempt2/statistics-result/after/summary.json`（本机证据，不随仓库分发）。

旧首次 get_matched_publication_data 返回 3（BAD_PARAMETER）时没有记录所选 peer GUID，根因未证实。现测试采用独立 exec 对的 Topic，并断言精确父 Writer GUID/handle，lookup 原断言保留；后续通过和一次带 GUID 诊断均不能证明最初失败来自旧端点。原失败仍在 tsan-final 原始日志：`artifacts/datasharing-followups-20261101/tsan-final/logs/test-loan_part_1.log`（本机证据，不随仓库分发）。

最终受测代码 `f32da8390b0af07500f67eb1d0a1279a9c968f4f` 已完成本机连续 `0bbeaed → f32da83 → 0bbeaed` 三组性能重测，各 27/27、合计 81/81 自然退出，零重跑。64 B RTPS 吞吐 28292.30 samples/s（相对两基线 -2.47% / +1.07%）及 p99 271.738 μs 均在双基线中位区间内；4 KiB ordinary 吞吐 -7.13% / -11.49%、p99 +141.02% / +204.07%，超出预设双基线差幅，退化观测必须保留，具体原因仍未归因。不能将这些差异归因新增 ReaderProxy 锁或声称整体没有影响。64 KiB RTPS 目标约 70.97 samples/s、p95/p99 约 198/200 ms；两基线吞吐本身差幅 60.36%。完整数据、噪声规则和旧轮观测见 [性能报告](DATASHARING-PERFORMANCE.md)，旧 64 B RTPS -5.61%/p99 0.94 ms 仍只属于历史测量。结果来自 2 核共享 VM 三轮短测，不是最大可持续吞吐、专机因果归因或 SLA。软件层“只写一次”验证应用在最终共享槽位单遍填写及框架不搬运完整载荷，不证明 DRAM 写入次数；键和控制元数据仍可复制。“无锁”限于通知及共享数据面不获取跨进程管理 mutex，不证明整个 DDS write 形式化 lock-free/wait-free。AUTO/ON 同 kind 的跨主机 RTPS/UDP 回退已有 24 组实测通过，链路为 SSH 虚拟二层；跨主机 OFF/mixed 和物理局域网仍未实测，详见 [验收矩阵](DATASHARING-ACCEPTANCE.md)。ASan 关闭 LSan，未做泄漏扫描，不能写“无泄漏”。

本机 jobs=1、nice=19，未新增 swap/本机支出，不在远端存放凭据，未再连接不可用 ECS。Modal 固定 2 CPU/4096 MiB、Ubuntu 24.04/g++ 13.3、无 GPU；本任务请求资源时长与下载量估计 $0.086696，含预留的保守估计 $0.488881，低于 $1 授权上限，已结算记录不含待运行预留；不是账单。最终审计确认 Modal app 已停止、/dev/shm 无本任务新增残留、旧 strict/portable 保护路径核验通过。

本次继承的回放修复避免只读扫描/无关回放 lease 占用已消费空闲节点；retained replay pin、Writer、pending 和应用 Loan 仍阻止槽位复用。Reader 发布后重复绑定已删除，同非空端点重绑幂等；Writer 过滤器 map 在既有锁下取拥有型快照，SQL/TypeSupport/RTPS 锁外执行。这些改动须由本次最终身份检查，历史定向通过不能替代新证据。

旧 ecb4b3c 的严格补全快照为 542/542、9 示例、94.132%（2984/3170，143 TU）和检查器各 99 项；v6/v4 为 529/9/93.936% 和检查器各 86 项。旧 strict/portable 两目录只读保留，不能给本次新源码背书。

首次诊断归档 attempt1：`artifacts/datasharing-final-20261101/statistics/attempt1/statistics-result/results.json`（本机证据，不随仓库分发） 为配置失败：源码包遗漏 cmake/，configure-library 退出 1，未构建/执行任何测试；不得计为测试失败或通过。补齐源码包的 attempt2 保留独立输入/工具/程序 SHA 与原始 before/after 证据；旧归档不覆盖。

首轮受测 556c918d7e9659e42c266c5c524d8f2c038fb0c0 的 TSan 在 ReaderProxy::RequestedChangesSet 写请求 vector 与异步 RequestedChanges 复制读取之间报告真实数据竞争。模块退出 66，用时 12.542 s、限时 180 s，timed_out=false；尚有子进程 [2511]，清理记录 killed_pids=[1065, 2511]。这不是超时或 gVisor 环境失败。旧私有容器原始 shm_added 保留一个 DS 段并判失败，不能改写为旧轮无残留。后续已确认旧私有容器所属 app 停止且任务数为 0；停止记录不等于现场验证原段已删除，原始残留仍作为失败事实保留，与成功轮最终无新增残留分开。产品最小修复单独提交后，本轮在新的源码身份上从零重跑全部四模式、示例、覆盖率及 proof；旧 plain/coverage/ASan 的成功和旧失败 TSan 都不能替代新提交的验收。详见 原始 race 分析：`artifacts/datasharing-final-20261101/race/README.md`（本机证据，不随仓库分发）与首次 TSan 原始结果：`artifacts/datasharing-final-20261101/remote/tsan/results.json`（本机证据，不随仓库分发）。



以下是 2026-10-28 完整检查流程的复现命令，测试数量和结果属于当时的冻结版本。新运行使用新的输出目录，所有编译保持单线程：

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

`CORE_TESTS=1` 在该历史版本额外运行 247 项核心测试；不设置时运行池/Loan 集成与共享池测试。测试数量以新运行的实际 XML 为准。

覆盖率统计 GCC 标记的可执行生产代码行，取相对指定固定基线的新增行，合并库与测试翻译单元中的模板实例；不把测试、依赖和文档计入分母。QoS 阶段的历史快照为 1166/1225（95.18%），本轮测量以当前报告为准；这不是分支覆盖率，也不证明未实现的功能已完成。完整运行记录见 `OSPP-261760062.md`。


### 自动收发阶段验证快照（2026-10-27）

无键固定布局类型已在可靠/尽力 × 纯共享/同参与者混合 Reader 的独立 exec 场景验证。普通版 247+104+22 项通过，独立 ASan/UBSan 104+22 项通过（无泄漏扫描），新增生产代码行覆盖率 1504/1617（93.012%）。最终证据与剩余边界见 `DATASHARING-PLAN.md`，旧阶段覆盖率数字仅保留其历史含义。


### 普通池公开配置阶段验证快照（2026-10-27）

`DataWriterQos::payload_pool` 与 XML 的 `<payload_pool>` 可选四种策略、容量和对齐，详细块计数与预留空间说明见 [普通池配置手册](PAYLOADPOOL-QOS.md)。当前普通测试 247+113+22=382 项，独立 ASan/UBSan 113+22=135 项通过；两版各 30 轮独立进程重复验证通过，新增生产代码行覆盖率 1559/1682（92.687%），无泄漏扫描结论。完整验收边界继续以实施清单为准。


### 带键实例与本地快照

带键原生类型与无键类型采用相同的布局约束。TypeSupport 的 `MakeKey` 必须只根据键字段稳定生成实例句柄，成功返回 0；键字段可以使用键序列化与 MD5，而纯共享的 `serialize_data` / `deserialize_data` 调用次数为 0。dispose/unregister 通过 RTPS inline key/status 通知，不能据此声称不发送网络控制消息。

- 带键类型进入 native 必须实现下列 TypeSupport 钩子并声明有效 NativeKeyInfo。capture 只能读取键字段，restore 只能修改目标的键字段。元数据和表示在 TypeSupport 生命周期内保持稳定，快照必须是无指针、无需构造/析构的独立字节，不能保留样本地址。具体键表示无需在对端共享，但 Writer 与 Reader 各自在准入时校验它；当前还要求 discovery v5 和共享布局 v7 兼容，旧 v1/v2/v3/v4 对端保留 RTPS。

```cpp
virtual NativeKeyInfo get_native_key_info() const;
virtual bool capture_native_key(void* snapshot, const void* sample) const;
virtual bool restore_native_key(void* sample, const void* snapshot) const;
```

- Writer 首次注册和 Reader 首次接受该实例 ALIVE 时仅分配 NativeKeyInfo::size 字节；实际分配对齐至少为 max_align_t。`get_key_value` 通过 restore 还原键，非键字段保留调用前值。相同键再次注册返回原句柄，重复查询不消费快照。
- size 必须非零且不超过原生样本大小，alignment 必须非零、为二次幂且不大于原生样本对齐。缺失/非法元数据在 ON enable 阶段返回 UNSUPPORTED；AUTO 不广告 DS，普通 write 使用 CDR/RTPS，loan_sample 返回 UNSUPPORTED。内部 native 创建/查询若遇到缺失/非法元数据返回 BAD_PARAMETER，capture/restore false 或标准异常返回 ERROR，分配失败返回 OUT_OF_RESOURCES；register_instance 只返回句柄，相关失败表现为 HANDLE_NIL。
- Reader 首次键快照捕获失败时拒绝本条准入，在排序、过滤、淘汰和所有权状态变更前回滚刚创建的空实例，不占 max_instances；已有实例与历史不因失败删除。已有快照的 restore 失败返回 ERROR 并保留快照，便于修正后重新查询；调用方不应依赖失败查询的输出内容。
- 默认钩子不声明能力，带键类型不能进入 native；即使应用提供元数据但保留默认 capture/restore，实际操作也失败，不回退原生全样本 memcpy/copy_data。普通 CDR 注册保留序列化快照并通过原池回收，不受严格 native 快照规则影响。
- 快照不占共享槽位、不保留源样本引用，直到实例回收/注销或实体销毁才释放；take/return_loan 后仍可查询已知键。CDR Reader 保存独立缓冲链，每次查询使用独立的全部分片游标。
- `lookup_instance` 对未知键、空数据或键计算失败返回 HANDLE_NIL。注册的空数据、负时间戳和资源耗尽也返回 HANDLE_NIL；隐式注册的 write 在实例数耗尽时返回 OUT_OF_RESOURCES。
- write 使用未注册的显式句柄返回 BAD_PARAMETER；样本键与已注册句柄不一致返回 PRECONDITION_NOT_MET。dispose/unregister 可以用空数据加有效句柄，空数据与 HANDLE_NIL 的组合返回 BAD_PARAMETER。
- 注销成功后 Writer 的句柄不再可查，键快照释放；相同键可重新注册。Reader 保留其已知实例身份，并通过 dispose、无 Writer、重新 ALIVE 的通知跟踪实例状态与代数。

每个已知实例额外保留一份本地快照，成本不包含在 `data_sharing.max_samples` 或段预算内。native 键存储约为 `NativeKeyInfo::size × 已知实例数`，另有本地分配/索引开销，CDR 快照按序列化长度计。键快照及索引是会复制的元数据，不是完整用户载荷；这里不作“任何字节都不复制”的保证。Writer 可通过 `resource_limits.max_instances` 限制注册数；Reader按有限生命周期回收已知实例；默认无限时仍保留预算占用，具体语义见QoS手册。

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

该历史阶段对可靠/尽力接收逻辑的验证见实施清单；当前已运行的独立进程与并发代表性组合见 [10/31 报告](DATASHARING-PROGRESS-20261031.md)，不宣称故障、发现租约或线程交错穷尽。


### 已排队的 requested deadline 回调

接收生命周期通知或最后一个 Writer 消失，可以与已开始的超时回调交错。超时推进在 history 锁内重新确认实例仍为 ALIVE；否则停止该次监视并跳过 missed 通知，再扫描其他活动实例。这里保留已知实例身份和键快照，不删除实例。该确定性交错已有回归；并发set_qos与Timer内部同步现已实现，并有有界压力与回调重入用例，完整结果见本轮验收矩阵。


### 晚加入、容量与故障恢复

双方 TRANSIENT_LOCAL 支持 Writer 预发布后 Reader 独立进程加入并借用保留样本；VOLATILE Reader 只收新数据。晚加入 OFF Reader 使用按需 CDR。BEST_EFFORT 历史在匹配时发送一次，不承诺任意发现顺序或丢包下都送达。

ON 强制本地准备成功，对端兼容但共享映射失败仍使用 RTPS。兼容 Reader 的映射/注册暂时失败会自动退避重试，注册与重放 ready 后同一个 Reader 转到 DS；损坏 native session 不承诺透明恢复。Reader 默认无限 autopurge 时，已知键占用 max_instances 直到端点删除，take 和 return_loan 都不会释放实例预算。

重启期间旧端点的 DDS lease 可能尚未到期，Writer 可为这些旧目标准备 CDR；新 native Reader 仍走共享收样。应用已持有的 Reader Loan 可跨 Writer 死亡、段精确 unlink、Writer 重启及 Reader 删除保持可读，直至主动归还/释放。若管理锁持有者死亡，或已死亡登记遗留无法安全归属的短操作计数，session 进入 CORRUPT，应重建端点；已有 Loan 仍保留映射，不能继续从损坏段借出新样本。

## 当前失败原子性与回收语义（2026-10-28）

native类型输出的大小/对齐不兼容返回BAD_PARAMETER，纠正参数后仍可读取同一样本。十种序列read/take及instance/condition/next变体统一preview/materialize/commit；输出分配、复制、CDR解码失败不消费history或将其提前标记已读。旧动态TypeSupport没有尺寸元数据，仍依赖其原有解码约定。

Reader有限autopurge删除实例身份后，旧句柄的get_key_value返回BAD_PARAMETER，lookup_instance返回HANDLE_NIL；同键可重新创建，按新的状态转移重新计时。已交付的Loan和SampleInfo引用共同保活原始缓存；必须分别return/unloan，两者最后引用释放前共享槽位不会复用。默认无限配置保持已知键，take/return_loan不会隐式删除实例。

调度及Timer同步、deadline listener查询状态/修改QoS/注销的当前回归已加入验收。上文“尚未验证”段落保留为历史范围描述，当前结果见DATASHARING-ACCEPTANCE.md；未运行的检查器不会由ASan代替。


当前布局仍为 v7 / discovery v5，最终源码与新完整证据的状态为 通过（限本页所述范围），见 [11/1 报告](DATASHARING-PROGRESS-20261101.md)和[环形历史与载荷写入说明](DATASHARING-RING.md)。前轮 v6/v4 的 321 项冻结输入对应 529 项普通回归、9 次示例、93.936% 覆盖率和两个检查版各 86 项，原始范围保留于 [前轮报告](DATASHARING-PROGRESS-20261031.md)；这些数字不能作为本轮新源码通过结果。[10/30 报告](DATASHARING-STRICT-20261030.md) 继续保留更早 v3 的历史。LSan 未启用，历史 WaitSet 崩溃根因仍未知。
