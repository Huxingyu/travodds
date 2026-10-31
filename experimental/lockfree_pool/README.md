# 单 Writer / 固定 Reader 共享池无锁提交原型

这是独立实验，尚未接入 TravoDDS 的生产 `SharedPayloadPool`、DDS API、发现、QoS 或 RTPS。它研究的范围是固定容量共享段内的槽位提交、借用与复用；不能据此宣称整个 DDS write 无锁或完成全部官方验收。当前实验布局版本为 2，包含 Producer-only 轮转 cursor；不与首轮未发布的版本 1 布局或生产 Layout v5 互通。

## 已实现的协议

每个槽位的一个对齐 64 位原子 `control` 同时包含：高 32 位 generation；低 15 位 pending Reader；再 15 位已借出 Reader Loan；一位 Writer/history pin；一位 WRITING。编译期要求目标平台的 32/64 位 GCC 原子始终 lock-free。共享布局仅有固定整数/数组，不保存进程地址、容器或 owning C++ 对象。

| 操作 | 原子状态转换 / 线性化点 |
|---|---|
| reserve | 仅当所有 pin/WRITING 位为零，CAS `{gen,0}` 到 `{gen+1,WRITING+Writer}`；成功 CAS 取得上代所有归还的 release sequence，再允许覆盖载荷。 |
| commit | 写完载荷、size 和原子 sequence，CAS WRITING 到 Writer+目标 Reader pending；最后 release-store 全段 published_watermark，作为最终发布门控与线性化点。 |
| take | 先 acquire 水位，只考虑不超过该快照的 sequence，选最早的 pending 槽；单次 control CAS 将自己的 pending 位换成 loaned 位，再访问非原子 size/payload。 |
| release Writer/history | 同一 control CAS 校验 generation 与 Writer 位后清除 Writer 位；WRITING 的取消必须调用 abort。 |
| return Loan | 同一 control CAS 校验 generation 和对应 Reader 的 loaned 位后清除；不出现 pending 清掉而 loaned 尚未设置的零引用窗口。 |
| discard pending | 对应 Reader 在自己的消费通道里用带 generation 的 control CAS 丢弃未读引用。 |
| abort | 串行 Writer 将自己的 WRITING+Writer 原子替换为零 pin；不能与该 reservation 的 commit 并发。 |

session 为初始化后不变的 128 位身份；所有 token 同时校验 session、槽号和 generation。对不同代次的 CAS 整体比较使陈旧归还无法在“读 generation 后发生复用”的窗口清除新样本引用。generation 达到 UINT32_MAX 后，该槽用完即退休，绝不回绕。

Reader 的扫描不能只“看到一个已提交的槽就返回”：并发 Writer 可能刚好在 Reader 扫过早槽后提交它，又在稍后槽提交后续样本，导致先返回后续 sequence。全段水位快照将本轮可见集合限制为同一个已完成前缀；已经在该前缀中且仍为该 Reader pending 的槽不会被复用，因此扫描可以按 sequence 选择最早样本。`take/discard_pending` 对同一 Reader id 必须严格串行，这条前提保证扫描到 claim 期间自身 pending 持续固定该槽；其他 Reader 和 Writer pin 归还仍可并发。

所有 control CAS 使用 acq_rel，失败读取使用 acquire。这同时连接“发布载荷→Reader 读”和“所有 Reader 读完→最后归还→下一次 reserve→覆写”的 happens-before，不能把 pin 归还放宽成缺少释放语义的普通写。

## 通知与进展保证

`commit()` 内没有 mutex、动态分配、/proc 探测、系统调用，也不等待 Reader；它只写自己的槽位与必要原子状态，再增加 changed。`reserve()` 扫描固定槽表，存在空槽就可申请；找不到时返回 Exhausted。单个慢 Reader 可以固定它涉及的槽位，但不会持有阻止其他槽位提交的全段锁。容量最终被慢 Reader 全部占满时，明确背压是安全语义，不应伪装成“所有 write 永远成功”。

