# 有限运行的 DataSharing 示例

三个示例仅使用公开 DDS API，运行在 Linux 同一主机上。每次启动一个发布进程和一个独立 exec 订阅进程，完成固定条数后自行退出。无需 GPU、外部服务、账号或密钥。当前库对应共享布局 v6、发现能力 v3，并对带键 native 强制 key-only 准入；必须重新编译示例并链接同一份当前库。当前严格版本三个示例各 3 轮，共 9 次执行通过；同一源码的库重点回归在独立完整插桩 ASan/UBSan 与 TSan 下各 57 项通过，范围限于 focused 用例，LSan 未执行。最终汇总审计、结果及私有库哈希见 [严格准入报告](../../../docs/DATASHARING-STRICT-20261030.md)，官方对应关系见 [验收矩阵](../../../docs/DATASHARING-ACCEPTANCE.md)。同日 discovery v2 的示例结果继续作为历史单独保留。

| 可执行文件 | 演示内容 | 每轮预期检查 |
|---|---|---|
| `datasharing_loan` | Writer Loan 与普通 write；Reader take/return_loan；删除实体后借用保活 | 6 条，双方载荷序列化/反序列化为 0 |
| `datasharing_keyed` | 两个键的注册、查询、dispose、恢复、unregister、旧句柄拒绝与重新注册 | 5 条 ALIVE、2 条生命周期控制；载荷序列化/反序列化为 0 |
| `datasharing_mixed` | 同一 Writer 对接 ON Reader 和后加入的 OFF Reader；共享与 UDP RTPS 同时通信 | 每个 Reader 6 条；Writer 序列化 6 次，OFF Reader 反序列化 6 次 |

## 编译与运行

在仓库根目录执行，先将 DS_INSTALL 改为当前生产库实际安装的私有前缀，DS_EXAMPLES 指向本轮示例构建目录。库、应用及自定义 TypeSupport 子类必须一起重编，不能只替换旧动态库：

```bash
DS_INSTALL=/absolute/private-build/install
DS_EXAMPLES=/absolute/private-build/examples
cmake -S example/cpp/data_sharing -B "$DS_EXAMPLES" \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$DS_INSTALL"
cmake --build "$DS_EXAMPLES" --parallel 1

"$DS_EXAMPLES/datasharing_loan"
"$DS_EXAMPLES/datasharing_keyed"
"$DS_EXAMPLES/datasharing_mixed"
```

每个进程输出一条 `DS_RESULT` JSON。成功时两个进程均返回 0；Writer 会等待并回收订阅进程。keyed 示例故意使用注销后的旧句柄，库可能输出一条预期的 `handle invalid` 错误日志，示例要求该操作返回 BAD_PARAMETER。

可用仓库提供的检查器验证进程退出、条数、序列化计数、删除后借用及共享目录清理：

```bash
python3 tools/ospp/datasharing_examples.py \
  --build "$DS_EXAMPLES" \
  --output /absolute/new-finite-examples-evidence --rounds 1
```

输出目录必须尚不存在，避免覆盖旧结果。`--rounds 30` 顺序运行 30 轮；检查器将超时强制终止判为失败，保存每轮 stdout、stderr 和结果。

## API 流程与配置

`sample.h/.cpp` 提供 64 字节、64 字节对齐、无指针的固定布局样本，显式声明版本化布局 ID。布局、大小、对齐和字段含义在所有进程中必须一致；不能把含指针、string、vector 的普通对象直接放进共享段。带键类型单独实现键哈希，并显式实现 get_native_key_info/capture_native_key/restore_native_key：只保存 8 字节键，不复制 64 字节样本。键查询只修改输出的键，非键字段保持原值。示例 Writer 和 Reader 的 TypeSupport 均实现这些强制钩子。两端分别校验有效 NativeKeyInfo，只有 discovery v3 同契约端点才能进入 DS；键表示仍是本地元数据，具体快照字节无需通过发现传输。缺失/非法元数据时 ON enable 返回 UNSUPPORTED，AUTO 不广告 DS 且保留普通 CDR/RTPS，loan_sample 返回 UNSUPPORTED。捕获/还原失败直接报错，不会复制完整样本兜底。键和 RTPS 控制元数据仍有复制，零调用计数仅针对完整载荷 CDR，不代表没有任何字节复制。

`demo.cpp` 创建 Domain 146、包含启动 PID 的唯一 Topic，并通过 QoS 向两个进程传入同一个私有 `/dev/shm/travodds-demo-*` 目录。Writer 使用 ON、16 槽、4 Reader 上限、1 MiB 段预算、可靠传输和 history depth 4；Reader 也限制 1 MiB 映射预算。实际共享段小于该预算。示例禁用旧 SHM transport，发现和普通载荷回退使用 UDP。

发现匹配成功不代表共享映射已注册。示例先禁止载荷序列化/反序列化，用一条预备 Loan 验证实际共享借用；该条不计入演示数据和统计。混合示例随后创建 OFF Reader，等待两个端点匹配，再开启 CDR 并重置计数。Writer 通过公开 `loan_sample` → 填充 → `write(loan)` 交出可写所有权；失败时 Loan 仍归调用方，成功后句柄变空。严格零载荷复制指该 Writer Loan → Reader Loan 分支；loan 示例中的普通裸指针 write 仍复制一次样本。

Reader 使用 StatusCondition 和 WaitSet 等待，再 `take` 一条。空序列请求原生借用；OFF Reader 返回自身拥有的 CDR 解码数据。Reader 校验内容和顺序后 `return_loan`，并特意保留第一条借用直至 Writer 与 Reader 都删除。该借用仍可读，最后由序列 `unloan()` 释放。业务代码通常应在删除 Reader 前归还借用。

两个进程通过本地控制 socket 明确每个演示步骤，避免测试依赖任意 sleep。每个阶段有超时。Writer 删除时共享文件应 unlink；已存在的映射和借用继续保活，释放后不留下共享段。父进程最后检查目录为空并删除目录，失败清理也仅尝试 rmdir，不递归删除未知文件。

三个场景的具体调用分别在 `loan.cpp`、`keyed.cpp`、`mixed.cpp`。本示例验证固定条数和资源生命周期，不作为吞吐量或延迟基准。
