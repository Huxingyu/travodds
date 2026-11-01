# VM ↔ openlet-eval4090 RTPS 回退实测

受测实现：`f32da8390b0af07500f67eb1d0a1279a9c968f4f`。控制组通过；跨主机 24/24 通过，零重跑。这里只作功能结论。

在同一受测实现 `f32da8390b0af07500f67eb1d0a1279a9c968f4f` 上，VM（Linux 6.8）与 `openlet-eval4090` 的 Kubernetes 容器（另一主机的 Linux 5.15）完成双向实测，双方 boot_id 不同、均为 x86_64。两端使用同字节独立收发程序和私有库，库 SHA-256 为 `7ed3c24c732b236a4a89ec0aa44dfae0778487f3a5795e1dfbab1f57fc44dda6`。VM 的 TAP 独立网络命名空间与远端 `unshare -Urn` 内 veth/AF_PACKET 经 SSH stdin/stdout 传送以太网帧；这是 **SSH 承载的虚拟二层，不是物理局域网**。RTPS 仍使用 UDP，SPDP 仍通过 `239.255.0.1` 发现，未修改产品代码或添加静态对端；收发器显式移除传统 SHM transport，以隔离 RTPS/UDP 路径，DataSharing 仍设 AUTO/ON。先行单播及组播双向 echo 均通过；程序 peer IP 仅用于应用控制握手。

同机 AUTO/native 控制组为 64 条：Writer 序列化 0、Reader 反序列化 0、Reader Loan 64、完整验证 64。跨主机矩阵为两端同设 AUTO/ON × ordinary/Writer Loan × 64/4096/65536 B × 两种 Writer 方向，**24/24 组通过**，每组正式 64 条：序列化 64、反序列化 64、Reader Loan 0、完整载荷验证 64；共 1536 条跨主机样本。控制组及跨主机共 50 个进程均自然退出 0，没有组重跑或失败后重写。每组另有一条不计入正式计数的握手样本；所有 loan/write 均单次检查返回码，native 控制先只读观察精确 Reader GUID 注册就绪。独立审核逐一解析 50 份原始日志并重核矩阵、计数、boot_id、SHA 和退出码。

远端只用 nice 19、1 核，未安装软件或访问 GPU，未改 OpenLET 文件/配置、未向其 PID 发送信号。已记录 OpenLET 前后 PID：后测仍观察到原控制器，随后记录到新评测控制器/worker，不能声称所有旧 PID 全程不变，也不以进程记录证明评测成绩相同。SSH/bridge、VM 命名空间和 TAP、远端临时目录均已清理，两端 `/dev/shm` 无新增残留。完整证据见 [跨主机报告](REPORT.md)、[逐组结果](results.json)、[独立日志审核](results-independent-review.json)和[清理](cleanup.json)。本轮不作性能结论；未覆盖物理局域网、跨主机 OFF/mixed 组合、其他发行版或所有故障场景。生产代码、测试和性能文档未改，本次文档只在本地单独提交、不推送。


| 组 | Writer 所在 | kind | 写法 | 字节 | 序列化 | 反序列化 | Reader Loan | 校验 | 两端退出 |
|---|---|---|---|---:|---:|---:|---:|---:|---|
| 01 | VM | auto | ordinary | 64 | 64 | 64 | 0 | 64 | 0 / 0 |
| 02 | VM | auto | ordinary | 4096 | 64 | 64 | 0 | 64 | 0 / 0 |
| 03 | VM | auto | ordinary | 65536 | 64 | 64 | 0 | 64 | 0 / 0 |
| 04 | VM | auto | loan | 64 | 64 | 64 | 0 | 64 | 0 / 0 |
| 05 | VM | auto | loan | 4096 | 64 | 64 | 0 | 64 | 0 / 0 |
| 06 | VM | auto | loan | 65536 | 64 | 64 | 0 | 64 | 0 / 0 |
| 07 | VM | on | ordinary | 64 | 64 | 64 | 0 | 64 | 0 / 0 |
| 08 | VM | on | ordinary | 4096 | 64 | 64 | 0 | 64 | 0 / 0 |
| 09 | VM | on | ordinary | 65536 | 64 | 64 | 0 | 64 | 0 / 0 |
| 10 | VM | on | loan | 64 | 64 | 64 | 0 | 64 | 0 / 0 |
| 11 | VM | on | loan | 4096 | 64 | 64 | 0 | 64 | 0 / 0 |
| 12 | VM | on | loan | 65536 | 64 | 64 | 0 | 64 | 0 / 0 |
| 13 | eval4090 | auto | ordinary | 64 | 64 | 64 | 0 | 64 | 0 / 0 |
| 14 | eval4090 | auto | ordinary | 4096 | 64 | 64 | 0 | 64 | 0 / 0 |
| 15 | eval4090 | auto | ordinary | 65536 | 64 | 64 | 0 | 64 | 0 / 0 |
| 16 | eval4090 | auto | loan | 64 | 64 | 64 | 0 | 64 | 0 / 0 |
| 17 | eval4090 | auto | loan | 4096 | 64 | 64 | 0 | 64 | 0 / 0 |
| 18 | eval4090 | auto | loan | 65536 | 64 | 64 | 0 | 64 | 0 / 0 |
| 19 | eval4090 | on | ordinary | 64 | 64 | 64 | 0 | 64 | 0 / 0 |
| 20 | eval4090 | on | ordinary | 4096 | 64 | 64 | 0 | 64 | 0 / 0 |
| 21 | eval4090 | on | ordinary | 65536 | 64 | 64 | 0 | 64 | 0 / 0 |
| 22 | eval4090 | on | loan | 64 | 64 | 64 | 0 | 64 | 0 / 0 |
| 23 | eval4090 | on | loan | 4096 | 64 | 64 | 0 | 64 | 0 / 0 |
| 24 | eval4090 | on | loan | 65536 | 64 | 64 | 0 | 64 | 0 / 0 |

本机/远端程序 SHA、ELF/私库解析见 [runtime-provenance.json](runtime-provenance.json)；驱动源码/编译见 [driver-build.json](driver-build.json)、[driver-audit.json](driver-audit.json)；网络权限、校验和 offload 关闭及双向 echo 见 [network-proof.json](network-proof.json)。原始日志以逐组结果中的 log 字段定位。

执行记录：2026-11-01T12:31:33.775888+00:00 至 2026-11-01T12:35:37.438621+00:00。任务从 12:26:39 UTC 起按 20 分钟上限安排，清理及提交完成时间以 delivery.json 为准。未使用 Modal、Mac 或 Muse；没有新建或续租计费资源，既有评测机仍按原租赁计费，本任务不虚报其增量账单为零。
