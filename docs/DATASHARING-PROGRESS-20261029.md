# TravoDDS DataSharing 推进记录：无锁核心与动态控制面

> 历史快照：以下保留 10 月 29 日实验阶段的原始范围和证据，不能当作当前生产状态。后续同步协议见 [生产后端历史报告](DATASHARING-PRODUCTION-20261030.md)，当前共享布局 v6 / discovery v3 的强制 key-only 契约与新验证见 [严格准入报告](DATASHARING-STRICT-20261030.md)。

日期：2026-10-29。实际工作树：`/home/ubuntu/project/travodds/.worktrees/datasharing-progress`；分支 `work/datasharing-progress`；基点 `3da8e077b9902142cdb4549eafa5b43114bdf936`。本文记录的是该基点上的未提交改动，不以其他工作树的实现代替本轮结果。

## 达到的程度

已经从“通知计数使用原子操作，但发布事务仍持 metadata mutex”，推进到一个**有实际跨进程与并发验证的独立无锁提交原型**，并实现了动态 Reader 身份租用、关闭和代际复用的可选控制层。生产 `SharedPayloadPool` 和 DDS 写入链路保留原实现；本轮没有将实验协议接入生产后端，也没有提交、推送或新建 PR。

这两个判断应同时保留：实验核心的 `commit()` 不取得用户态锁，不分配内存、不探测 `/proc`、不调用 syscall；现有完整 DDS `write()` 仍包含 Writer/history 等锁，不能据实验结果宣称它已无锁。

官方原文使用“DataSharingNotification（无锁通知）”和“实现跨进程通知机制”，没有精确定义整个 publish/write 的无锁验收范围。本轮探索更强的提交目标，不代替导师确认验收口径。此前完整核对见 `/tmp/travodds-discussion-review-result-20261029.md`。

## 本轮实现

| 部分 | 实现及其用途 | 明确的边界 |
|---|---|---|
| [共享核心](../experimental/lockfree_pool/shared_pool.hpp) | 每槽以单个 64 位 CAS 同时校验 generation、pending、loaned、Writer/history pin、WRITING。Reader 用一次 CAS 从 pending 转为 loaned，避免引用短暂归零导致覆写。 | 单个串行 Producer；同一 Reader 的 take/discard 串行；最多 15 Reader。 |
| 发布水位 | 先提交槽，再 release-store 全段 watermark；Reader 先采样水位，只取该快照内最早的 sequence。 | 解决扫描跨多次发布而倒序的问题；不是多 Writer 协议。 |
| [映射和等待层](../experimental/lockfree_pool/mapped_pool.hpp) | move-only Loan 保活数据映射；先采样 changed，再检查谓词，最后 futex 比较等待；保留总截止时间并周期重查。 | futex wake 含 syscall，其内核进展不属于核心无锁保证。 |
| [动态控制面](../experimental/lockfree_pool/controller.hpp) | 独立 registry 的单个 gate 打包注册 epoch、OPEN/CLOSING/FREE、inflight。关闭先排除未来发布，等操作租约退出后清 pending，保留已借出的 Loan；旧 pin 和租约全部退出后才允许身份复用。 | 请求只有值类型接口；没有异步控制通道或 DDS discovery。暂停/死亡的租约只阻止其身份复用，当前不强制清票。 |
| [生产边界测试](../test/unit_payloadpool/test_sharedpayloadpool.cpp) | 新增活 metadata 持锁者 SIGSTOP、通知计数回绕、等待线程收到信号后三项测试。 | 第一项确认现有阻塞边界，没有修改生产行为。 |
| [验证入口](../tools/ospp/notification_progress.py) | 串行编译、超时与内存限制、源码/库 SHA-256、逐命令日志，区分普通版、ASan/UBSan、TSan。 | 不把检查器通过推广为完整形式化证明。 |

动态关闭的关键是**旧操作退出之前不重用身份**。仅先检查 epoch、随后单独增加 inflight，会留下旧 take 跨注册复用的窗口。gate 的整体 CAS 将这两步合并。return-only 操作可在 CLOSING 期间进入；最终 FREE CAS 还会检查计数，不能在归还操作尚未退出时复用。

低层 Controller 的 Sample/Lease 仍依赖调用方同时保持数据与 registry 映射；它与 MappedPool 的 RAII Loan 是不同接口，尚未具备生产 DDS Loan 的双映射共享所有权封装。禁止绕过 Controller 混用裸 core 的发布或 Reader 操作。

## 验证证据

