# TravoDDS 上游原版代码审查（2026-10-28）

状态：进行中（第一批已复现）。只审查 Gitee 上游 `489067108405b27afbd369245542f5e88d457f18` 的原版代码，跳过本轮 DataSharing/Loan 改动；文件即使被本轮修改过，也按上游原版判断。

范围：`src/` + `include/` 共 240 个 C/C++ 文件、75,389 行；第三方依赖不做完整人工审计。
上游快照：`artifacts/review-20261028/upstream-source/`。
证据目录：`artifacts/review-20261028/findings/`（含探针源码、二进制、ASan/valgrind 日志）。
机器可读结果：`findings/findings.json`。

原则：本审查不改生产源码、不提交、不推送、不发起 PR；每条结论必须带可复现证据。

## 一、已复现问题（第一批）

| ID | 级别 | 问题 | 位置 | 证据 |
|---|---|---|---|---|
| RTPS-1 | 严重 | AckNack 位图 `numBits` 无上限校验，远程栈溢出 | `messageprocessor.cpp:1064-1067` | `rtps-acknack-asan.log` |
| RTPS-2 | 严重 | Heartbeat 用线上 `subMessageLength` 作 memcpy 长度，远程栈溢出 | `messageprocessor.cpp:937` | `rtps-heartbeat-asan.log` |
| RTPS-3 | 高 | 大端位图转换循环用字节数索引 uint32 数组，合法报文也越界 | `messageprocessor.cpp:1069-1073`（NackFrag/Gap 同构） | `rtps-acknack-be-asan.log` |
| RTPS-4 | 高 | 子报文长度与报文总长无校验，长度下溢致越界读/死循环 | `messageprocessor.cpp:832-833` | 代码审查 |
| CDR-1 | 高 | 跨字节序 float/double 用数值转换代替位重解释 | `travoddscdr.h:172-190, 485-499` | `cdr-endian.log` |
| XTYPES-1 | 中 | `DynamicType` 引用计数归零不释放，`delete_type` 泄漏 | `dynamictype.cpp:108-112`、`dynamictypebuilderfactoryimpl.cpp:168-171` | `dyn-leak-valgrind.log` |
| CDR-2 | 中 | 反序列化字符串按线上长度直接 resize，可致内存耗尽 | `travoddscdr.cpp:483-492` | 代码审查 |
| RTPS-6 | 高 | Gap 位图 `size` 漏做字节序转换，且同样无上限校验 | `messageprocessor.cpp:1422-1432` | 代码审查 |
| RTPS-7 | 高 | Data/DataFrag 载荷长度减法可下溢为极大值 | `messageprocessor.cpp:899, 1161` | 代码审查 |
| DISC-1 | 严重 | 发现协议 `ParameterList` 按线上长度解析，堆越界读 | `parameter.cpp:621-1179` | `param-parse-asan.log` |
| DISC-2 | 高 | `LoadMessage` 参数循环 `offset += len` 无边界校验 | `parameter.cpp:1176` | 代码审查 |
| XML-1 | 中 | QoS 解析失败返回 `RETCODE_ERROR(=1)` 被当成 true | `dds_xml_repo.cpp:350,393` | 代码审查 |
| XML-2 | 低 | XML 数值字段无范围/格式校验 | `dds_xml_repo.cpp` 多处 | 代码审查 |
| SR-1 | 高 | `StatelessReader::matchWriters_` 无锁且 `operator[]` 未校验 | `statelessreader.cpp:145,325,400+` | 验收期已修复+回归 |
| LOGGER-1 | 高 | `AsyncLogger` 并发首次启动竞争，可 `std::terminate` | `logger.cpp:243-260` | `logger-before.xml/log`、`logger-race-diagnostic.log` |
| SW-1 | 高 | `StatelessWriter` 发送缓冲区被并发读写（SPDP 重发） | `statelesswriter.cpp:660` | `tsan-attempt1/logs/test-qos.log` |
| TCP-1 | 高 | `MessageTransmitterTCP` 会话表无锁、空 socket 解引用、持锁 cancel | `messagetransmittertcp.cpp:31-60,248-275` | 验收期已修复 |
| SHM-1 | 中 | `SharedMemSocket` 环形缓冲非对齐 `reinterpret_cast` | `sharedmemsocket.cpp:280-295` | 验收期已修复 |
| RECV-1 | 中 | `ReciverSocket` 自 join / `io_context` 生命周期 | `recivesocket.h:20-45` | 验收期已修复 |
| STATUS-1 | 高 | 状态回调持锁调用用户监听器，重入自死锁（所有 status） | `statusmanager.cpp:30-57,280-305` | 代码审查（非递归 `mtx_`） |
| SUB-1 | 高 | `SubscriberImpl::readerHandles_` 无锁并发访问 | `subscriberimpl.cpp:127,168,178,221` | 代码审查 |
| DR-1 | 高 | `set_listener` 无同步（源码自带 TODO），回调读到半更新状态 | `datareaderimpl.cpp:221-233,894-910` | 代码审查 |
| HC-1 | 高 | `HistoryCache::instanceChanges_` 部分访问不加 `changeMutex_` | `historycache.cpp:222-254,406-433` | 代码审查 |
| HC-2 | 中 | 按实例删历史后不 `notify_all`，阻塞写端空等超时 | `historycache.cpp:445-472` | 代码审查 |
| HC-3 | 中 | `WriterWaitFor` 解锁窗口内持有失效迭代器 + `[0]` 未判空 | `historycache.cpp:482-505` | 代码审查 |
| IQ-1 | 严重 | Inline QoS 解析无边界校验，单包堆越界读写（匹配前即解析） | `messageprocessor.cpp:1234-1281` | `inline-qos-asan.log` |
| IQ-2 | 高 | `PID_KEY_HASH` 固定拷 16 字节，不校验参数长度 | `reader.cpp:235-241` | 代码审查 |
| IQ-3 | 中 | `related_sample_identity` 32 位字段用 `ntohs` | `messageprocessor.cpp:1256` | 代码审查 |
| DF-1 | 严重 | `fragmentSize=0` 未校验 → 接收线程整数除零 SIGFPE | `statefulreader.cpp:211` | 代码审查 |
| DF-2 | 高 | 分片数无上限（`sampleSize` 线上可控，且先减 4）→ OOM/CPU 耗尽 | `statefulreader.cpp:207-219` | 代码审查 |
| DF-3 | 高 | `AddFragData` 分片表为空仍解引用 `begin()` | `writerproxy.cpp:370-392` | 代码审查 |
| DF-4 | 低 | `GetAckSequenceSet` 截断为 int32，可能 `SetBit(负数)` | `writerproxy.cpp:408-423` | 代码审查 |
| HB-1 | 严重 | Heartbeat `firstSN..lastSN` 无上限，单包插入数十亿条状态 | `writerproxy.cpp:143-157` | 代码审查 |
| GAP-1 | 严重 | Gap `gapStart..bitmapBase` 区间无上限，单包 OOM | `statefulreader.cpp:396` | 代码审查 |
| GAP-2 | 高 | Gap 校验用 `&&`，`bitmap.size>256` 未被拒绝 → `IsBitSet` 越界读 | `statefulreader.cpp:373,400` | 代码审查 |
| GAP-3 | 中 | `NonAvailableChangeSet` 填满区间 + 内层 `std::count`，O(n·m) | `writerproxy.cpp:49-86` | 代码审查 |
| INFO-1 | 低 | InfoTimestamp/InfoSource 不校验 `subMessageLength` | `messageprocessor.cpp:1187-1215` | 代码审查 |
| DISC-3 | 高 | `PID_ORIGINAL_WRITER_INFO` 内层循环无边界，`len-24` 下溢 | `parameter.cpp:1002-1027` | 代码审查 |
| XTYPES-3 | 中 | `DynamicData::operator=` 只增不减类型引用计数 → `DynamicType` 泄漏 | `dynamicdata.cpp:36-46` | 代码审查 |
| XTYPES-4 | 低 | `DynamicData::equals` 恒返回 false | `dynamicdata.cpp:295-301` | 代码审查 |
| SHM-2 | 中 | 共享内存 `writepos` 非原子，多写者丢更新 | `sharedmemsocket.cpp:230-261` | 代码审查 |
| SHM-3 | 高 | 读端完全信任对端 `dataSize` → 越界读 + 计数下溢死循环 | `sharedmemsocket.cpp:279-291` | 代码审查 |
| LIVE-1 | 中 | 存活消息先读 16 字节后判长度，`buffer[19]` 守卫写成 `>0` | `livelinessmanager.cpp:492-512` | 代码审查 |
| SPDP-1 | 中 | `Spdp::discParts_` 裸指针无锁，删除与查询并发 UAF | `spdp.cpp:370-380` | 代码审查 |
| CDR-3 | 中 | `deserializeWString` 按线上长度 resize | `travoddscdr.cpp:498-511` | 代码审查 |
| RTPS-5 | 低 | AckNack 32 位 `count` 用 `ntohs` 转换 | `messageprocessor.cpp:1058` | 代码审查 |

