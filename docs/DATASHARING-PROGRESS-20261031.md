# DataSharing 缺口补全与验收报告（2026-10-31）

本轮从 checkpoint `570b779` 补齐验收缺口，共享布局 **v6**、发现能力 **v4**。321 项输入已冻结，正式 coverage-release2 已通过 **529/529 回归、9 次有限示例及 93.936% 新增生产可执行行覆盖率（2835/3018，142 个翻译单元）**，独立 **ASan/UBSan 与 TSan 各 86/86 项 focused 通过**。最终汇总审计通过，当前主证据包和历史补充包已生成并分别完成搬迁复核；八项交付在本页限定范围内完成，边界见 [验收矩阵](DATASHARING-ACCEPTANCE.md)。

## 本轮必须补齐的行为

| 缺口 | 本轮契约与实现方向 | 当前验收状态 |
|---|---|---|
| 第 1 项：Reader 普通载荷池 | 为 Reader 增加本地 payload_pool 配置，在普通 RTPS history 准入边界将完整 CDR 分片链事务性转入 Topic 池；配置相同的同 domain/topic/type Reader/Writer 复用池，native SharedReadLoan 直接保留 | **19 项 Reader 回归在正式模块 245 项、ASan/UBSan 及 TSan 中通过**，包含真实 RTPS/分片、池共享/隔离/冲突、原池归还、耗尽回滚、MultiTopic 与删除后缓存寿命 |
| 第 5 项：借出时不初始化完整载荷 | TypeSupport 增加保守默认 false 的 supports_native_uninitialized_sample()；NativeTypeSupport 仅对 trivially default constructible 类型默认允许，以 placement new T 开始对象寿命，不使用 T{} 清零 | **10 项无初始化回归在正式模块、ASan/UBSan 及 TSan 中通过**，包括逐字节哨兵、槽位复用、类型门控及真实 DDS Loan |
| 旧 native 对端隔离 | discovery v4 表示同时满足无初始化及 key-only 门控；v1/v2/v3 不选择 native，保留普通 RTPS；共享布局仍为 v6 | 正式模块中的旧协议隔离与当前门控回归通过；旧 v3 结果不用于证明新契约 |
| 可移交证据 | 小型原始 JSON/XML/log、冻结源码哈希及相对索引可独立打包；大库/二进制/对象缓存留在原构建目录，重建时重新生成 | [当前主包](../../../artifacts/portable-evidence-20261031/native-v4-final-review/README.md) 与 [历史补充包](../../../artifacts/portable-evidence-20261031/native-v4-history-review/README.md) 已生成；前者严格匹配当前源码，后者保留四次失败与两次诊断 |

Reader Topic 池预算仅约束转入 history 的普通 CDR 载荷分片，不把 transport 接收整包、独立 CDR key snapshot 或 native 共享槽位混入同一预算。原始接收缓冲继续归其原池；任何准入分配失败必须归还已分配分片，不留下半条 history 样本。具体 QoS、预算与不可变性见 [普通池手册](PAYLOADPOOL-QOS.md)。

Reader 的 KEEP_LAST 配置至少需要完整 history 加一条入库暂存载荷；enable 拒绝 max_payloads 不大于 history.depth 的明确不足配置。这只是每样本一片时的必要条件，大样本分片、同池其他 Writer/Reader 的 history 及外部缓存持有者都需要额外预算。预算不足返回 APPLY_MEM_ERR 并保留旧样本。长度为零的控制描述符不分配普通载荷，ALIVE 空描述符仍拒绝；分片样本的重复 read 与后续 take 回归已在本轮正式模块中通过。

无初始化原生 Loan 的应用必须在 publish 前填写所有可能被 Reader、键钩子或 RTPS 回退序列化器读取的字段。借出不保证零初值，也不保证旧槽位已被擦除。不能平凡默认构造的类型仍可编译和使用 CDR/RTPS；默认不具备 native 无初始化资格，ON 拒绝，AUTO 保留普通路径。显式自定义 TypeSupport 可以声明符合契约的固定类型，声明者负责对象寿命与字段布局。

