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
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <fstream>
#include <string>
#include <memory>
#include <chrono>
#include <sstream>
#include <sys/stat.h>

#ifdef _WIN32
#include <windows.h>
#include <process.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <iostream>
#include <fstream>
#endif

TRAVODDS_NAMESPACE_BEGIN

// 日志记录结构体
struct LogRecord {
    LogType type;
    std::string timestamp;
    std::string file;
    std::string func;
    uint32_t line;
    std::string message;
    bool toConsole;
    bool toFile;
};

// 异步日志系统类
class AsyncLogger {
private:
    std::thread logThread;
    std::queue<LogRecord> logQueue;
    std::mutex queueMutex;
    std::condition_variable condition;
    bool running;
    std::ofstream logFile;
    std::string currentLogFileName;
    size_t currentFileSize;
    uint32_t consoleMask;
    uint32_t fileMask;
    static const size_t MAX_FILE_SIZE = 1024 * 1024; // 1MB

    std::string getProcessName() {
#ifdef _WIN32
        // Windows平台: 使用GetModuleFileNameA获取完整路径
        char buffer[MAX_PATH];
        GetModuleFileNameA(NULL, buffer, MAX_PATH);
        std::string path(buffer);
        size_t pos = path.find_last_of("\\");
        if (pos != std::string::npos) {
            path = path.substr(pos + 1);
        }
        pos = path.find_last_of(".");
        if (pos != std::string::npos) {
            path = path.substr(0, pos);
        }
        return path;
#else
        // Linux/Unix平台实现 - 多种方法确保获取到进程名
        // 方法1: 从/proc/self/exe读取符号链接获取完整路径（最准确）
        char buffer[1024];
        ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
        if (len != -1) {
            buffer[len] = '\0';
            std::string path(buffer);
            size_t pos = path.find_last_of('/');
            if (pos != std::string::npos) {
                return path.substr(pos + 1);
            }
            return path;
        }
        
        // 方法2: 从/proc/self/comm读取进程名（限制16字符但通常足够）
        std::ifstream comm_file("/proc/self/comm");
        if (comm_file.is_open()) {
            std::string process_name;
            std::getline(comm_file, process_name);
            comm_file.close();
            // 移除可能的换行符
            if (!process_name.empty() && process_name.back() == '\n') {
                process_name.pop_back();
            }
            if (!process_name.empty()) {
                return process_name;
            }
        }
        
        // 方法3: 尝试从环境变量获取程序路径
        const char* program_name = getenv("_");
        if (program_name) {
            std::string path(program_name);
            size_t pos = path.find_last_of('/');
            if (pos != std::string::npos) {
                return path.substr(pos + 1);
            }
            return path;
        }
        
        // 最后的后备方案
        return "travodds";
#endif
    }

    uint32_t getProcessId() {
#ifdef _WIN32
        return GetCurrentProcessId();
#else
        return getpid();
#endif
    }

    std::string generateLogFileName() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        
        std::stringstream ss;
        ss << getProcessName() << "_" << getProcessId() << "_";
        
        struct tm timeinfo;
#ifdef _WIN32
        struct tm* tm_ptr = localtime(&time_t);
        if (tm_ptr) {
            timeinfo = *tm_ptr;
        }
#else
        localtime_r(&time_t, &timeinfo);
#endif
        
        char timeStr[32];
        strftime(timeStr, sizeof(timeStr), "%Y%m%d_%H%M%S", &timeinfo);
        ss << timeStr << "_" << ms.count() << ".ddslog";
        
