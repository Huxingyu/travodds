# 2026-10-28 验收材料

这些文件是发布前验收材料的原样副本，对应标签
`datasharing-accepted-20261028`，提交
`145648bd547412e47f64218a0a00723623364dbd`。

- `delivery-source.json`：118 个交付文件的 SHA256。
- `frozen-source.json`：构建、源码、测试和工具的输入清单。
- `verification.json`：执行命令、退出码、结果、资源与限制的审计汇总。
- `final-audit.json`：验收交付审计及材料哈希。
- `coverage-summary.json`：覆盖率统计、未覆盖行及源码哈希。
- `benchmark-summary.json`：性能测量汇总。
- `SHA256SUMS`：上述副本的 SHA256。

可在本目录执行 `sha256sum -c SHA256SUMS` 校验材料完整性。
清单和汇总保留原执行路径及原日志路径；完整原始日志未上传，
仍保存在执行机 `artifacts/completion-20261028/`。
这些材料不是第三方验证，也不能替代原日志或自行复现。

报告见 [实现与交付报告](../../DATASHARING-COMPLETION-20261028.md)，
迁移及后续提交说明见 [GitHub 上游同步](../../GITHUB-UPSTREAM.md)。
报告中的“未提交”描述对应验收完成时，早于后续 GitHub 发布授权。
