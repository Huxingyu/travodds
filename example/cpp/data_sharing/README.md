# 有限运行的 DataSharing 示例

三个示例仅使用公开 DDS API，运行在 Linux 同一主机上。每次启动一个发布进程和一个独立 exec 订阅进程，完成固定条数后自行退出。无需 GPU、外部服务、账号或密钥。

| 可执行文件 | 演示内容 | 每轮真实数据 |
|---|---|---|
| `datasharing_loan` | Writer Loan 与普通 write；Reader take/return_loan；删除实体后借用保活 | 6 条，双方载荷序列化/反序列化为 0 |
| `datasharing_keyed` | 两个键的注册、查询、dispose、恢复、unregister、旧句柄拒绝与重新注册 | 5 条 ALIVE、2 条生命周期控制；载荷序列化/反序列化为 0 |
| `datasharing_mixed` | 同一 Writer 对接 ON Reader 和后加入的 OFF Reader；共享与 UDP RTPS 同时通信 | 每个 Reader 6 条；Writer 序列化 6 次，OFF Reader 反序列化 6 次 |

## 编译与运行

在仓库根目录执行，`install` 是事先编译安装的 TravoDDS 私有前缀：

```bash
cmake -S example/cpp/data_sharing -B artifacts/local-loan/build-data_sharing \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="$PWD/artifacts/local-loan/install"
cmake --build artifacts/local-loan/build-data_sharing --parallel 1

artifacts/local-loan/build-data_sharing/datasharing_loan
artifacts/local-loan/build-data_sharing/datasharing_keyed
artifacts/local-loan/build-data_sharing/datasharing_mixed
```

每个进程输出一条 `DS_RESULT` JSON。成功时两个进程均返回 0；Writer 会等待并回收订阅进程。keyed 示例故意使用注销后的旧句柄，库可能输出一条预期的 `handle invalid` 错误日志，示例要求该操作返回 BAD_PARAMETER。

可用仓库提供的检查器验证进程退出、条数、序列化计数、删除后借用及共享目录清理：

```bash
python3 tools/ospp/datasharing_examples.py \
  --build artifacts/local-loan/build-data_sharing \
  --output artifacts/local-loan/results/finite-examples --rounds 1
```

输出目录必须尚不存在，避免覆盖旧结果。`--rounds 30` 顺序运行 30 轮；检查器将超时强制终止判为失败，保存每轮 stdout、stderr 和结果。

## API 流程与配置

`sample.h/.cpp` 提供 64 字节、64 字节对齐、无指针的固定布局样本，显式声明版本化布局 ID。布局、大小、对齐和字段含义在所有进程中必须一致；不能把含指针、string、vector 的普通对象直接放进共享段。带键类型单独实现键哈希。键操作和 RTPS 控制消息仍存在，零调用计数仅针对完整载荷 CDR。

`demo.cpp` 创建 Domain 146、包含启动 PID 的唯一 Topic，并通过 QoS 向两个进程传入同一个私有 `/dev/shm/travodds-demo-*` 目录。Writer 使用 ON、16 槽、4 Reader 上限、1 MiB 段预算、可靠传输和 history depth 4；Reader 也限制 1 MiB 映射预算。实际共享段小于该预算。示例禁用旧 SHM transport，发现和普通载荷回退使用 UDP。

发现匹配成功不代表共享映射已注册。示例先禁止载荷序列化/反序列化，用一条预备 Loan 验证实际共享借用；该条不计入演示数据和统计。混合示例随后创建 OFF Reader，等待两个端点匹配，再开启 CDR 并重置计数。Writer 通过公开 `loan_sample` → 填充 → `write(loan)` 交出可写所有权；失败时 Loan 仍归调用方，成功后句柄变空。

Reader 使用 StatusCondition 和 WaitSet 等待，再 `take` 一条。空序列请求原生借用；OFF Reader 返回自身拥有的 CDR 解码数据。Reader 校验内容和顺序后 `return_loan`，并特意保留第一条借用直至 Writer 与 Reader 都删除。该借用仍可读，最后由序列 `unloan()` 释放。业务代码通常应在删除 Reader 前归还借用。

两个进程通过本地控制 socket 明确每个演示步骤，避免测试依赖任意 sleep。每个阶段有超时。Writer 删除时共享文件应 unlink；已存在的映射和借用继续保活，释放后不留下共享段。父进程最后检查目录为空并删除目录，失败清理也仅尝试 rmdir，不递归删除未知文件。

三个场景的具体调用分别在 `loan.cpp`、`keyed.cpp`、`mixed.cpp`。本示例验证固定条数和资源生命周期，不作为吞吐量或延迟基准。