`MappedPool` 的 Linux futex 等待/唤醒是单独一层。正确等待顺序为：先采样 changed，再测试队列谓词，再以旧代次 futex 比较后睡眠；EAGAIN/EINTR/超时都重新检查谓词，最多每 100 ms 重查一次，以免极端代次 ABA 或发布后未调用 wake 导致永久沉睡。`wake_all()` 含内核 syscall，不对其内核内部锁、调度或最坏延迟声明形式化 lock-free。core 返回/提交不依赖等待层。

`MappedPool::Loan` 是进程本地的 move-only 所有者，保持 mapping 的 shared_ptr，所以销毁 facade 或关闭 Writer 不会撤销已经借用的映射。共享状态本身没有 shared_ptr。core 的原始 `Sample` 则只是内部 pin 令牌；直接使用 core 的调用方必须保持映射、恰好一次归还，不可对裸 Sample 复制出多个独立的同一 Reader 归还者。

## 必须保留的限制

- 核心使用最多 15 个固定 Reader 位；核心本身没有注册 generation、注销握手、GUID/session 发现或历史重放。targets 只选择已知 Reader 位。下方可选控制面演示这些位的动态租用与关闭，但仍未接入 DDS 发现；未注册 Reader 的未来回放不在核心协议内。
- 一个串行 Producer 负责 reserve/commit/abort/close；每个 Reader id 的 take/discard 严格串行。已经发布的 Writer pin 和已有 Loan 可由其他线程归还，但调用方要先以本地共享所有权保证最后一个 owner 才清对应共享 pin。
- 没有死 Reader/Writer 的身份检测或 pin 回收。进程死亡可能永久固定容量，需要新 session；不会把“不再运行”当成可立即覆盖旧 Loan 的依据。Writer 死在 control 已提交但 watermark 未推进的阶段，该槽可能永久固定且不向 Reader 暴露。
- close 只禁止新 reserve/commit，并允许排空已发布 pending；它不自动退还 Writer/history pin、未完成 reservation 或 Reader pin。已有 Loan 可以照常归还。关闭后的清理策略仍属于尚未集成的控制路径。
- generation 只有 32 位。若一个槽每秒复用一百万次，约 71.6 分钟就会退休；每秒十万次约 11.9 小时。Producer-only 轮转 cursor 分散热点并保留跨 facade 复制的轮转，但只是延缓某槽耗尽，不解决长期运行寿命。任何正式采用都需重新设计位宽/Reader 数量/代次或其他有证明的 epoch 协议。
- 共享段初始化要先通过外部握手发布映射；只支持当前 Linux ABI/编译目标。映射建立、进程创建、页故障、OS 调度、调用方内存分配、futex 和完整 DDS/history 流程的进展不由本原子协议保证。它也不是恶意共享进程隔离边界。
- 本轮不提供与 Fast DDS 的性能优劣结论；功能与故障实验不能代替同条件基准。

## 文件与验证入口

- `shared_pool.hpp`：无 OS 依赖的核心固定槽位协议（仍要求 GCC 兼容 always-lock-free 原子）。
- `mapped_pool.hpp`：Linux mapping 保活、move-only Loan、futex 等待/唤醒。
- `test_pool.cpp`：14 个具名检查，包括完整 pin 条件、Writer history、陈旧 token、generation/sequence 耗尽、半提交不可见、水位、映射寿命、changed 普通回绕、睡前发布、等待、子进程暂停及死亡、3 Reader 并发校验。
- `exec_demo.cpp`：由主线程编写的独立 exec/重新映射验证入口。
- `controller.hpp` / `test_controller.cpp`：可选动态注册控制面与 8 项检查，详见下文。

## 可选动态控制面（独立实验）

新增 `controller.hpp` 在不改动上述核心的前提下维护一个独立的共享 registry 映射。每个 Reader 位的 gate 是 `{registration generation 32bit, phase 2bit, inflight count 30bit}` 原子整体；phase 为 FREE、OPEN、CLOSING。Reader 不直接注册/关闭，向单 Writer 提交 `OpenRequest/CloseRequest` 值，由 `apply_open/apply_close` 应用；异步请求 mailbox、传输、DDS 发现适配尚未实现。使用者必须建立控制请求与响应的进程间通道。