仓库当前没有将任意 IDL 自动适配到上述 native 契约的生成器。示例的布局声明、无初始化能力及 key-only 钩子是手写 TypeSupport。HelloWorld 的无界 string 不能作为原生共享样本，其 CDR/RTPS 回退是正确行为；“支持 bounded/plain”不等于已自动支持任意 IDL 类型或有界 C++ 容器。

共享存储是**固定槽位池的轮转扫描**：每次从轮转起点扫描可复用槽位，必须先确认 Writer、pending 和 Reader Loan pin 均已释放。它不是按 FIFO 头尾推进的环形队列，不按顺序覆盖最旧样本，也不会覆盖慢 Reader 仍借用的槽位。

## 当前证据与最终审计

coverage-release2/results.json 已为 passed=true，全部 14 个命令退出 0：共享池 37 + payloadpool/Loan 245 + 核心 247 = **529/529 通过**，失败/错误/禁用/跳过均为 0；loan/keyed/mixed 各 3 轮，共 **9 次示例执行通过**。相对固定基线 `489067108405b27afbd369245542f5e88d457f18` 的新增生产可执行行覆盖率为 **2835/3018 = 93.936%**，142 个翻译单元，变更生产翻译单元无漏采集。结果完成于 `2026-10-31T05:56:30.721745Z`。

asan-final/results.json 已为 passed=true，11 个命令均退出 0，独立生产库与测试的 **ASan/UBSan 37 + 16 + 10 + 19 + 4 = 86/86 项 focused 通过**；分别为共享池、key/admission/ThreadPool、无初始化、Reader 普通池，以及 1 项真实 DDS 持有控制锁时的进展和 3 项键/失败回归。失败/错误/禁用/跳过均为 0，结果完成于 `2026-10-31T06:05:25.171427Z`。LSan 明确未启用（ASAN_OPTIONS=detect_leaks=0），不作无泄漏结论；记录保留既有 ptrace 环境限制说明，本轮没有重新执行泄漏检查。

tsan-final/results.json 已为 passed=true，使用独立 build-tsan 重新构建，11 个命令均退出 0，相同 **86/86 项 focused 通过**，无 race 报告；失败/错误/禁用/跳过均为 0，结果完成于 `2026-10-31T06:10:00.062875Z`。构建将 GoogleTest 枚举延迟至 PRE_TEST；本轮直接运行测试 binary，TSan 实际测试以 setarch x86_64 -R 执行，设置 TSAN_OPTIONS=halt_on_error=1:exitcode=66。两个检查版均绑定当前冻结源码；focused 不代表完整 DDS 套件，也不证明所有线程交错无竞争。

[最终审计原始记录](../../../artifacts/portable-evidence-20261031/native-v4-final-review/records/04-current-audit-metadata/final-verification.json) 于 `2026-10-31T06:10:27.599189Z` 记录 passed=true：三模式 321 项源码哈希相同，每模式 68 个安装头匹配，16 个程序链接各自私有生产库，生产库/GTest/测试插桩与编译链接证据符合各自模式。审计同时核对 137 项预正式覆盖率计数归档的成员名称/大小/哈希、资源及缓存清理记录、9 次示例、覆盖率算术和变更翻译单元；临时 swap 已移除，遗留共享文件与当前验收程序均为 0。完整审计依赖原构建库、程序、覆盖率缓存与宿主状态；小包保留其原始结果及脚本，便携校验不会冒充重新执行完整审计。