全部产物位于 `/home/ubuntu/project/travodds/artifacts/notification-progress-20261029/`。运行输入均保存 SHA-256，测试完成后再次校验，以避免用旧结果证明新源码。

| 当前证据 | 实际结果 | 范围 |
|---|---|---|
| `normal-controller-final/results.json` | 核心 14/14、独立 exec、控制面 8/8，分别重复 3 轮，全部通过。 | 本轮最终实验源码。 |
| `asan-controller-final/results.json` | 核心 14/14、exec、控制面 8/8，ASan+UBSan 通过。 | 明确 `detect_leaks=0`，未执行泄漏扫描。 |
| `tsan-controller-final/results.json` | 核心 14/14、exec、控制面 8/8，TSan 通过。 | 使用 `setarch x86_64 -R`；不包含生产库 TSan。 |
| `normal-baseline-verified/results.json` | 生产共享池 29/29 通过。 | 复用已核对冻结库，当前测试源码。 |
| `asan-baseline-final/results.json` | 生产共享池 29/29，ASan+UBSan 通过。 | 复用检查版冻结库，当前测试源码；未执行 LSan。 |
| `serial-cmake-final/results.json` | 全流程通过：实验 CTest 5/5；新构建生产库上共享池 29/29、payloadpool/Loan 集成回归 195/195，均无失败、错误或禁用。 | 当前工作树完整生产库及上述两个测试目标重新构建；不是整个仓库所有测试。 |

本机串行构建编译全部 96 个生产 `.cpp`，库构建耗时约 2 分 27 秒，峰值 RSS 449,212 KiB（约 439 MiB）；所有构建均 `--parallel 1`、`-O1 -g0`。新库 SHA-256 为 `d94e491a68aefbaf281c0795614496301afe3eee65aab6ce1e82b4ebf07ebb56`。该普通版重建与之前复用冻结库的 ASan/UBSan 验证是两份不同证据；本轮未完整重建 sanitizer 生产库。生产构建仍有既有编译警告，实验 C++ 目标则以 `-Wall -Wextra -Wpedantic -Werror` 编译通过。

核心 14 项包括多方 pin、Writer history、陈旧 token、代次/序号耗尽、半提交不可见、扫描水位、映射寿命、通知计数回绕与睡前发布、等待、暂停/死亡参与者、3 Reader 线程各 3,000 个样本。额外 8 项控制测试覆盖关闭保留 Loan、延迟 take/return、旧注册拒绝、epoch/count 耗尽、不同 session 和死亡租约。

独立 `exec_demo.cpp` 由 Reader 重新打开文件并映射到与 Writer 不同地址，不依赖 fork 继承地址。Reader0 SIGSTOP 并持有原 Loan 时，Writer 完成 10,000 次明确只面向 Reader1 的提交，随后恢复 Reader0 验证原字节不变。**这 10,000 条没有继续发送给暂停 Reader0**；持续面向慢 Reader 导致固定容量耗尽的情况另有明确返回 Exhausted 的测试，不能将两者混为无限发布能力。

真实线程控制面用例执行 128 轮借用、关闭、归还和重注册。每轮旧 Loan 被独立线程持有时，另一身份接收 16 条发布；每次发布后通过原子握手要求旧 Loan 持有者重新检查完整 64 字节。总计 2,048 条其他发布，加上 128 个初始样本为 2,176 次提交。测试验证注册 generation 连续增加，旧注册和旧归还不能影响新身份。

### 模型与独立复查

[独立协议审查](../experimental/lockfree_pool/PROTOCOL-REVIEW-20261029.md) 及 [源文件快照](../experimental/lockfree_pool/review-source-hashes.json) 已封存。审查覆盖最终 core、mapping、controller 与新增线程用例；没有发现阻断当前受限实验协议的缺陷，但明确拒绝直接替换生产池。

| 有限模型 | 结果 |
|---|---|
| generation 检查与清位分离 / 完整 CAS | 分离版本在 20 个状态内找到陈旧归还反例；CAS 版本 19 状态未发现同类反例。 |
| 一槽、两 Reader、两代生命周期 | 69 个状态、134 条转换，安全断言通过。 |
| 无水位 / 有水位扫描 | 各 35 种交错；无水位 2 个倒序反例，有水位 0 个。 |
| 无租约 / 有租约的旧 take | 19 / 15 状态；无租约存在跨注册消费，有租约未发现。 |
| 假设未来支持同一样本 replay 的延迟 return | 35 / 30 状态；租约门控排除了模型中的跨注册 ABA。当前实现拒绝 replay，反例不是当前支持路径的已复现 bug。 |