        return ss.str();
    }

    void openNewLogFile() {
        if (logFile.is_open()) {
            logFile.close();
        }
        
        currentLogFileName = generateLogFileName();
        logFile.open(currentLogFileName, std::ios::out | std::ios::app);
        currentFileSize = 0;
    }

    size_t getFileSize(const std::string& filename) {
        struct stat stat_buf;
        int rc = stat(filename.c_str(), &stat_buf);
        return rc == 0 ? stat_buf.st_size : 0;
    }

    void writeToFile(const LogRecord& record) {
        if (!logFile.is_open() || currentFileSize >= MAX_FILE_SIZE) {
            openNewLogFile();
        }

        std::string logLine = "[" + record.timestamp + "][" + getLogTypeString(record.type) + "]";
        if (!record.file.empty() && !record.func.empty()) {
            logLine += "[" + record.file + ":" + record.func + ":" + std::to_string(record.line) + "]";
        }
        logLine += record.message + "\n";

        logFile << logLine;
        logFile.flush();
        currentFileSize += logLine.length();
    }

    std::string getLogTypeString(LogType type) {
        switch (type) {
            case LOG_ERROR: return "ERROR";
            case LOG_WARNING: return "WARNING";
            case LOG_DEBUG: return "DEBUG";
            case LOG_INFO: return "INFO";
            default: return "UNKNOWN";
        }
    }

    void processLogs() {
        while (running) {
            std::unique_lock<std::mutex> lock(queueMutex);
            condition.wait(lock, [this] { return !logQueue.empty() || !running; });

            while (!logQueue.empty()) {
                LogRecord record = logQueue.front();
                logQueue.pop();
                lock.unlock();

                // 输出到控制台
                if (record.toConsole) {
                    printf("[%s][%s]", record.timestamp.c_str(), getLogTypeString(record.type).c_str());
                    if (!record.file.empty() && !record.func.empty()) {
                        printf("[%s:%s:%d]", record.file.c_str(), record.func.c_str(), record.line);
                    }
                    printf("%s\n", record.message.c_str());
                }

                // 输出到文件
                if (record.toFile) {
                    writeToFile(record);
                }

                lock.lock();
            }
        }
    }

public:
    AsyncLogger() : running(false), currentFileSize(0), consoleMask(0xffff), fileMask(0xffff) {}

    ~AsyncLogger() {
        stop();
    }

    void start() {
        if (!running) {
            running = true;
            logThread = std::thread(&AsyncLogger::processLogs, this);
        }
    }

    void stop() {
        if (running) {
            {
                std::lock_guard<std::mutex> lock(queueMutex);
                running = false;
            }
            condition.notify_all();
            if (logThread.joinable()) {
                logThread.join();
            }
            if (logFile.is_open()) {
                logFile.close();
            }
        }
    }

    void log(const LogRecord& record) {
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            logQueue.push(record);
        }
        condition.notify_one();
    }

    void setMasks(uint32_t consoleMaskValue, uint32_t fileMaskValue) {
        consoleMask = consoleMaskValue;
        fileMask = fileMaskValue;
    }

    bool shouldLogToConsole(LogType type) const {
        return (consoleMask & type) != 0;
    }

    bool shouldLogToFile(LogType type) const {
        return (fileMask & type) != 0;
    }
};

static std::unique_ptr<AsyncLogger> g_logger;

int32_t LogInitial(uint32_t consoleMask, uint32_t fileMask)
{
    if (!g_logger) {
        g_logger.reset(new AsyncLogger());
        g_logger->start();
    }
    
    g_logger->setMasks(consoleMask, fileMask);
    return 0;
}

int32_t LogDestroy()
{
    if (g_logger) {
        g_logger->stop();
        g_logger.reset();
    }
    
    return 0;
}

void LogToConsole(LogType type, const char* file, const char* func, 
    uint32_t line, const char* format, ...)
{
    LogInitial();
    bool toConsole = g_logger->shouldLogToConsole(type);
    bool toFile = g_logger->shouldLogToFile(type);
    
    if (!toConsole && !toFile) {
        return;
    }

    // 生成时间戳
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    struct tm timeinfo;
#ifdef _WIN32
    struct tm* tm_ptr = localtime(&time_t);
    if (tm_ptr) {
        timeinfo = *tm_ptr;
    }
#else
    localtime_r(&time_t, &timeinfo);
#endif
    
    char timeStr[32];
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);

    // 格式化消息
    va_list args;
    va_start(args, format);
    
    va_list args_copy;
    va_copy(args_copy, args);
    int len = vsnprintf(nullptr, 0, format, args_copy);
    va_end(args_copy);
    
    std::string message(len + 1, '\0');
    vsnprintf(&message[0], len + 1, format, args);
    va_end(args);
    
    message.resize(len);

    // 创建日志记录
    LogRecord record;
    record.type = type;
    record.timestamp = timeStr;
    record.file = file ? file : "";
    record.func = func ? func : "";
    record.line = line;
    record.message = message;
    record.toConsole = toConsole;
    record.toFile = toFile;

    // 提取文件名（去掉路径）
    if (!record.file.empty()) {
        size_t pos = record.file.find_last_of("/\\");
        if (pos != std::string::npos) {
            record.file = record.file.substr(pos + 1);
        }
    }

    g_logger->log(record);
}

TRAVODDS_NAMESPACE_END
