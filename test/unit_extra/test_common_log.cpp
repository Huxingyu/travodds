#include "common/log/logger.h"
#include "gtest/gtest.h"
#include <cstdlib>
#include <fstream>
#include <string>

USING_TRAVODDS_NAMESPACE

TEST(DDSTest, CommonLog_initialize_destroy_idempotent)
{
    EXPECT_EQ(LogDestroy(), 0);
    EXPECT_EQ(LogInitial(), 0);
    EXPECT_EQ(LogInitial(), 0);

    LogToConsole(LOG_INFO, __FILE__, __FUNCTION__, __LINE__, "unit-test-log-%d", 1);
    LogToConsole(LOG_WARNING, __FILE__, __FUNCTION__, __LINE__, "unit-test-log-%d", 2);

    EXPECT_EQ(LogDestroy(), 0);
    EXPECT_EQ(LogDestroy(), 0);
}

TEST(DDSTest, CommonLog_env_mask_parse)
{
    EXPECT_EQ(LogDestroy(), 0);

#ifndef _WIN32
    setenv("TRAVODDS_LOG_MASK", "console:0x0,file:0x0", 1);
#endif

    EXPECT_EQ(LogInitial(), 0);
    LogToConsole(LOG_ERROR, __FILE__, __FUNCTION__, __LINE__, "masked-log");
    EXPECT_EQ(LogDestroy(), 0);

#ifndef _WIN32
    unsetenv("TRAVODDS_LOG_MASK");
#endif
}

// ========== 新增测试用例 ==========

TEST(DDSTest, CommonLog_all_log_levels)
{
    EXPECT_EQ(LogDestroy(), 0);

#ifndef _WIN32
    // 设置所有级别都输出
    setenv("TRAVODDS_LOG_MASK", "console:0xF,file:0xF", 1);
#endif

    EXPECT_EQ(LogInitial(), 0);

    // 测试所有日志级别
    LogToConsole(LOG_ERROR, __FILE__, __FUNCTION__, __LINE__, "error-log-test");
    LogToConsole(LOG_WARNING, __FILE__, __FUNCTION__, __LINE__, "warning-log-test");
    LogToConsole(LOG_DEBUG, __FILE__, __FUNCTION__, __LINE__, "debug-log-test");
    LogToConsole(LOG_INFO, __FILE__, __FUNCTION__, __LINE__, "info-log-test");

    EXPECT_EQ(LogDestroy(), 0);

#ifndef _WIN32
    unsetenv("TRAVODDS_LOG_MASK");
#endif
}

TEST(DDSTest, CommonLog_console_only_mask)
{
    EXPECT_EQ(LogDestroy(), 0);

#ifndef _WIN32
    // 只输出到控制台，不输出到文件
    setenv("TRAVODDS_LOG_MASK", "console:0xF,file:0x0", 1);
#endif

    EXPECT_EQ(LogInitial(), 0);
    LogToConsole(LOG_INFO, __FILE__, __FUNCTION__, __LINE__, "console-only-log");
    EXPECT_EQ(LogDestroy(), 0);

#ifndef _WIN32
    unsetenv("TRAVODDS_LOG_MASK");
#endif
}

TEST(DDSTest, CommonLog_file_only_mask)
{
    EXPECT_EQ(LogDestroy(), 0);

#ifndef _WIN32
    // 只输出到文件，不输出到控制台
    setenv("TRAVODDS_LOG_MASK", "console:0x0,file:0xF", 1);
#endif

    EXPECT_EQ(LogInitial(), 0);
    LogToConsole(LOG_INFO, __FILE__, __FUNCTION__, __LINE__, "file-only-log");
    EXPECT_EQ(LogDestroy(), 0);

#ifndef _WIN32
    unsetenv("TRAVODDS_LOG_MASK");
#endif
}

TEST(DDSTest, CommonLog_separate_env_vars)
{
    EXPECT_EQ(LogDestroy(), 0);

#ifndef _WIN32
    // 测试单独的环境变量
    setenv("TRAVODDS_LOG_CONSOLE_MASK", "0x7", 1);
    setenv("TRAVODDS_LOG_FILE_MASK", "0x3", 1);
#endif

    EXPECT_EQ(LogInitial(), 0);
    LogToConsole(LOG_ERROR, __FILE__, __FUNCTION__, __LINE__, "separate-vars-log");
    LogToConsole(LOG_DEBUG, __FILE__, __FUNCTION__, __LINE__, "separate-vars-debug");
    EXPECT_EQ(LogDestroy(), 0);

#ifndef _WIN32
    unsetenv("TRAVODDS_LOG_CONSOLE_MASK");
    unsetenv("TRAVODDS_LOG_FILE_MASK");
#endif
}