| 已通过的当前产物（相对本轮证据根） | SHA-256 |
|---|---|
| coverage-release2/results.json | `e16ffb1e7db6efbf5cb0d2dafddb408d273bf4511ebf24254ebbae7d5c3e8dc2` |
| coverage-release2/coverage/summary.json | `24ae3f3c63b5c1d3e2f2193a75a788f9ab92aecc575404b24bfc01364a5b7dd7` |
| coverage-release2/examples/summary.json | `d7dd66a98c065900fc86ea4880446415e68213ae36d4e9837d61b13bfac7f48e` |
| build-coverage/install/lib/libtravodds.so.1.1.0 | `47b087dadba7364b99d47f54526cef552e0c321c1f20786777457fff7d1008ac` |
| frozen-source-multitopic.json | `b75ae262003fac62040259efd90eb6d7f35e79524ddc6d40338329f9df3d9fd9` |
| asan-final/results.json | `33502908b9f71378cca94ac7748a8f89fce851ec63d6667cc112ac917998a36d` |
| build-asan/install/lib/libtravodds.so.1.1.0 | `d06d475e47656d3fc0abe30ea045f283ccc74946b9a5c6a9a3269f70b0c25b3a` |
| tsan-final/results.json | `b105b1c7b6e0f1942861ba1baa0cff4370d503d5630168118d1cd88ae2639722` |
| build-tsan/install/lib/libtravodds.so.1.1.0 | `7215c77feca0e89006c3cf1318e389e7f629378a533baf741c7a86fdb6d92b24` |
| final-verification.json | `37adf9c386506c8f2831e616d2cef23c5bdcd36723e3fc301945042f6f8ee09e` |
| final-audit.py | `db2671f1d1450cf4571570d09ec3eaa473f36c16bb9c6ce46f1f671ad1d34632` |

用户明确授权在当前 VM 单线程慢速推进。当前运行采用 nice 19、ionice best-effort 7（-c2 -n7）、编译子进程地址空间 768 MiB；512 MiB/idle 的原计划保留在 resource-plan.json，调整另记 resource-adjustment.json，避免 idle I/O 在共享机上长期饥饿；当前 GCC 使用 ggc-min-expand=15、ggc-min-heapsize=16384，并将单个构建阶段超时设为 7200 秒。曾临时增加 256 MiB swap，创建记录为 temporary-swap.json；在无编译的 core 阶段确认可用 RAM 801 MiB、临时 swap 仅用 1.9 MiB 后，已提前安全 swapoff 并删除，temporary-swap-cleanup.json 记录退出 0、removed=true、passed=true，原 /swap.img 保持。提前释放磁盘供后续构建，最终审计仍独立核验文件及 /proc/swaps；进程 time -v 的 Swaps:0 不代表宿主没有换页。

为腾出磁盘，旧 openEuler 环境归档到 .worktree-archive/openeuler-rootfs-20261031，tar compare 退出 0；这不是 openEuler 平台验收。编译阶段完成后仅按 *-cache.json 记录删除可重建对象与哈希相同的构建库副本，安装库、程序、GTest archives、gcno/gcda 及日志保留。

本轮证据根为 artifacts/noinit-reader-20261031。三次失败均保留：coverage-final 在库构建阶段超过原 1800 秒超时，退出 124；coverage-verified 库构建/安装成功，但 test_loan.cpp 测试编译触及 512 MiB 虚拟地址空间限制，退出 2；coverage-final768 中 test_loan.cpp 已编译成功，新增 Reader 测试第 479 行使用未声明的 GetEndianess，测试编译再次退出 2。这三轮均未运行正式测试，不能计为通过。

第三轮后先只修正 test_reader_payloadpool.cpp 的两处测试问题，当时生产源码没有修改。diagnostic-reader-fix 成功重编，定向 20 项运行有 1 项失败，完整模块 244 项为 243 通过、1 失败，均指向 FragmentedSamplesCanBeReadRepeatedlyBeforeTake。该用例重复 read 后使用 take_next_sample，后者只选 NOT_READ，不能取回已标为 READ 的样本；修正改用 take 的 ANY_SAMPLE_STATE 并断言 READ 状态及完整载荷。diagnostic-reader-contract 中最终新增 28 项定向回归全部完成通过；它只证明诊断范围，不替代正式模块、核心、示例、覆盖率或 sanitizer。

