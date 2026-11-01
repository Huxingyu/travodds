# DataSharing 最终收尾与验收报告（2026-11-01）

本轮完整证据与最终审计已通过，范围与证明边界见下文。文档单独提交，不改变受测代码身份；之后源码/测试/构建工具再变化，受影响证据必须重跑。本报告不声称文档已提交或分支已推送；普通推送 origin 的最终 HEAD 以交付记录为准，不 force、不推 upstream/gitee、不提 PR。

最终受测代码提交 `f32da8390b0af07500f67eb1d0a1279a9c968f4f`，共享布局 **v7 / discovery v5**；`5609` 项源码/测试/依赖/构建工具输入的同一规范身份为 `65a3cde4be51c42897dfd974a2eea661e4c74b06abd1d2a5d40a18810da82861`。起点 `0bbeaed` 已包含回放 c78607d、Reader/过滤器竞态 0caef22 及性能记录 0bbeaed；此前 Modal 独立复验不是本次严格验收。当前新证据根为 `artifacts/datasharing-final-20261101/`。

## statistics TIMEOUT 的分类与最小改动

statistics() 通过非阻塞 trylock 获取控制面快照，合法并发持锁可返回 TIMEOUT。原测试在子 Reader listener 仍运行时直接要求关闭快照返回 OK，没有建立必要的静止前提，属于测试握手缺口。本轮在真实 statistics Guard 已成功持有真实共享 mutex 后仅暂停执行：子工作线程的 PID/TID、mutex owner TID 和解析到 readerdatasharing.cpp 的调用栈均匹配。旧测试原 ASSERT 确定性失败，退出 1、TIMEOUT；增加 D/C/L 握手，等待 Reader 删除返回并完成 listener join，同时保留首条应用 Loan，原 OK、writer_alive=false、pending=0、loan_references=1 精确断言全部通过，退出 0。仅修测试的同步前提，未改产品统计语义、未新增 sleep/重试或放宽断言。这证明一种真实 listener 的同类合法交错可以产生该失败；历史偶发轮没有记录持锁者，不能据此声称已经识别那个历史 holder。原全量约 8 轮 1 次失败、单独 Modal 200 次/本机 40 次未复现均仍为历史现象。见 [before](../../../artifacts/datasharing-final-20261101/statistics/attempt2/statistics-result/before/summary.json)、[after](../../../artifacts/datasharing-final-20261101/statistics/attempt2/statistics-result/after/summary.json)。

测试修复提交 `556c918` 只加入关闭/释放握手；最终 test_loan.cpp 的 SHA 与确定性 after 候选原字节一致。首次诊断归档 [attempt1](../../../artifacts/datasharing-final-20261101/statistics/attempt1/statistics-result/results.json) 为配置失败：源码包遗漏 cmake/，configure-library 退出 1，未构建/执行任何测试；不得计为测试失败或通过。补齐源码包的 attempt2 保留独立输入/工具/程序 SHA 与原始 before/after 证据；旧归档不覆盖。

## 本轮捕获的 ReaderProxy 产品竞争

首轮 TSan 捕获 ACKNACK 线程的 `RequestedChangesSet()` 与异步重传线程的 `RequestedChanges()` 同时访问同一个 vector。后者原本已按值返回，但复制本身没有同步；写线程外层的 Writer 锁覆盖不到异步读取。产品修复提交 `f32da83` 仅为替换、快照及 `NextRequestedChange()` 三处访问加入现有 `ackmtex_` 保护。持锁块只操作代理自己的集合，不调用 history、Writer 或 transport，返回副本后才执行这些操作，未增加反向锁序。

三条永久 `ReaderProxyRequests` 回归检查快照独立所有权、并发完整快照和并发首项读取；两条并发用例用共同起点与固定 256 次操作，无新增 sleep 或失败重试。它们已进入四模式完整程序，使模块身份从 256 增至 259。原始真实 race 作为修复前证据完整保留，本轮四模式全部从该产品修复提交重新构建。

## 四模式全量证据（实际完成）

| 模式/范围 | Modal 执行 | 同字节产物回本机真实内核执行 | 完成的唯一身份数 | 状态 |
|---|---|---|---|---|
| plain 普通 | payloadpool 259 | 共享池 49 + core 247；四项 proof 另补跑 | 555，proof 重复不加数 | 通过（限本页所述范围） |
| coverage 普通 | payloadpool 259 | 共享池 49 + core 247；四项 proof 另补跑；有限示例 9 | 555；覆盖率 95.556%（3096/3240，144 个翻译单元） | 通过（限本页所述范围） |
| ASan/UBSan | payloadpool 259 | 共享池 49；四项 proof 另补跑 | 308，不含 core/LSan | 通过（限本页所述范围） |
| TSan | payloadpool 259 | 共享池 49；四项 proof 另补跑 | 308，不含 core | 通过（限本页所述范围） |

