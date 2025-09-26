# dds-security-sm

本项目为基于OMG DDS Security 1.2标准的商密算法（SM2/SM3/SM4等）安全插件实现框架，支持DDS安全认证、访问控制、加密、插件接口。

## 主要特性

- 遵循DDS Security 1.2标准
- 插件化支持商密算法（SM2/SM3/SM4）
- 与travoDDS集成


## 目录结构

```
include/dds_security_sm/         # 插件接口与类型定义
include/dds_security_sm/SM/      # SM算法适配层
src/                             # 插件实现
src/SM/                          # SM算法实现
cmake/                           # CMake模块
 test/                            # 单元测试
 docs/                            # 文档
```
## 准备工作
需要安装以下依赖的程序、模块或库
- ubuntu22.04
```bash
sudo apt install cmake
sudo apt install build-essential
sudo apt install libasio-dev
sudo apt install libgtest-dev
sudo apt install libtinyxml2-dev
# 覆盖率测试需要安装
sudo apt install lcov
```

## 快速开始
1. 安装包含商密算法的加密算法库（如openSSL等支持SM2/SM3/SM4的实现）
2. 配置CMake并编译
3. 参考`test/`目录进行功能验证

## 单元测试
```bash
mkdir -p build && cd build
cmake ..
cmake --build .
make test
```
## 覆盖率测试
```bash
# 使用 COVERAGE=ON 打开覆盖率，重新编译代码。
cmake -DCOVERAGE=ON ..
# 在build目录下可以执行如下一条龙脚本：
rm -rf ../build/* ; cmake -DCOVERAGE=ON .. ; cmake --build . ; make test

# 编译完成后，进行覆盖率数据初始化、运行测试、生成覆盖率数据及报表。一条龙脚本如下
$$$ bash << EOF
# 创建原始代码覆盖率数据
lcov -d . -z
lcov -d . -b $(dirname `pwd`) --no-external --initial -c -o initCoverage.info
# 运行代码或执行测试
ctest
# 生成新代码覆盖率数据
lcov -d . -b $(dirname `pwd`) --no-external  -c -o testCoverage.info
# 生成代码覆盖率报表(html格式)
mkdir -p html
genhtml -o html --prefix=$(dirname `pwd`) initCoverage.info testCoverage.info
EOF
# $$$ 一条龙脚本结束
```
## 参考文档

- [OMG DDS Security 1.2](https://www.omg.org/spec/DDS-SECURITY/1.2)

