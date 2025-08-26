//Copyright(c)[2025][AGIROS][TravoDDS] is licensed under Mulan PSL v2.
//
//You can use this software according to the terms and conditions of
//the Mulan PSL v2.You may obtain a copy of Mulan PSL v2 at :
//http://license.coscl.org.cn/MulanPSL2
//
//THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF
//ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
//
//See the Mulan PSL v2 for more details.

#include "common/log/logger.h"
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

TRAVODDS_NAMESPACE_BEGIN

static uint32_t g_consoleMask = 0xffff;
static uint32_t g_fileMask = 0xffff;

int32_t LogInitial(uint32_t consoleMask, uint32_t fileMask)
{
    g_consoleMask = consoleMask;
    g_fileMask = fileMask;
    return 0;
}

int32_t LogDestroy()
{
    g_consoleMask = 0;
    g_fileMask = 0;
    return 0;
}

void LogToConsole(LogType type, const char* file, const char* func, 
    uint32_t line, const char* format, ...)
{
    if (!(g_consoleMask & type)) {
        return;
    }

    time_t now;
    time(&now);
    struct tm* timeinfo = localtime(&now);
    char timeStr[32];
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", timeinfo);

    // 打印时间和日志级别
    const char* typeStr = "";
    switch (type) {
        case LOG_ERROR: typeStr = "ERROR"; break;
        case LOG_WARNING: typeStr = "WARNING"; break;
        case LOG_DEBUG: typeStr = "DEBUG"; break;
        case LOG_INFO: typeStr = "INFO"; break;
        default: typeStr = "UNKNOWN"; break;
    }
    
    printf("[%s][%s]", timeStr, typeStr);
    
    // 打印文件信息
    if (file && func) {
        printf("[%s:%s:%d]", file, func, line);
    }
    
    // 打印日志内容
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    
    printf("\n");
}

TRAVODDS_NAMESPACE_END
