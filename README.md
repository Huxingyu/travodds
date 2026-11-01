# TravoDDS

[![Build Status](https://img.shields.io/badge/build-passing-brightgreen)](https://gitee.com/agiros/travodds)
[![License](https://img.shields.io/badge/license-MulanPSL--2.0-blue)](LICENSE)

本仓库在 [AGIROS / TravoDDS（Gitee 上游）](https://gitee.com/agiros/travodds) 基础上继续开发，保留上游提交历史、分支与标签。GitHub 开发主分支为 `main`，`master` 保留迁移时的上游版本。

本分支已加入 DataSharing、PayloadPool 与 Loan API。参见 [当前验收矩阵](docs/DATASHARING-ACCEPTANCE.md)、[示例](example/cpp/data_sharing/README.md)、[测试指南](docs/DATASHARING-TESTING.md) 和 [GitHub 迁移及同步上游](docs/GITHUB-UPSTREAM.md)。

## 目录

- [介绍](#介绍)
- [关键特性](#关键特性)
- [软件架构](#软件架构)
- [快速开始](#快速开始)
- [API 参考](#api-参考简要)
- [测试](#测试)
- [发布与打包](#发布与打包)
- [文档](#文档)
- [FAQ](#常见问题-faq)
- [贡献指南](#贡献指南)
- [常见配置与日志](#常见配置与日志)
- [Changelog](#changelog)
- [许可证](#许可证)

## 介绍

travodds 是 AGIROS 开源社区自研的 DDS 数据分发通信中间件，支持完整的发布/订阅机制、15 种 QoS 策略、插件化架构、X86/ARM/RISC-V 异构平台、国产系统以及跨平台运行。

## 关键特性

- 完整 DDS 数据生命周期：发现、连接、QoS 策略、数据传输、可靠性保障
- 支持 15 种 QoS 策略灵活配置（可靠性、历史、持久性、寿命、延迟预算）
- 支持 C/C++ API 以及向上对接 AGIROS 平台的接口集成
- 插件化可扩展设计：运行时动态加载/卸载模块
- 支持多平台：x86、ARM、RISC-V；Ubuntu、openEuler、国产麒麟等
- 支持 TCP、UDP、共享内存传输方案
- 安全框架：认证、访问控制与安全策略

## 软件架构

- `cmake/` 生成配置
- `include/` 公共头文件
- `src/` 核心实现（common、dcps、rtps、qos、security、xml、cdr、type、xtypes）
- `test/` 测试用例与单元测试
- `example/` 示例代码
- `tools/` 辅助脚本
- `thirdparty/` 依赖库
- `rpm/`、`debian/`、`rpm-agiros-loong/`、`debian-agiros-loong/` 打包脚本

## 快速开始

### 1. 环境准备

- Linux (Ubuntu 22.04+/openEuler22.03+/国产麒麟)
- CMake 3.16+
- GCC/Clang
- make（或使用 cmake --build）

### 2. 克隆仓库

```bash
git clone https://github.com/Huxingyu/travodds.git
cd travodds
```

### 3. 构建

```bash
mkdir -p build && cd build
cmake ..
make
sudo make install
```

### 4. 运行示例（Hello World）

```bash
cd example/cpp/hello_world
mkdir -p build && cd build
cmake ..
make
./hello_world pub
./hello_world sub
```

## 测试

```bash
cd test/unit_inside
mkdir -p build && cd build
cmake ..
make
./unit_test
```

DataSharing/Loan 的测试程序、串行运行、检查器及当前验收范围见 [测试指南](docs/DATASHARING-TESTING.md)。

## API 参考（简要）

1. DDS工厂接口：`TheParticipantFactory`
2. 创建 DomainParticipant、Publisher/Subscriber
3. 创建 Topic、DataWriter/DataReader
4. 设置 QoS：`set_qos()`
5. 数据读写：`write()`, `read()` 或回调监听
6. 资源释放：`delete_datawriter()`, `delete_datareader()`, `delete_contained_entities()`

> 详细 API 请参见 `include/` 头文件与 `docs/`。

## 常见配置与日志

- 日志级别：`debug` / `info` / `warn` / `error`

## 发布与打包

- Debian/Ubuntu: `dpkg.sh` 或 `debian/` 下规则
- RPM: `rpm.sh` 或 `rpm/` 下规范
- AGIROS: `debian-agiros-loong/`、`rpm-agiros-loong/`
- 自动化打包: shell 运行对应 `*.sh`，即可完成对应自动打包

## 文档

- `docs/` 业务文档
- `README.en.md` 英文说明

## 常见问题 (FAQ)

Q: CMake 项目工程中如何快速加载 `TravoDDS` 库？
A: 请参考 `example/cpp/hello_world/CMakeLists.txt`。

```cmake
find_package(TRAVODDS 1 REQUIRED)
# include directories and library directories (if needed) from TRAVODDS package
# target_link_libraries 可以直接使用导出的目标
# 例如：
# target_link_libraries(${PROJECT_NAME} PRIVATE TRAVODDS::TRAVODDS)

# 若需手动设置：
# include_directories(${TRAVODDS_INCLUDE_DIRS})
# link_directories(${TRAVODDS_LIBRARIES_DIRS})
```

Q: `example/cpp/hello_world` 下运行失败？
A: 请先确认已执行 `cd example/cpp/hello_world && mkdir -p build && cd build && cmake .. && make`，然后再执行 `./hello_world pub` 或 `./hello_world sub`。

## 贡献指南

1. Fork 本仓库
2. 新建 `feat/<your-feature>` 分支
3. 提交代码并推送
4. 创建 Pull Request 并描述变更

### 代码规范

- 遵循项目 CMake 和 clang-format 规则
- 提交前运行单元测试

## Changelog
### [1.1.0] - 2026-06-25
#### Added
- 添加 Sample Identity (Extension for RPC)
### [1.1.0] - 2026-06-18
#### Added
- 添加 test/performance_test 性能测试用例
### [1.1.0] - 2026-06-09
#### Fixed
- 修复 ACK（丢包情况） 数据重传失败问题
### [1.1.0] - 2026-06-04
#### Fixed
- 修复 ACK（丢包情况） 和 HB 报文应答问题
### [1.1.0] - 2026-06-02
#### Fixed
- 修复 ACK 和 HB 报文应答问题
- 修复 SHM 新连接并行建立连接问题
- 修复 SHM 旧连接释放问题
### [1.1.0] - 2026-05-21
#### Changed
- travodds_dll.h.in 设置成模板文件，需要添加的宏均模板加入
### [1.1.0] - 2026-05-18
#### Fixed
- 添加 WriterProxy 资源异步保护

### [1.1.0] - 2026-04-03

#### 新增功能 (Added)
- 

#### 修复 (Fixed)
- 

#### 改进 (Changed)
- 

#### 移除 (Removed)
- 

#### 文档 (Documentation)
- 添加 README.md 和 README.en.md


## 许可证

本项目采用 [木兰宽松许可证第 2 版（MulanPSL-2.0）](LICENSE)。保留上游及各第三方组件的版权声明和许可证。