Reader 每次 take/discard 先对匹配 session+注册 generation 的 OPEN gate CAS 加 inflight，再进入核心操作，完成后减 inflight；归还现有 Loan 允许匹配的 OPEN 或 CLOSING gate。epoch 检查与进入操作是同一次 CAS，不能改成“先检查身份，再另外增加计数”。显式的 begin/finish API 仅将这个操作租约窗口暴露出来，便于测试暂停情形；所有映射必须在租约和 Samples 生命周期内保持映射。

Writer close 先原子改为 CLOSING 并立即从未来 targets 中排除；有 inflight 操作就返回 Pending，不同步等待。inflight 为零后，Writer 通过核心提供的带代次 CAS 清理该 Reader 的 pending，绝不清除 loaned。注册位上全部旧 pending/loaned 都归零且 inflight=0 后，最终 gate CAS 才将其变成 FREE；下次注册增加 generation。旧操作尚未进入核心却被暂停、或者陈旧归还检查后暂停时，该 lease 都阻止位复用，避免旧操作误消费新身份或误清新身份引用。注册 generation 耗尽同样退休。

已有 take 若在 close 前成功取得 lease，可以在 close 后完成一次旧样本借用；关闭会等其 lease 退出后清理剩余未读引用。已有 Loan 可以在 CLOSING 期间归还，并继续固定自己的载荷和注册身份。return-only lease 与 pending 清理可能并行，但不创建 pending；最终 exact gate CAS 会拒绝在 return 尚未退出时复用该身份。

**使用硬约束：**registry 只能在新会话初始化时建立，不能在同一 session 有活 Reader/lease 时重置。所有 reserve/commit/abort、apply 请求和 poll 必须由同一个串行 Writer lane 执行；所有发布必须通过 Controller 筛选 OPEN mask，不能混用直接向任意位写 targets 的 core 发布。Reader 同一身份的 take/discard 仍须串行，所有 take/return/discard 必须通过注册租约，不得混用绕过 registry 的裸核心访问。原始 Controller::Sample 是需明确归还的内部令牌，并非 DDS Loan 或自动持有双映射的 RAII 对象；当前映射保活由调用方保证。

关闭后卡住或死亡的 inflight 参与者会使该注册位长期 CLOSING，其他身份仍可用剩余槽位继续提交；本层没有死亡检测、强制清票或自动端点重建。不能为了回收该位而擅自清除 inflight，否则会重新引入旧操作恢复后触及新身份的竞态。

`replay=true` 明确返回 Unsupported：冻结核心没有为已有 Writer-held 样本原子增加新注册 pending 的入口；为实现重放而直接改 control 会绕过已验证协议。将来若增加 replay，必须同时重新证明 registration generation、操作 lease、同一样本 generation 下陈旧归还与 pin 添加的交互。

`test_controller.cpp` 提供 8 项额外检查：动态注册关闭与未读清理、关闭保留 Loan、旧身份拒绝、暂停 take 租约跨关闭、延迟归还防 epoch ABA、子进程死亡只阻止本身份复用、epoch/count 耗尽、replay 拒绝与不同 session；真实线程用例执行 128 轮关闭和重注册，在独立 Reader 持 Loan 时向另一身份累计发布 2,048 条样本，每次发布后握手复查旧 Loan 的完整 64 字节内容（加上每轮起始样本，共 2,176 次提交）。实际通过结果由主线程统一执行记录，不能以核心测试结果替代本层测试。

可由 CMake 独立构建，不依赖 TravoDDS 全量构建或 GoogleTest。该目录的测试/源码数量不计入既有生产验收数字；实际本轮运行结果由主线程记录，不能把旧的 DataSharing 测试结果当作本原型已通过。

```sh
cmake -S experimental/lockfree_pool -B /tmp/travodds-lockfree-build -DCMAKE_BUILD_TYPE=Release
cmake --build /tmp/travodds-lockfree-build --parallel 1
ctest --test-dir /tmp/travodds-lockfree-build --output-on-failure
```