### 关键复现

RTPS 解析越界（ASan，单包 UDP 可达，SECURITY=OFF 无需认证）：

```
$ ./findings/rtps_parse_overflow acknack
acknack numBits=4096 -> GetByteSize=512, bits[] capacity=32 bytes
ERROR: AddressSanitizer: stack-buffer-overflow ... WRITE of size 512
  #1 TRAVODDS::MessageProcessor::InterpretAckNackMessage
exit=1
```

```
$ ./findings/rtps_parse_overflow heartbeat
heartbeat subMessageLength=65535 -> memcpy length=65539 into 64-byte struct
ERROR: AddressSanitizer: stack-buffer-overflow ... WRITE of size 65539
  #1 TRAVODDS::MessageProcessor::InterpretHeartbeatMessage
exit=1
```

大端合法位图也会越界：

```
$ ./findings/rtps_parse_overflow acknack-be
acknack-be numBits=256 -> GetByteSize=32, conversion loop runs i<uilen=32 over bits[8]
ERROR: AddressSanitizer: stack-buffer-overflow ... READ of size 4
exit=1
```

CDR 跨字节序（本机小端，目标大端）：

```
ser float 1.0f BE  (got):      00 3f 00 47
ser float 1.0f BE (expected):  3f 80 00 00
ser double 1.0 BE (got):       00 00 00 00 e0 07 ee 40
deser float BE 3f800000 -> 0 (expected 1)
ser float 1.0f LE (control):   00 00 80 3f
```