正式重跑前已将 111 个诊断 gcda 归档至 diagnostic-reader-contract/pre-final-counters.tar.gz 并清空，避免诊断命中累加进最终覆盖率；记录为同目录 coverage-reset.json，归档 SHA-256 为 `64678ffd888f8dc28bb93377f619e1cf84c74fe57a37d5fd7b5781d3513b3eb0`。当时 321 项输入清单保留为 frozen-source-final.json，canonical SHA-256 为 `c033b7c3892afd245e6ca309911f12ced1f83045e038969d541754999a1fabdc`，不再视作当前冻结身份。

第四轮 coverage-release 正式共享池 37 项和模块 244 项全部通过；核心 247 项运行结束为 246 通过、1 失败，唯一失败为 DomainParticipantTest.DomainParticipant_delete_multitopic3，整轮 passed=false，未生成最终覆盖率与示例结果。新 Reader 池使用为空的 wire Topic 名注册，使 MultiTopic Reader enable 失败；此前测试没有断言 Reader 创建成功，随后 delete_multitopic 返回 OK 而预期 PRECONDITION_NOT_MET，暴露真实生产兼容性回归。

本次最小生产修复让 MultiTopic 使用自身结果 Topic 名与类型作为池身份；ContentFilteredTopic 继续使用其关联 wire Topic。新增 MultiTopicReadersUseResultIdentityAndPreserveReferencedLifetime 回归，覆盖共享池身份、活动引用阻止删除及缓存保活；独立静态复核通过，该新增项和此前失败的核心用例均已在 coverage-release2 正式通过。失败后再次将 137 个 gcda 归档/清空，记录为 multitopic-freeze/coverage-reset.json，归档 SHA-256 为 `840b185c6a51a1460db4c54806ed65b2f36a177933e64678eba57556fb47f0fd`。

当前 321 项清单为 frozen-source-multitopic.json，canonical SHA-256 为 `6ef3367ca3902a456b5ea5a59881f7190582fedac7fc4be78fbb85bc0d81cead`。已通过的 coverage-release2、asan-final 与 tsan-final 分别使用 build-coverage、build-asan 和 build-tsan，均为 768 MiB 与低优先级单线程配置，绑定同一当前源码身份。本轮外部审计已核对这三个目录、19 项 Reader 回归及新的 137 个计数归档，不覆盖四次失败、旧冻结清单或 10/30 历史证据。

## 当前三轮完整复现命令

在含上述固定基线的 Git 检出目录根执行，先核对当前源码身份。每轮使用新的空证据目录和独立构建目录，按下列顺序串行运行；新的 build-coverage 没有旧 gcda，无需导入本机历史计数。运行器默认编译上限仍为 512 MiB，以下必须显式传入 **--compiler-memory-mib 768**，避免复用本轮已失败的较低上限；--trim-build-cache 仅删除有清单的可重建对象，保留覆盖率注记/计数、安装库、程序及 GTest archives。