模型采用顺序一致性和有限状态抽象，没有证明 C++ 全部弱内存行为、任意进程故障或内核性质。TSan 对跨进程共享内存的可见性也有限，因此保留真实进程实验、内存序论证与模型，不能只看检查器是否报错。

### 运行条件与失败记录

按用户要求在本机低优先级、单任务构建，编译进程地址空间上限 768 MiB，未修改内核设置。此前远端连接不可用；后续本机完整构建使用 `serial_build.py` 和新的产物目录，不修改已有冻结库。

最初生产测试读取 `/proc/<pid>/syscall` 遇到权限错误；改用实际可见的 futex wchan，并结合单线程子进程的阶段握手判断等待状态。没有把这次观察工具失败记作产品 bug。ASan 测试复用的 GTest archive 带 gcov，首次链接缺符号，补 `-lgcov` 并重定向计数后通过。控制面在统一编译前核对发现 const facade 调用非 const core 的接口问题，现已通过局部复制仅包含 State 指针的 facade 修正并补审。

新的最小 LSan 探针也在泄漏扫描启动时 fatal，因此当前环境不能提供泄漏结论；ASan/UBSan 使用 `detect_leaks=0` 的结果必须附带这一限制。原始失败日志全部保留。早期 `normal-final/results.json` 总体失败来自后续冻结源码字节校验，不能称整条流程通过；后续记录已处理且重跑。冻结源的唯一差异是 `payloadbuffer.cpp` 精确少一个 EOF LF，292 个生产输入中其余一致，脚本只允许这一字节差异，不接受任意源码变化。

## 距离生产接入的最小路径

1. **先解决 ABI 与代次寿命。**生产 Layout v5 使用运行时槽数/载荷大小、最多 64 Reader 与 64 位 generation；本实验为模板固定布局、最多 15 Reader、32 位 generation。代次达到上限后退休保障安全，但热点槽每秒复用百万次约 71.6 分钟可耗尽；轮转只分散热点，不解决长期运行。需先确定可协商的新 layout 与更长寿命的协议，不能原位偷换。
2. **接实际控制通道及 QoS。**把 GUID 目标选择、注册请求/应答、关闭及进程身份统一交给真实的串行 Writer lane。当前 replay 明确返回 Unsupported；TRANSIENT_LOCAL 历史和迟到 Reader 的语义尚未实现。最小接入宜显式 opt-in、VOLATILE 小能力集，保留当前后端，先定义不支持能力时 AUTO/ON 的行为。
3. **接双映射 Loan、history 与故障会话生命周期。**生产 CacheChange/nativePayload/Writer history 会延迟释放 pin，也会按需 RTPS 序列化；需要自动保活两份映射的 Loan owner、批次校验和关闭规则。卡住/死亡的 inflight 当前不强制清除，Writer 死在 slot commit 与 watermark 间可能固定容量；安全恢复需要设计新 session、旧 Loan 保活及端点重建边界。
4. 在上述接口稳定后，再做真实 DDS 混合本地/远端、发现/QoS/RTPS 故障矩阵、长期运行、不同平台与同条件性能测试。当前没有 Fast DDS 性能优劣结论，也没有声称其他 DDS 框架一概不存在无锁设计。

本轮交付点是可复现、经过检查器与独立审查的协议原型和生产边界实验。要使新协议成为正式 DDS 后端，还需上述兼容性与生命周期工作；本轮结果不能直接记成“整个 DDS write 无锁”或“官方全部目标验收完成”。

## 重现入口

实验模块独立构建，无需构建整个 DDS：

```sh
cmake -S experimental/lockfree_pool -B /tmp/travodds-lockfree-build -DCMAKE_BUILD_TYPE=Release
cmake --build /tmp/travodds-lockfree-build --parallel 1
ctest --test-dir /tmp/travodds-lockfree-build --output-on-failure -j 1
```

带输入快照、限制与日志的定向运行：

```sh
python3 tools/ospp/notification_progress.py \
  --baseline-root /home/ubuntu/project/travodds \
  --output /tmp/travodds-notification-normal-new \
  --mode normal --phase prototype --repeat 3
```

检查版分别选择 `--mode asan --leak-check off` 和 `--mode tsan`，使用不同的新输出目录；仅在泄漏扫描环境限制仍成立时关闭 LSan。完整单线程 CMake 构建的精确入口保存于产物根目录 `serial_build.py`，该脚本拒绝覆盖已有结果；重现应先为它选择新的 OUTPUT。