本次 plain/coverage/asan/tsan 四模式均在 Modal 从同一最终源码重新编译生产库和相应测试程序，不复用旧对象、旧二进制或旧覆盖计数；Ubuntu 24.04、g++ 13.3，固定 CPU=2、memory=4096 MiB。冻结清单包含 thirdparty、GTest zip、实际编译源码/头文件和构建脚本，逐文件 SHA 与最终代码身份一起绑定；清单数量以实际审计为准。

各模式模块 259 项在 Modal 执行；完整共享池 49 项在本机真实内核运行，plain/coverage 的 core 247 项也在本机运行。ASan/UBSan 与 TSan 不包含 core。按预先约定分工补齐 gVisor 的 futex 观测、IPv6 和网络环境限制，回传 ELF/私有库与 Modal 产物 SHA 完全相同，用例身份无遗漏；原始环境失败记录保留，不能把真实竞争归为环境限制。

每模式 Modal 模块内四项 NativePayloadWrites 均留原始 proof；本机使用相同 ELF/私有库独立补跑四项，由 tools/ospp/native_payload_writes.py 重解析 XML/log/proof。重复身份仅增加真实内核证据，不额外计数。Loan/keyed/mixed 各 3 次有限示例在本机 coverage 模式自然退出，共 9 次，实际收发计数通过核对。

覆盖率实测 95.556%（3096/3240，144 个翻译单元），最低要求 70%。使用 -fprofile-update=atomic，Modal/local 各从空的新计数目录采集，合并相同编译身份的新 gcda，全部实际链接 gcno（包括无 gcda 的零命中单元）进入统计。tools/ospp/coverage.py 按固定基线 489067108405b27afbd369245542f5e88d457f18 重算新增生产可执行行；原始生产负计数 0 个，负计数均按未覆盖处理，未复用旧计数。这不是分支覆盖率或全仓库文本行覆盖率。

本轮实际证据：[冻结输入](../../../artifacts/datasharing-final-20261101/round2/source-manifest.json)、[最终审计](../../../artifacts/datasharing-final-20261101/round2/audit/final-audit.json)、[statistics 原始结果](../../../artifacts/datasharing-final-20261101/statistics/attempt2/statistics-result/results.json)、[费用/用量记录](../../../artifacts/datasharing-final-20261101/usage-ledger.json)。受测实现 f32da8390b0af07500f67eb1d0a1279a9c968f4f，源码身份 65a3cde4be51c42897dfd974a2eea661e4c74b06abd1d2a5d40a18810da82861（5609 项输入）；plain/coverage 各 555 = 259 模块 + 49 共享池 + 247 core，ASan/UBSan 与 TSan 各 308（不含 core），9 次有限示例，新增生产可执行行覆盖率 95.556%（3096/3240，144 个翻译单元）。 审计核对四模式真实 XML/log、用例身份、同一源码、私有库 ELF/插桩、原始载荷 proof、覆盖率及资源清理；此前 Modal 独立复验和 followups 仅保留其原有历史身份。

最终审计已核对所有模式源码/依赖/脚本身份、逐文件 SHA、完整 XML 用例集合及失败/禁用/跳过、程序/私有库 ELF 和实际插桩、原始 proof、Modal/local 新计数及零命中 notes、覆盖率算术、示例收发计数、Modal app 和共享内存清理。gVisor 不支持 setarch -R 不等于 TSan 未运行；须记录各环境实际命令。环境失败保留原退出/输出，在预定真实内核分工补齐相同身份的检查，不能将失败改写为通过或任意排除真实 race。

## 官方八项与交付

PayloadPool、共享段/真实历史环、跨进程通知、Loan API、零载荷搬运/RTPS 回退、DataSharingQosPolicy、测试/覆盖率/示例、设计/API/QoS/调试文档分别见 [八项验收矩阵](DATASHARING-ACCEPTANCE.md)。前六项继承的实现已在最终源码上完成回归；第 7 项的新完整结果、同一身份审计及第 8 项的文档实质更新均已完成。当前各项状态为通过（限本页所述范围），不以更换旧文档哈希或历史成功代替。

五个当前/历史入口文档与代码修复分开提交，最终文档不替换受测代码身份。源码再变更则重跑受影响检查。全部通过后普通推送 origin 的 work/datasharing-progress，记录最终 HEAD；任何未通过就不推送，解释原因，不 force、不提 PR、不推 upstream/gitee。

## 未解决现象与证明范围

旧首次 get_matched_publication_data 返回 3（BAD_PARAMETER）时没有记录所选 peer GUID，根因未证实。现测试采用独立 exec 对的 Topic，并断言精确父 Writer GUID/handle，lookup 原断言保留；后续通过和一次带 GUID 诊断均不能证明最初失败来自旧端点。原失败仍在 [tsan-final 原始日志](../../../artifacts/datasharing-followups-20261101/tsan-final/logs/test-loan_part_1.log)。