```bash
set -e
DS_REPRO_ROOT="$PWD/../reproduce-20261031"

nice -n 19 ionice -c 2 -n 7 python3 tools/ospp/production_lockfree_verify.py \
  --output "$DS_REPRO_ROOT/coverage-release2" \
  --build-dir "$DS_REPRO_ROOT/build-coverage" \
  --mode coverage --scope integration --core --examples 3 \
  --baseline 489067108405b27afbd369245542f5e88d457f18 \
  --compiler-memory-mib 768 --compile-timeout 7200 --trim-build-cache

nice -n 19 ionice -c 2 -n 7 python3 tools/ospp/production_lockfree_verify.py \
  --output "$DS_REPRO_ROOT/asan-final" \
  --build-dir "$DS_REPRO_ROOT/build-asan" \
  --mode asan --scope focused \
  --focus-test KeyedNativeCaptureFailurePreservesLoanAndRollsBackRegistration \
  --focus-test KeyedNativeReaderCaptureFailureDoesNotConsumeInstanceCapacityOrRetainSource \
  --focus-test NativeReaderIntegration.KeyedNativeKeySnapshotSurvivesSingleSlotReuseAndWriterDeletion \
  --compiler-memory-mib 768 --compile-timeout 7200 --trim-build-cache

nice -n 19 ionice -c 2 -n 7 python3 tools/ospp/production_lockfree_verify.py \
  --output "$DS_REPRO_ROOT/tsan-final" \
  --build-dir "$DS_REPRO_ROOT/build-tsan" \
  --mode tsan --scope focused \
  --focus-test KeyedNativeCaptureFailurePreservesLoanAndRollsBackRegistration \
  --focus-test KeyedNativeReaderCaptureFailureDoesNotConsumeInstanceCapacityOrRetainSource \
  --focus-test NativeReaderIntegration.KeyedNativeKeySnapshotSurvivesSingleSlotReuseAndWriterDeletion \
  --compiler-memory-mib 768 --compile-timeout 7200 --trim-build-cache
```

运行器固定 --parallel 1，编译子进程单独应用地址空间上限；ASan/TSan 运行时 shadow mapping 不受该上限影响。ASan 自动设置 detect_leaks=0 与 UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1；TSan 自动应用上述 setarch 和选项。三轮预期分别为普通 529 项/9 次示例/覆盖率、两版 focused 各 86 项；失败时保留新目录中的原始记录，不覆盖本次已通过证据。只解压 git archive 源码而没有固定基线的 Git 历史，不能直接重算差异覆盖率；仍可先用便携复核核对源码身份，再在包含基线的检出目录重建。

## 便携证据与复现

源码包与证据包分开交付。源码冻结并提交后，源码包可以由 git archive 导出该确切提交；未提交的工作树不能用 git archive HEAD 代替。证据工具会从每个原始 results.json 的 source_sha256 计算排序紧凑 JSON 的 SHA-256，按这个身份保存源码清单；不同历史源码不会被合并成一个新版本的通过结果。

主证据包只收集 coverage-release2、asan-final、tsan-final 三个当前成功 run，以及单独暂存的 current-audit-metadata；后者包含最终审计、资源创建/调整/清理、当前冻结清单、计数重置元数据和 62 份原字节编译/链接记录。flags.make 以 .txt 后缀保存原字节，映射索引记录原路径与哈希；没有改写结果 JSON 或混入旧 results.json。主包恰有 **3 个 passed=true run、1 个当前源码身份、221 个已哈希文件**，打包和搬迁后复核均使用 --require-source-match。

历史补充包保留四个 passed=false 原始 run、两套诊断目录、3 个旧源码身份和 89 个已哈希文件。诊断没有伪造 results.json，不算作新通过轮次。搬迁后不带 --require-source-match 的复核通过，三个旧身份均如实报告 changed、missing 为空；与当前源码不匹配是预期历史状态。

| 已生成的小包 | 压缩字节数 | 归档 SHA-256 | manifest.json SHA-256 |
|---|---|---|---|
| [当前主包](../../../artifacts/portable-evidence-20261031/native-v4-final-review.tar.gz) / [相对索引](../../../artifacts/portable-evidence-20261031/native-v4-final-review/README.md) | 319712 | `16eebf15b37fc460db8439bdb608ed28c2ea85ca31304d58e7c6121f3bcdc627` | `c3ced7838c57db467226b94160b49a6079ed9484888240a5a27b66e35c0de4d6` |
| [历史补充包](../../../artifacts/portable-evidence-20261031/native-v4-history-review.tar.gz) / [相对索引](../../../artifacts/portable-evidence-20261031/native-v4-history-review/README.md) | 285651 | `5c79838a532785b09fb5d1a1b6a0505350eebca4697429bf1ea0f292282d785b` | `b3dfc4e3ec2d4ab617e9cc253060cc3a5a0b7006c5873313a5ad20bf60bd9cbb` |

