# TravoDDS 问题与修复跟踪表

更新日期：2026-10-29。官方代码基线：`489067108405b27afbd369245542f5e88d457f18`。

**当前已完成 1 项独立上游修复并提交 PR：日志器并发崩溃 LOGGER-1，对应 [PR !115](https://gitee.com/agiros/travodds/pulls/115)。PR 当前尚未合并，不能称为官方版本已经修复。**此前 DataSharing 开发分支还修过多项问题，需要逐项拆分和复核后再提交上游。

## 统计口径与来源

| 项目 | 数量 / 状态 |
| --- | --- |
| 2026-10-28 审查记录 | 46 条；这是审查条目数，不等同于 46 个独立根因 |
| 原审查证据分类 | 7 条有运行复现、6 条引用此前开发修复、33 条主要是静态分析记录 |
| 原审查风险级别 | 严重 7、高 20、中 14、低 5；沿用原记录，并非本次重新评定 |
| 独立上游修复 PR | 1 项：LOGGER-1，PR !115，已提交、待合并 |
| 下表已知在开发分支修过 | LOGGER-1、SW-1、SR-1、TCP-1、SHM-1、RECV-1、XML-1；除 LOGGER-1 外尚未作为本轮独立 PR 交付 |

审查汇总由 DeepSeek 会话整理，其中包含之前开发时已发现、修复的缺陷，也包含其他模型先提出的线索。不能把全部 46 条都称为 DeepSeek 首次独立发现。本表保留原编号，便于回查。

“静态待复现”表示还需构造测试、确认触发前提和影响；不会因为写进表格就将其算成已证实的运行故障。“未独立处理”表示尚未完成本轮独立 PR，不代表已经逐行确认开发分支仍保留原缺陷。

## 已提交的独立修复

| 编号 | 问题 | 修复与验证 | 上游状态 |
| --- | --- | --- | --- |
| LOGGER-1 | 多线程首次写日志及并发销毁竞争，可能触发 `std::terminate()`；运行状态和日志掩码也有数据竞争 | 生命周期锁保护线程启停和回收；队列锁保护运行状态；日志掩码使用原子变量。原版普通回归复现 SIGABRT；原版 TSan 报告掩码写写竞争及运行状态读写竞争；修复后 15 项通过，两个并发用例各重复 10 次通过，TSan 15 项通过且无竞争报告 | [PR !115](https://gitee.com/agiros/travodds/pulls/115)，待合并；当前提交 `1edc5d9`（作者名更新，代码 tree 不变） |

本次 PR 只包含 `src/common/log/logger.cpp` 和 `test/unit_extra/test_logger_concurrency.cpp`，未混入 DataSharing 实现。GitHub 与 Gitee 的同名修复分支已同步；`main` 没有因提交说明的调整而改变。提交说明由英文改为中文并移除段内硬换行，代码 tree 保持一致。

证据：[普通回归原版](../artifacts/pr-logger-20261029/before.log)、[修复后](../artifacts/pr-logger-20261029/after.log)、[并发重复](../artifacts/pr-logger-20261029/after-stress.log)、[原版 TSan](../artifacts/pr-logger-20261029/before-tsan.log)、[修复后 TSan](../artifacts/pr-logger-20261029/tsan.log)。本轮是 Linux 日志模块独立验证，未重新运行整个 DDS 测试套件，也未验证 Windows。

## 46 条审查记录总表

本表的“运行复现”沿用原审查材料；本次整理文档没有重新执行全部探针。风险级别为原审查判断。涉及网络输入的触发前提、实际影响及修复范围需要在处理对应条目时继续核验。

| 编号 | 级别 | 问题摘要 | 证据与当前处理状态 |
| --- | --- | --- | --- |
| RTPS-1 | 严重 | AckNack 位图长度未限制，可能越界写栈 | 原审查 ASan 复现；未独立处理 |
| RTPS-2 | 严重 | Heartbeat 按报文声明长度复制到固定结构体 | 原审查 ASan 复现；未独立处理 |
| RTPS-3 | 高 | 大端位图转换按字节数索引 `uint32_t` 数组 | 原审查 ASan 复现；未独立处理 |
| RTPS-4 | 高 | 子报文长度未与剩余报文长度校验，可能下溢 | 静态待复现；未独立处理 |
| RTPS-5 | 低 | AckNack 的 32 位 count 使用 16 位字节序转换 | 静态待复现；未独立处理 |
| RTPS-6 | 高 | Gap 位图 size 漏转换字节序，并缺少上限校验 | 静态待复现；未独立处理 |
| RTPS-7 | 高 | Data / DataFrag 载荷长度减法可能下溢 | 静态待复现；未独立处理 |
| CDR-1 | 高 | 跨字节序浮点编解码使用数值转换而非位重解释 | 原审查运行复现；独立工作树已有候选改动，完整修复验收待完成，尚无 PR |
| CDR-2 | 中 | 字符串反序列化按报文长度直接扩容 | 静态待复现；未独立处理 |
| CDR-3 | 中 | 宽字符串反序列化按报文长度直接扩容 | 静态待复现；未独立处理 |
| XTYPES-1 | 中 | DynamicType 引用计数归零未释放 | 原审查 Valgrind 复现；未独立处理 |
| XTYPES-3 | 中 | DynamicData 赋值只增加类型引用计数 | 静态待复现；未独立处理 |
| XTYPES-4 | 低 | `DynamicData::equals()` 恒返回 false | 静态待复现；未独立处理 |
| DISC-1 | 严重 | 发现参数列表按不可信长度解析，可能越界读 | 原审查 ASan 复现；未独立处理 |
| DISC-2 | 高 | 参数列表循环推进 offset 时缺少边界校验 | 静态待复现；与 DISC-1 相关，未独立处理 |
| DISC-3 | 高 | ORIGINAL_WRITER_INFO 内层参数循环缺少边界校验 | 静态待复现；未独立处理 |
| XML-1 | 中 | QoS 解析失败的错误码被 bool 解释为成功 | 原审查列为静态记录；开发期已修正，见开发计划，尚未拆独立 PR |
| XML-2 | 低 | XML 数值字段缺少范围、格式检查 | 静态待复现；部分新配置已有严格解析，原条目涉及的全部字段仍需逐项核验 |
| SR-1 | 高 | StatelessReader 匹配表并发访问及空条目解引用 | 历史开发修复与回归记录；尚未拆独立 PR |
| LOGGER-1 | 高 | 日志器并发启停崩溃、状态与掩码竞争 | **修复完成；普通回归及 TSan 前后对照；PR !115 待合并** |
| SW-1 | 高 | SPDP 首次发送、重发竞争同一发送缓冲 | 历史 TSan 报告及开发修复；尚未拆独立 PR |
| TCP-1 | 高 | TCP 会话表同步、空 socket、持锁取消问题 | 历史开发修复；空 socket 场景有前后验证，复合条目需按子问题拆分；尚未独立 PR |
| SHM-1 | 中 | 共享环形缓冲中的非对齐结构体访问 | 历史 UBSan 证据及开发修复；尚未拆独立 PR |
| RECV-1 | 中 | 接收 socket 自身线程销毁时 join / 事件循环生命周期异常 | 历史原版 SIGABRT、修复后通过；尚未拆独立 PR |
| STATUS-1 | 高 | 持状态锁调用用户回调，重入可能死锁 | 静态待复现；不能用其他回调锁修复替代本项证明 |
| SUB-1 | 高 | Subscriber 的 readerHandles 并发访问缺少锁 | 静态待复现；未独立处理 |
| DR-1 | 高 | DataReader 监听器更新与回调访问缺少同步 | 静态待复现；未独立处理 |
| HC-1 | 高 | HistoryCache 实例历史部分访问未持统一锁 | 静态待复现；开发分支改过 history，需逐路径复核原条目 |
| HC-2 | 中 | 按实例删除历史未通知等待写端 | 静态待复现；未独立处理 |
| HC-3 | 中 | WriterWaitFor 解锁期间迭代器可能失效，且访问前未判空 | 静态待复现；未独立处理 |
| IQ-1 | 严重 | Inline QoS 解析缺少边界校验 | 原审查 ASan 复现；未独立处理 |
| IQ-2 | 高 | KEY_HASH 固定复制 16 字节，未校验参数实际长度 | 静态待复现；未独立处理 |
| IQ-3 | 中 | related_sample_identity 的 32 位字段使用 16 位转换 | 静态待复现；未独立处理 |
| DF-1 | 严重 | DataFrag 的 fragmentSize 未判零就参与除法 | 静态待复现；原记录描述 SIGFPE 风险，本表不计为运行复现 |
| DF-2 | 高 | 分片数量未限制，可能消耗过多内存 / CPU | 静态待复现；未独立处理 |
| DF-3 | 高 | 分片表为空时解引用 begin 迭代器 | 静态待复现；未独立处理 |
| DF-4 | 低 | 分片确认序号截断到 int32，负值未检查 | 静态待复现；未独立处理 |
| HB-1 | 严重 | Heartbeat 序号区间未限制，可能批量创建过多状态 | 静态待复现；未独立处理 |
| GAP-1 | 严重 | Gap 序号区间未限制，可能耗尽内存 | 静态待复现；不同于开发期迟加入 GAP 的功能错误 |
| GAP-2 | 高 | Gap 有效性检查使用 `&&`，未拒绝超大位图 | 静态待复现；未独立处理 |
| GAP-3 | 中 | 补齐缺失序号状态时存在大量循环与重复查找 | 静态待复现；未独立处理 |
| INFO-1 | 低 | InfoTimestamp / InfoSource 未校验子报文长度 | 静态待复现；未独立处理 |
| SHM-2 | 中 | 共享写位置在多写者下可能丢更新 | 静态待复现；需要确认实际多写者调用约束 |
| SHM-3 | 高 | 共享读端信任 dataSize，可能越界与计数下溢 | 静态待复现；未独立处理 |
| LIVE-1 | 中 | 存活消息读取前的长度检查不足 | 静态待复现；未独立处理 |
| SPDP-1 | 中 | 发现参与者裸指针表的并发删除 / 查询风险 | 静态待复现；需核对调用链同步，未独立处理 |

## 开发过程中发现的其他问题

下表补充历史开发记录中的问题，不计入上述 46 条，也不直接把它们与 46 相加作为独立根因总数。这里以便于交付的主题分组，后续应按具体测试和源码拆分。

| 跟踪编号 | 问题摘要 | 当前已知状态 / 证据 |
| --- | --- | --- |
| DEV-WAITSET | WaitSet 残留通知导致条件已不成立仍被当作触发 | 开发分支已修，已有确定性回归；[开发计划](DATASHARING-PLAN.md)记录了旧通知测试 |
| DEV-READCOND | 查询 ReadCondition 时消耗变化标记，影响后续读取条件判断 | 开发分支已修并补回归；尚未拆独立 PR |
| DEV-TESTEXIT | 测试主函数忽略 `RUN_ALL_TESTS()` 返回值，失败仍退出 0 | 开发分支已修；需要独立提交时核对各测试入口 |
| DEV-REMOTEWRITER | AddLocalReader 无锁遍历 remoteWriters，发现线程删除时崩溃 | 已捕获 `std::_Rb_tree_increment` 崩溃栈，开发分支补锁，后续回归通过；[开发计划](DATASHARING-PLAN.md)有记录 |
| DEV-METADATA | 匹配回调发生时元数据尚未保存，查询返回 BAD_PARAMETER | 原版双向回调查询失败，开发分支已修；[前后验证](../artifacts/debug-native/metadata-proof.json) |
| DEV-LATEGAP | 迟加入 Reader 的缺失序号未补齐，扩展 GAP 时覆盖已收状态 | 原版 5 项回归中 2 项失败，开发分支修复后全通过；[证据](../artifacts/debug-native/late-gap-proof.json)。不等同于 GAP-1 的不可信区间问题 |
| DEV-DEADLINE | 旧 deadline 回调可重新启用已非 ALIVE 的实例 | 开发分支已有前后回归；[证据](../artifacts/debug-native/deadline-lifecycle-proof.json)，完整交付范围需另行核定 |
| DEV-WAITSET-UNKNOWN | 历史完整核心测试在 WaitSet 用例发生一次无栈 SIGSEGV | **根因未确认，不能标已修复。**后续通过不等于解释了该次崩溃；[开发计划](DATASHARING-PLAN.md)明确保留此限制 |

DataSharing 新功能自身在开发期间出现的过期样本、错误 take、单槽预算等问题，记录在其实现与验收文档中；未将它们直接包装成现成的上游原版 bug。

## 官方两个遗留 Issue 的核查

| 官方 Issue | 当前判断 | 依据 / 下一步 |
| --- | --- | --- |
| [IC7GUM：日志接口只完成屏显](https://gitee.com/agiros/travodds/issues/IC7GUM) | **按“缺少文件输出”的描述，功能已实现；Issue 尚未关闭** | 上游 2025-08-26 已加入文件日志；本次直接验证原版生成 `.ddslog` 并含预期内容。[证据](../artifacts/issues-check-20261029/upstream-file-log/verification.json)。这不是 PR !115 新增的功能 |
| [IC7GX2：hostid 算法更新](https://gitee.com/agiros/travodds/issues/IC7GX2) | **原改进目标未落实** | 当前仍抽取 16 位随机 host_id，旧实现已标注 TODO，后续迁移不构成修复；没有实现 Issue 建议的主机地址四字节哈希。[核查](../artifacts/issues-check-20261029/assessment.md)。未复现 GUID 碰撞或通信失败 |

## 后续更新规则

每处理一项，记录原版复现、修复提交、针对性验证、PR 链接和上游合并状态。提交说明和 PR 正文使用中文，段落内部不按固定宽度硬换行；通过完整 UTF-8 文件提交并回读核对。

优先从已经有开发期证据的独立修复中选择下一项。CDR-1 的候选修改仍保留在 `.worktrees/cdr-endian-bits`，不因本表整理而恢复或提交它。

## 原始材料

- [2026-10-28 原审查报告](CODE-REVIEW-20261028.md)
- [原审查机器记录：46 条](../artifacts/review-20261028/findings/findings.json)
- [DataSharing 开发计划与问题记录](DATASHARING-PLAN.md)
- [DataSharing 完成验收记录](DATASHARING-COMPLETION-20261028.md)
- [日志器 PR 交付材料](../artifacts/pr-logger-20261029/README.md)
- [官方两个 Issue 核查](../artifacts/issues-check-20261029/assessment.md)

本文仅为本地工作文稿，未加入日志器 PR，未额外发布到 Gitee 或 GitHub。
