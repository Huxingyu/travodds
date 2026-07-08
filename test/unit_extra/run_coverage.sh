#!/bin/bash
# TRAVODDS 单元测试覆盖率脚本
# 所有产物统一放在 unit_extra/build 目录下

set -e

GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
BLUE='\033[0;34m'
NC='\033[0m'

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
WORKSPACE="${SCRIPT_DIR}/../.."
TRAVODDS_DIR="${WORKSPACE}"
BUILD_DIR="${SCRIPT_DIR}/build"
GTEST_ZIP="${SCRIPT_DIR}/../googletest-1.17.0.zip"

# 子目录
GTEST_DIR="${BUILD_DIR}/gtest"
LIB_DIR="${BUILD_DIR}/travodds"
TEST_BUILD="${BUILD_DIR}/test"
COVERAGE_DIR="${BUILD_DIR}/coverage"

echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}TRAVODDS 单元测试覆盖率${NC}"
echo -e "${GREEN}========================================${NC}"

# ---- 检查依赖 ----
for cmd in lcov genhtml g++ cmake make; do
    command -v $cmd &> /dev/null || { echo -e "${RED}错误: 找不到 $cmd${NC}"; exit 1; }
done
[ -f "${GTEST_ZIP}" ] || { echo -e "${RED}错误: 找不到 ${GTEST_ZIP}${NC}"; exit 1; }

mkdir -p "${BUILD_DIR}"

# ---- 准备 GTest ----
echo -e "\n${YELLOW}[1/5] 准备 GTest...${NC}"
GTEST_SRC="${BUILD_DIR}/googletest-1.17.0"
GTEST_BUILD="${GTEST_DIR}/build"
if [ ! -d "${GTEST_SRC}" ]; then
    cd "${BUILD_DIR}" && unzip -q "${GTEST_ZIP}"
fi
if [ ! -f "${GTEST_BUILD}/lib/libgtest.a" ]; then
    mkdir -p "${GTEST_BUILD}" && cd "${GTEST_BUILD}"
    cmake "${GTEST_SRC}" \
        -DCMAKE_BUILD_TYPE=Debug \
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
        -DBUILD_GMOCK=OFF
    make -j$(nproc) gtest gtest_main
fi
echo -e "${GREEN}GTest 就绪${NC}"

# ---- 编译带插桩的 debug 动态库 ----
LIB_FILE="${LIB_DIR}/lib/libtravoddsd.so.1.1"
echo -e "\n${YELLOW}[2/5] 编译带插桩的 debug 动态库...${NC}"
if [ -f "${LIB_FILE}" ]; then
    echo -e "${GREEN}动态库已存在，跳过编译: ${LIB_FILE}${NC}"
else
    rm -rf "${LIB_DIR}" && mkdir -p "${LIB_DIR}/lib"
    cd "${LIB_DIR}"
    cmake "${TRAVODDS_DIR}" \
        -DCMAKE_BUILD_TYPE=Debug \
        -DBUILD_SHARED_LIBS=ON \
        -DSECURITY=ON \
        -DCMAKE_CXX_FLAGS="--coverage -O0" \
        -DCMAKE_C_FLAGS="--coverage -O0" \
        -DCMAKE_LIBRARY_OUTPUT_DIRECTORY="${LIB_DIR}/lib"
    make -j$(nproc)
    echo -e "${GREEN}动态库已生成: ${LIB_FILE}${NC}"
fi

# ---- 编译单元测试 ----
echo -e "\n${YELLOW}[3/5] 编译单元测试...${NC}"
rm -rf "${TEST_BUILD}" && mkdir -p "${TEST_BUILD}"
cd "${TEST_BUILD}"
cmake "${SCRIPT_DIR}" \
    -DCMAKE_BUILD_TYPE=Debug \
    -DTRAVODDS_LIB="${LIB_FILE}" \
    -DCMAKE_CXX_FLAGS="--coverage -O0" \
    -DCMAKE_C_FLAGS="--coverage -O0" \
    -DCMAKE_BUILD_RPATH="${LIB_DIR}/lib"
make -j$(nproc)
echo -e "${GREEN}测试可执行文件已生成${NC}"

# ---- 运行测试 ----
echo -e "\n${YELLOW}[4/5] 运行单元测试...${NC}"
export LD_LIBRARY_PATH="${LIB_DIR}/lib:${LD_LIBRARY_PATH}"
find "${BUILD_DIR}" -name "*.gcda" -delete 2>/dev/null || true
cd "${TEST_BUILD}"
./unit_test
# 清理测试产生的日志文件
cd "${SCRIPT_DIR}" && rm -f *.ddslog 2>/dev/null || true
echo -e "${GREEN}测试执行完成${NC}"

# ---- 生成覆盖率报告 ----
echo -e "\n${YELLOW}[5/5] 生成覆盖率报告...${NC}"
rm -rf "${COVERAGE_DIR}" && mkdir -p "${COVERAGE_DIR}"

# 只从动态库目录收集覆盖率数据，排除测试代码，避免 inconsistent 错误
lcov --capture \
    --directory "${LIB_DIR}" \
    --base-directory "${TRAVODDS_DIR}" \
    --output-file "${COVERAGE_DIR}/coverage_total.info" 2>/dev/null || true

# 只提取关注的五个模块
lcov --extract "${COVERAGE_DIR}/coverage_total.info" \
    "*/src/xml/*" \
    "*/src/security/*" \
    "*/src/common/log/*" \
    "*/src/xtypes/*" \
    "*/src/cdr/*" \
    --output-file "${COVERAGE_DIR}/coverage_filtered.info" 2>/dev/null || true

# 排除三方库 tinyxml2
lcov --remove "${COVERAGE_DIR}/coverage_filtered.info" \
    "*/tinyxml2*" \
    --output-file "${COVERAGE_DIR}/coverage_filtered.info" 2>/dev/null || true

genhtml "${COVERAGE_DIR}/coverage_filtered.info" \
    --output-directory "${COVERAGE_DIR}/html" \
    --title "TRAVODDS Coverage Report" \
    --legend \
    --prefix "${TRAVODDS_DIR}"

# ---- 显示结果 ----
echo -e "\n${GREEN}========================================${NC}"
echo -e "${GREEN}覆盖率统计完成！${NC}"
echo -e "${GREEN}========================================${NC}"
echo -e "\n${YELLOW}覆盖率摘要:${NC}"
lcov --summary "${COVERAGE_DIR}/coverage_filtered.info"
echo -e "\n${YELLOW}HTML 报告: ${COVERAGE_DIR}/html/index.html${NC}"
echo -e "${GREEN}完成！${NC}"