两包从归档解压到新的目录，321 项源码也复制到独立位置，分别使用包内自带 verifier 校验，均退出 0；[搬迁复核原始记录](../../../artifacts/portable-evidence-20261031/native-v4-relocation-verification.json) 保留精确命令和源码差异。以下为生成主包及搬迁后复核的等价命令，路径相对于交付目录；重新打包必须选新的输出名：

```bash
python3 tools/ospp/package_evidence.py package \
  --source . \
  --evidence ../run-evidence/coverage-release2 \
  --evidence ../run-evidence/asan-final \
  --evidence ../run-evidence/tsan-final \
  --evidence ../current-audit-metadata \
  --output ../evidence --archive ../evidence.tar.gz \
  --require-source-match

cd ..
python3 evidence/verify_evidence.py verify \
  --bundle evidence --source source --require-source-match
```

这里 source/ 为解压后的源码目录，run-evidence/ 为运行器证据根；上表索引可在当前工作区直接相对打开，独立交付时使用各归档内的 README.md。主包搬迁后必须继续通过 --require-source-match；历史补充包省略该选项，单独审阅旧身份的 changed/missing。打包器仅复制小型原始文件，不改写原 JSON 中记录的机器路径；搬迁校验只使用包内相对路径与源码相对哈希，不访问这些旧绝对路径。

打包和复核要求 Python 3.9 或更新版本。包内自带已哈希的 verify_evidence.py，审阅旧源码包无需额外寻找新工具。README.md 提供相对链接；manifest.json 列出每个文件的大小、SHA-256、排除项、原始通过/失败状态及源码身份。生产库、可执行文件、对象、构建目录和覆盖缓存不打入小包；它们在原始结果中只保留哈希，重新运行必须从匹配源码重建。包完整性通过只证明文件与源码身份一致，不等于重新执行了测试，也不能独立证明未随包交付的二进制插桩或链接。

## 10/30 与更早的历史证据

[10/30 严格 key-only 报告](DATASHARING-STRICT-20261030.md) 及其原始证据保持不变，绑定 discovery v3 的 319 项输入，canonical 身份为 `6f1669dbaa1608ba96c58ebc39d48adc417958b644667b0de68c1094535670b9`。当时 500 项回归、9 次示例、93.920% 新增行覆盖率和两版 focused 各 57 项通过；它们未验证本轮 Reader Topic 池和无初始化契约，不能作为本轮通过数字。

已使用打包工具收集 10/30 历史小证据：strict-native-v3-review.tar.gz 共 439225 字节，包内 200 个已哈希文件、5 份原始运行结果和 3 个源码身份；两次失败尝试的 passed=false 保留。归档 SHA-256 为 `5ef374f7b5496a796ab7990d1e144f495aad240944272446dab2945509e2a1bd`，manifest.json SHA-256 为 `be8ce48d8ae16d583fcf75e05c4e0def90e6275cf3670872eaacc2fcb7880a8b`。轻量校验覆盖搬迁后读取、失败结果保留、排除大库/构建树、源码不匹配和文件篡改拒绝；这不计为生产功能回归。当前开发源码与历史身份不匹配是预期状态，不能用该包给本轮背书。

[性能手册](DATASHARING-PERFORMANCE.md) 的 10/28 布局 v5 测量同样属于历史，不代表当前 v6/v4 的性能。本轮未测量新的吞吐、延迟或 RSS；共享数据面不获取跨进程管理 mutex 仍不等于整个 DDS write 路径形式化无锁，本地锁、控制注册/维护和 futex 系统调用继续存在。
