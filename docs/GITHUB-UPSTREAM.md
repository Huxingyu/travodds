# GitHub 开发与 Gitee 上游同步

GitHub 仓库：<https://github.com/Huxingyu/travodds>，迁移时设为私有。
上游仓库：<https://gitee.com/agiros/travodds>。

这是跨平台导入后继续开发的独立仓库，GitHub 不会显示它与 Gitee 的 Fork 网络关系。Git 提交历史仍然保持相同的对象和父子关系，可以正常比较、合并上游。

## 历史与分支

迁移前先将本地浅克隆补齐为完整仓库。迁移时的上游共有 496 个不同提交、7 个分支、11 个标签；上游 `master` 的历史包含 367 个提交，尖端为 `489067108405b27afbd369245542f5e88d457f18`。

- `main`：GitHub 开发主分支，包含本轮 DataSharing 成果与迁移整理。
- `work/ospp-datasharing-bootstrap`：本轮原始实现提交。
- `datasharing-accepted-20261028`：验收代码快照标签，指向 `145648bd547412e47f64218a0a00723623364dbd`。
- `master`、`develop`、`feature`、`hotfix`、`master-agiros`、`pixiu`、`release`：保留迁移时同名上游分支。
- 上游的 11 个标签保持原对象不变。

原提交哈希、作者、提交日期、提交说明及合并关系均保留。Gitee 的 Issue、合并请求、评论、平台权限和附件不属于 Git 历史，不随本次导入迁移。后续上游变化由 `upstream/*` 跟踪，再合并进 `main`；GitHub 上保留的其他分支不会自动更新。

## 本机远程配置

本次迁移后的本机将 `origin` 指向 GitHub，`upstream` 指向 Gitee，默认推送到 `origin`。`upstream` 的推送地址设为 `DISABLED`，用于读取与同步。

新机器克隆时可这样配置（私有仓库需要 GitHub 访问权限）：

```bash
git clone https://github.com/Huxingyu/travodds.git
cd travodds
git remote add upstream https://gitee.com/agiros/travodds.git
git remote set-url --push upstream DISABLED
git config remote.pushDefault origin
git fetch upstream --tags
git remote -v
```

## 同步上游

先提交或妥善保存工作区修改，再执行：

```bash
git fetch upstream --prune --tags
git switch main
git log --oneline main..upstream/master
git merge upstream/master
```

如果有冲突，解决后完成合并，运行与变更相适应的测试，再执行：

```bash
git push origin main
```

需要放弃本次未完成的合并时可用 `git merge --abort`。本流程使用 merge 保留双方已有提交；同步完成后，新的合并提交也会出现在 `git log` 中。

日常功能开发建议从 `main` 新建工作分支：

```bash
git switch main
git switch -c work/your-feature
```

## 验收快照与发布整理

`145648b` 提交包含验收交付清单中的全部 118 个文件，逐文件 SHA256 与交付清单一致，构建/源码/测试/工具冻结清单校验通过。这是普通版与 ASan/UBSan 各 468 项、重点重复、部分路径 TSan、94.118% 新增生产行覆盖率和性能测量所对应的快照。验证结论及历史 WaitSet 崩溃未归因等限制，见 [完整报告](DATASHARING-COMPLETION-20261028.md)。

报告中“未提交、未推送”的描述是验收交付时的历史状态；随后用户授权迁移到 GitHub，才创建上述提交及标签。

后续迁移整理修正 README 的许可证名称、增加同步说明、忽略本地日志和产物，并清理两个源文件中的四处纯空白问题。没有修改生产逻辑；不将本次迁移检查称为重新运行完整测试。原报告提到的 `git diff --check` 未覆盖当时尚未跟踪的新文件；正式暂存全部文件后发现这些空白问题，因此在独立整理提交中修正，验收标签保持原样。

许可证文件和上游 CMake 版权头声明均为 MulanPSL-2.0；原 README 中 Apache 2.0 的文字已改为与许可证文件一致，第三方组件仍遵循各自许可。

部分机器可读验收材料随代码保存在 [validation/20261028](validation/20261028/README.md)，包括原始哈希清单和结果汇总。完整原始日志、构建目录、候选失败记录及运行产物保留在执行机的 `artifacts/` 中，不纳入 Git 仓库。