DynamicType 泄漏：

```
$ valgrind --leak-check=full ./findings/dyn_leak
224 (224 direct, 4 indirect) bytes in 1 blocks are definitely lost
   by TRAVODDS::DynamicTypeBuilder::build() const (dynamictypebuilder.cpp:109)
```

### 构建与运行方式

上游原版库（普通构建，已存在）：`artifacts/review-20261028/upstream-build/src/libtravoddsd.so`
ASan 探针只单独编译 `messageprocessor.cpp` 一个翻译单元，其余仍链接原版库：

```bash
S=upstream-source
g++ -std=c++17 -O0 -g0 -fsanitize=address -fno-omit-frame-pointer \
    -D_DDS_DLL_FILE -D_USRDLL -I$S/include -I$S/src -I$S/thirdparty \
    -I$S/thirdparty/asio-1.13.0/include -I$S/thirdparty/tao \
    -c $S/src/rtps/messages/messageprocessor.cpp -o findings/messageprocessor_asan.o
g++ -std=c++17 -O0 -g -fsanitize=address -I$S/include -I$S/src -I$S/thirdparty \
    findings/rtps_parse_overflow.cpp findings/messageprocessor_asan.o \
    -L upstream-build/src -Wl,-rpath,$PWD/upstream-build/src -ltravoddsd \
    -o findings/rtps_parse_overflow
```

## 二、影响判断

- RTPS-1/2/3/4 同属一个根因：**收到的子报文长度字段被无条件信任**。`InterpretSubmessage` 只检查一次 `length >= sizeof(MessageHeader_t)`，之后既不复核 `subMessageLength`，也不复核位图/列表长度。任意局域网主机（或跨网可达的 UDP 端口）发送一个畸形 RTPS 包即可造成栈破坏，属于远程可利用的内存安全问题。
- CDR-1 不是内存安全，而是**互操作性缺陷**：与大端实现（部分嵌入式/RTOS、PowerPC 等）交换 float/double 会得到错误数值，且当前测试只覆盖同端序，未发现。
- XTYPES-1/CDR-2 属于**资源管理缺陷**：长时间运行且反复创建/删除动态类型会持续增长；畸形 CDR 可被用来放大内存占用。

## 二点五、第二批（发现协议与并发/传输）

第二批里 DISC-1 用 ASan 复现（`param-parse-asan.log`）：SPDP 是组播协议，局域网内任意主机发一个畸形参数列表即可触发堆越界读，并可借字符串长度字段放大内存。

LOGGER-1、SW-1、SR-1、TCP-1、SHM-1、RECV-1 是本仓库验收期间已经定位并修复的上游缺陷，证据保留在 `artifacts/completion-20261028/`（旧库失败用例、TSan 报告、修复前后 diff）。它们不是本轮新引入的问题，而是上游原版代码本身的缺陷。

## 二点六、第三批（DCPS 状态/监听器与历史缓存）

**STATUS-1 是本批最严重的一条**：`Update*Status` 在 `lock_guard(mtx_)` 内直接调用应用回调，而 `Get*Status` 取同一把非递归 `std::mutex`。用户在回调里查状态（DDS 标准用法）即自死锁；在回调里 `delete_datareader`/`delete_datawriter` 也会因内部取状态锁而死锁。影响 liveliness_lost、*_matched、deadline_missed、incompatible_qos、inconsistent_topic 全部状态。