TEST(DDSTest, CommonLog_null_file_func)
{
    EXPECT_EQ(LogDestroy(), 0);

#ifndef _WIN32
    setenv("TRAVODDS_LOG_MASK", "console:0xF,file:0xF", 1);
#endif

    EXPECT_EQ(LogInitial(), 0);
    // 测试 null file 和 func 参数
    LogToConsole(LOG_INFO, nullptr, nullptr, 0, "null-file-func-log");
    EXPECT_EQ(LogDestroy(), 0);

#ifndef _WIN32
    unsetenv("TRAVODDS_LOG_MASK");
#endif
}

TEST(DDSTest, CommonLog_long_message)
{
    EXPECT_EQ(LogDestroy(), 0);

#ifndef _WIN32
    setenv("TRAVODDS_LOG_MASK", "console:0xF,file:0xF", 1);
#endif

    EXPECT_EQ(LogInitial(), 0);
    // 测试长消息
    std::string long_msg(1000, 'A');
    LogToConsole(LOG_INFO, __FILE__, __FUNCTION__, __LINE__, "%s", long_msg.c_str());
    EXPECT_EQ(LogDestroy(), 0);

#ifndef _WIN32
    unsetenv("TRAVODDS_LOG_MASK");
#endif
}

TEST(DDSTest, CommonLog_multiple_format_args)
{
    EXPECT_EQ(LogDestroy(), 0);

#ifndef _WIN32
    setenv("TRAVODDS_LOG_MASK", "console:0xF,file:0xF", 1);
#endif

    EXPECT_EQ(LogInitial(), 0);
    // 测试多个格式化参数
    LogToConsole(LOG_INFO, __FILE__, __FUNCTION__, __LINE__, 
        "int=%d, float=%f, string=%s, hex=0x%x",
        42, 3.14, "test", 0xFF);
    EXPECT_EQ(LogDestroy(), 0);

#ifndef _WIN32
    unsetenv("TRAVODDS_LOG_MASK");
#endif
}

TEST(DDSTest, CommonLog_rapid_logging)
{
    EXPECT_EQ(LogDestroy(), 0);

#ifndef _WIN32
    setenv("TRAVODDS_LOG_MASK", "console:0xF,file:0xF", 1);
#endif

    EXPECT_EQ(LogInitial(), 0);
    // 测试快速连续日志
    for (int i = 0; i < 100; ++i) {
        LogToConsole(LOG_INFO, __FILE__, __FUNCTION__, __LINE__, "rapid-log-%d", i);
    }
    EXPECT_EQ(LogDestroy(), 0);

#ifndef _WIN32
    unsetenv("TRAVODDS_LOG_MASK");
#endif
}

TEST(DDSTest, CommonLog_mixed_levels_rapid)
{
    EXPECT_EQ(LogDestroy(), 0);

#ifndef _WIN32
    setenv("TRAVODDS_LOG_MASK", "console:0xF,file:0xF", 1);
#endif

    EXPECT_EQ(LogInitial(), 0);
    // 测试混合级别快速日志
    LogType levels[] = {LOG_ERROR, LOG_WARNING, LOG_DEBUG, LOG_INFO};
    for (int i = 0; i < 40; ++i) {
        LogToConsole(levels[i % 4], __FILE__, __FUNCTION__, __LINE__, "mixed-level-%d", i);
    }
    EXPECT_EQ(LogDestroy(), 0);

#ifndef _WIN32
    unsetenv("TRAVODDS_LOG_MASK");
#endif
}

TEST(DDSTest, CommonLog_selective_mask)
{
    EXPECT_EQ(LogDestroy(), 0);

#ifndef _WIN32
    // 只输出 ERROR 和 WARNING
    setenv("TRAVODDS_LOG_MASK", "console:0x3,file:0x3", 1);
#endif

    EXPECT_EQ(LogInitial(), 0);
    LogToConsole(LOG_ERROR, __FILE__, __FUNCTION__, __LINE__, "error-should-show");
    LogToConsole(LOG_WARNING, __FILE__, __FUNCTION__, __LINE__, "warning-should-show");
    LogToConsole(LOG_DEBUG, __FILE__, __FUNCTION__, __LINE__, "debug-should-not-show");
    LogToConsole(LOG_INFO, __FILE__, __FUNCTION__, __LINE__, "info-should-not-show");
    EXPECT_EQ(LogDestroy(), 0);

#ifndef _WIN32
    unsetenv("TRAVODDS_LOG_MASK");
#endif
}

TEST(DDSTest, CommonLog_partial_mask)
{
    EXPECT_EQ(LogDestroy(), 0);

#ifndef _WIN32
    // 控制台输出所有，文件只输出 ERROR
    setenv("TRAVODDS_LOG_MASK", "console:0xF,file:0x1", 1);
#endif

    EXPECT_EQ(LogInitial(), 0);
    LogToConsole(LOG_ERROR, __FILE__, __FUNCTION__, __LINE__, "error-both");
    LogToConsole(LOG_INFO, __FILE__, __FUNCTION__, __LINE__, "info-console-only");
    EXPECT_EQ(LogDestroy(), 0);

#ifndef _WIN32
    unsetenv("TRAVODDS_LOG_MASK");
#endif
}