最终代码 `f32da8390b0af07500f67eb1d0a1279a9c968f4f` 已按原方法在本机完成连续 `0bbeaed → f32da83 → 0bbeaed` 性能重测：三组各 27/27，共 81 次自然退出，载荷/CDR 与原始日志核验通过，无失败或环境重跑。64 B RTPS 吞吐 28292.30 samples/s（相对基线1/2 -2.47% / +1.07%）及 p99 271.738 μs 均位于双基线中位区间内，未超出本次噪声；p50/p95 有改善观测。4 KiB ordinary 吞吐 -7.13% / -11.49%、p99 +141.02% / +204.07%，超出双基线差幅，必须保留退化观测，具体原因尚未查实。36 个指标有 7 个改善、2 个退化观测超出预设噪声规则，但组内范围交叠且宿主 CPU/I/O/换页不同，不能将差异归因新增 ReaderProxy 锁或声称统计显著、整体没有影响。64 KiB RTPS 目标约 70.97 samples/s、p95/p99 约 198/200 ms，与既有行为相符；两基线吞吐本身差幅 60.36%。[完整性能报告](DATASHARING-PERFORMANCE.md)和[本次三组证据](../../../artifacts/datasharing-perf-f32da83/attempt2/REPORT.md)包含逐组数据、噪声规则、RSS及宿主条件。前次监控误杀的 9/27 部分结果仅作旧现象诊断，未用于本轮比较，原证据和 `553ac93` 保留。旧轮 64 B RTPS 吞吐 -5.61%/p99 0.94 ms 仍是历史未归因观测，不改写为当前成绩；本次不做性能优化。软件层“只写一次”不证明 DRAM 写入次数；键和控制元数据仍可复制。“无锁”限于通知及共享数据面，不证明整个 DDS write 形式化 lock-free/wait-free。OFF/mixed/UDP 回退没有真实跨主机验证；ASan 关闭 LSan，未做泄漏扫描。性能为两核共享 VM 短测，不是 SLA。

旧无栈 WaitSet 崩溃根因仍未知；返回 3 的原始 peer GUID 亦未记录。statistics 的测试握手前提已用同类真实交错和精确断言验证，仍不冒称已知历史 holder；历史小消息 RTPS 退化及本次 4 KiB ordinary 退化观测均未完成因果归因。受控载荷证明不是恶意进程安全沙箱，TypeSupport 类型契约仍必须满足。

## 资源、保护目录与停止条件

本机 jobs=1、nice=19，未新增 swap/本机支出，不在远端存放凭据，未再连接不可用 ECS。Modal 固定 2 CPU/4096 MiB、Ubuntu 24.04/g++ 13.3、无 GPU；本任务请求资源时长与下载量估计 $0.086696，含预留的保守估计 $0.488881，低于 $1 授权上限，已结算记录不含待运行预留；不是账单。最终审计确认 Modal app 已停止、/dev/shm 无本任务新增残留、旧 strict/portable 保护路径核验通过。

资源估算/停止条件见本次 resource-plan.json；本任务不扩大计算规格，预计超过费用上限时停止说明。已有四轮约 0.12 美元仅为 TASK 的历史记录，最终需区分本轮估计/实测用量与历史费用。证据中没有的费用或测试不写成已验证。

`artifacts/strict-ring-20261031/` 和 `artifacts/portable-evidence-20261031/` 严格只读，旧 frozen/source/failed/success 数字完整保留。followups 的首轮 normal/ASan、TSan 失败、ECS 不可达与 stopped-status.json 也保持历史身份；新 Modal 4 模式不覆盖它们。最终审计已核对 Modal app 停止、私有进程退出及 /dev/shm 无本任务新增残留。最终结果 **通过（限本页所述范围）**。

首轮受测 556c918d7e9659e42c266c5c524d8f2c038fb0c0 的 TSan 在 ReaderProxy::RequestedChangesSet 写请求 vector 与异步 RequestedChanges 复制读取之间报告真实数据竞争。模块退出 66，用时 12.542 s、限时 180 s，timed_out=false；尚有子进程 [2511]，清理记录 killed_pids=[1065, 2511]。这不是超时或 gVisor 环境失败。旧私有容器原始 shm_added 保留一个 DS 段并判失败，不能改写为旧轮无残留。后续已确认旧私有容器所属 app 停止且任务数为 0；停止记录不等于现场验证原段已删除，原始残留仍作为失败事实保留，与成功轮最终无新增残留分开。产品最小修复单独提交后，本轮在新的源码身份上从零重跑全部四模式、示例、覆盖率及 proof；旧 plain/coverage/ASan 的成功和旧失败 TSan 都不能替代新提交的验收。详见 [原始 race 分析](../../../artifacts/datasharing-final-20261101/race/README.md)与[首次 TSan 原始结果](../../../artifacts/datasharing-final-20261101/remote/tsan/results.json)。
