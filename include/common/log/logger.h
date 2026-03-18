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

#ifndef LOGGER_H
#define LOGGER_H

#include "travodds_dll.h"
#include <cstdint>
#include <sstream>
#include <string> 
TRAVODDS_NAMESPACE_BEGIN

/**
 * @enum LogType
 *
 * @brief   用于表示某一条日志的输出等级，对于不同的等级的用户可以看到不同的日志文件。
 */
typedef enum LogType
{
    /** @brief  程序出现不可忽略的错误。  */
    LOG_ERROR = 0x0001 << 0,
    /** @brief  警告信息但是能够正常运行。  */
    LOG_WARNING = 0x0001 << 1,
    /** @brief  通知用户视角信息。  */
    LOG_INFO = 0x0001 << 2,
    /** @brief  系统管理员视角的信息,比如一些调试信息。  */
    LOG_DEBUG = 0x0001 << 3,
} LogType;

/**
 * @fn    int32_t LogInitial(uint32_t consoleMask, uint32_t fileMask);
 *
 * @brief    初始化用户日志接口.
 *
 * @param    consoleMask    输出到控制台的日志等级掩码.
 * @param    fileMask       输出到日志文件的日志等级掩码.
 */
int32_t LogInitial(uint32_t consoleMask = 0x0007, uint32_t fileMask = 0x0007);

/**
 * @fn  int32_t LogDestroy();
 *
 * @brief   回收为日志系统提供资源等。
 */
int32_t LogDestroy();

/**
 * @fn  void LogToConsole(LogType type, const char* file, const char* func, uint32_t line, const char* format, ...);
 *
 * @brief   输出日志到控制台。
 *
 * @param   type    该日志所属的级别。
 * @param   file    产生日志的文件名。
 * @param   func    产生日志的函数名。
 * @param   line    产生日志的行号。
 * @param   format  字符串格式(类printf).
 * @param   ...     变参。
 */
void LogToConsole(LogType type, const char* file, const char* func, uint32_t line, const char* format, ...);

/**
 * @brief   输出日志的宏。
 *
 * @param   type    该日志所属的级别。
 * @param   format  字符串格式(类printf).
 * @param   ...     变参。
 */
#define TRAVODDS_LOG(type, format, ...) LogToConsole(type, __FILE__, __FUNCTION__, __LINE__, format, ##__VA_ARGS__); \

#define EMERGENCY_SECURITY_LOGGING(CLASS, format, ...) TRAVODDS_LOG(TRAVODDS::LOG_ERROR, "[" CLASS "] " format, ##__VA_ARGS__)
#define WARNING_SECURITY_LOGGING(CLASS, format, ...) TRAVODDS_LOG(TRAVODDS::LOG_WARNING, "[" CLASS "] " format, ##__VA_ARGS__)

#define TRAVODDS_LOG_STREAM_IMPL(type, category, stream_input) \
    do { \
        std::stringstream log_stream_buffer__; \
        log_stream_buffer__ << stream_input; \
        std::string final_message__ = "[" #category "] " + log_stream_buffer__.str(); \
        LogToConsole(type, __FILE__, __FUNCTION__, __LINE__, "%s", final_message__.c_str()); \
    } while (0)

#define REC_LOG_ERROR(category, stream_input)   TRAVODDS_LOG_STREAM_IMPL(TRAVODDS::LOG_ERROR,   category, stream_input)
#define REC_LOG_WARNING(category, stream_input) TRAVODDS_LOG_STREAM_IMPL(TRAVODDS::LOG_WARNING, category, stream_input)
#define REC_LOG_DEBUG(category, stream_input)   TRAVODDS_LOG_STREAM_IMPL(TRAVODDS::LOG_DEBUG,   category, stream_input)
#define REC_LOG_INFO(category, stream_input)    TRAVODDS_LOG_STREAM_IMPL(TRAVODDS::LOG_INFO,    category, stream_input)
TRAVODDS_NAMESPACE_END

#endif // LOGGER_H
