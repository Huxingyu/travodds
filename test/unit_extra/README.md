# unit_extra - 模块单元测试与覆盖率

本目录包含以下 **5 个模块** 的单元测试与覆盖率统计：
- **xml** - XML 配置解析
- **cdr** - CDR 序列化
- **xtypes** - 动态类型系统
- **security** - DDS 安全
- **common_log** - 日志模块

## 目录结构

```
unit_extra/
├── run_coverage.sh      # 一键运行脚本
├── CMakeLists.txt       # 测试构建配置
├── test_*.cpp           # 测试源文件
└── build/               # 所有产物（自动生成）
    ├── gtest/           #   GTest 编译产物
    ├── travodds/        #   debug 动态库（带插桩）
    ├── test/            #   测试可执行文件
    ├── coverage/        #   覆盖率报告
    │   └── html/        #     HTML 报告（浏览器查看）
    └── googletest-*/    #   GTest 源码（解压）
```

## 使用方法

### 一键运行

```bash
cd /home/tbin/travodds-workspace/travodds/test/unit_extra
bash run_coverage.sh
```

### 脚本流程

1. **准备 GTest** - 解压并编译 Google Test（仅 gtest，不含 gmock）
2. **编译动态库** - 带覆盖率插桩的 debug 版 `libtravoddsd.so`（已存在则跳过）
3. **编译测试** - 链接插桩库，编译单元测试
4. **运行测试** - 执行测试用例，收集覆盖率数据
5. **生成报告** - 仅提取 xml、cdr、xtypes、security、common_log 五个模块的覆盖率，输出 HTML 报告

### 查看报告

```bash
# 浏览器打开
xdg-open build/coverage/html/index.html
```

脚本执行完毕后会直接在终端打印覆盖率统计信息。

## 清理产物

```bash
# 删除所有产物，重新编译
rm -rf build/
```

## 注意事项

- 首次运行需要安装 `lcov`、`genhtml`、`cmake`、`g++`
- 动态库编译完成后会跳过，节省时间。删除 `build/travodds/` 即可重新编译
- 所有产物都在 `build/` 目录下，不会污染其他位置
