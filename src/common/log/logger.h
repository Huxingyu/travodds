#ifndef Logger_h__
#define Logger_h__

#include "agidds_dll.h"
#include <cstdint>
AGIDDS_NAMESPACE_BEGIN

/**
 * @enum LogType
 *
 * @brief   用于表示某一条日志的输出等级，对于不同的等级的用户可以看到不同的日志文件。
 */

typedef enum LogType
{
    /** @brief  程序出现不可忽略的错误。  */
    AGILOG_ERROR = 0x0001 << 1,
    /** @brief  警告信息但是能够正常运行。  */
    AGILOG_WARNING = 0x0001 << 2,
    /** @brief  系统管理员视角的信息,比如一些调试信息。  */
    AGILOG_DEBUG = 0x0001 << 3,
    /** @brief  通知用户视角信息。  */
    AGILOG_INFO = 0x0001 << 4,
} LogType;

/**
 * @fn    int32_t AGILogInitial(uint32_t consoleMask, uint32_t fileMask);
 *
 * @brief    初始化用户日志接口.
 *
 * @param    consoleMask    输出到控制台的日志等级掩码.
 * @param    fileMask       输出到日志文件的日志等级掩码.
 */

int32_t AGILogInitial(uint32_t consoleMask, uint32_t fileMask);

/**
 * @fn  int32_t AGILogDestroy();
 *
 * @brief   回收为日志系统提供资源等。
 */

int32_t AGILogDestroy();

/**
 * @fn  void AGILogToConsole(LogType type, const char* file, const char* func, uint32_t line, const char* format, ...);
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
void AGILogToConsole(LogType type, const char* file, const char* func, uint32_t line, const char* format, ...);

/**
 * @brief   输出日志的宏。
 *
 * @param   type    该日志所属的级别。
 * @param   format  字符串格式(类printf).
 * @param   ...     变参。
 */

#define AGILOG(type, format, ...) AGILogToConsole(type, __FILE__, __FUNCTION__, __LINE__, format, ##__VA_ARGS__); \

AGIDDS_NAMESPACE_END

#endif // Logger_h__