`HC-1` 的并发模型问题：写线程走 `Writer::AddNewCachce → AddKeyHistory`（无锁），确认线程走 `ReceiveAckNackMessage → RemoveAllAckedData → RemoveChange`（持 `changeMutex_`），两者同时改 `instanceChanges_`，属确定性数据竞争。

## 二点七、第四批（Inline QoS 与 DataFrag）

**IQ-1 是本次审查最容易被利用的一条**：`InterpretInlineQosMessage` 在 `InterpretDataMessage` 里于**匹配判断之前**执行，参数长度直接来自报文，`std::vector::assign` 按该长度从报文体拷贝，既不校验子报文剩余长度，也不要求哨兵存在。ASan 复现见 `findings/inline-qos-asan.log`（`InterpretInlineQosMessage → std::copy`，heap-buffer-overflow READ，exit=1）。攻击者只需要能发 UDP，不必先完成发现与匹配。

**DF-1 是一条单包远程崩溃**：`fragmentSize` 是 16 位线上字段，解析时只做 `htons`，接收端直接拿它做除数。与本地 reader 匹配的 writer 发一包 `fragmentSize=0` 的 DATA_FRAG，接收进程立即 SIGFPE。

## 二点八、第五批（Heartbeat / Gap 序号算术）

**HB-1 与 GAP-1 是同一类问题：把线上的 64 位序号直接当作循环上界**。`RecvHeartbeat` 用 `for (insertSeq_ = firstSN; insertSeq_ <= lastSN; insertSeq_++)` 往 `changesFromWriters_` 逐条插状态，`ReceiveGapMessage` 用 `for (seq = gapStart; seq < bitmapBase; seq++)` 往 vector 里 push。两者都只做了"大于 0 / 不小于 firstSN-1"的弱校验，没有区间上限，也没有"与已提交序号的距离"约束。一个包即可让接收进程 OOM 或长时间 100% CPU。

GAP-2 是校验逻辑写错：`if (gapStart <= 0 && (size == 0 || size > 256))` 应该是 `||`（或拆成两个独立判断）。`gapStart > 0` 时 `size` 可到 2^32，随后 `IsBitSet(i)` 直接越界读 8 个 `uint32` 的位图数组。

## 二点九、第六批（发现协议/共享内存/存活/XTypes 收尾）

- `SHM-3`：共享内存读端把对端写入的 `dataSize` 当作可信长度，`startpos` 可以越过段尾继续 `reinterpret_cast`，`usedBytes` 还会下溢成接近 2^32 导致读循环永不结束。共享内存只做同机进程间传输，属本地信任边界问题。
- `SPDP-1`：`discParts_` 存裸指针且无锁，租约过期删除与安全插件查询并发时是确定的 UAF。
- `LIVE-1`：`buffer_size > 0` 的守卫与后面读 `buffer[19]` 不匹配，短载荷会把垃圾当 livelinessKind。
- `XTYPES-3`：`DynamicData::operator=` 只加不减引用计数，`DynamicType` 只增不释放。

### 已排查但排除的候选（避免误报）

- `BufferNode` 的 `calloc(size + sizeof(NodeInfo) - 1)` 看似少分配，实际数据偏移为 `offsetof(NodeInfo,data)`，留有 3 字节余量；`mempool_bounds` 探针在 ASan 下无报告。
- `LivelinessManager::SendLivelinessMessage` 的 `memcpy(..., sizeof(data))` 看似越界，但 `sizeof(InstanceHandle_t)` 为 24，`buffer_size` 恰为 28，`liveliness_bounds` 探针无报告（仅有 8 字节未初始化数据被发送）。
- `DynamicData` 析构看似泄漏 `complex_value`，但 `TypeValue` 自带析构函数会释放，非泄漏。

## 三、下一步（按优先级）

1. RTPS 长度校验统一收口：给所有 `Interpret*` 传入剩余长度并逐项校验（位图按位上限、列表按条数上限、字符串/序列按剩余字节）。
2. 位图结构改为动态或定长+显式上限（256 位），字节序转换按 `uilen/4` 迭代。
3. CDR 端序转换改为 `memcpy` 位重解释，补大端互操作回归。
4. XTypes 生命周期：引用计数归零 `delete`，或工厂统一持有并明确释放语义。
5. 继续按模块人工审查：`historycache`、`statefulreader/statefulwriter`、`transport/shm`、`security`、`xml`、`discovery`。

## 四、边界

- 本批结论均针对上游原版；本轮 DataSharing/Loan 实现不在范围内。
- RTPS 探针用 ASan 单独编译 `messageprocessor.cpp`，其余链接原版库；未对全库做 ASan 构建。
- 未做模糊测试、未做真实双机跨端序联调；RTPS-4 与 CDR-2 目前是代码路径证据，尚未构造独立崩溃用例。
