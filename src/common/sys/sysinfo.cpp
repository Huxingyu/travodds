#include "common/sys/sysinfo.h"	
#include <sstream>
#include <ctime>
#include <iomanip>
// 平台相关头文件
#ifdef _WIN32
#include <windows.h>
#include <process.h>
#include <lmcons.h>
#else
#include <sys/types.h>
#include <signal.h> 
#include <unistd.h>
#include <pwd.h>
#include <limits.h>
#endif

USING_TRAVODDS_NAMESPACE;
using namespace SysInfo;

std::string SysInfo::GetHostname()
{
    static std::string name = GetHostnameStatic();
    return name;
}

std::string SysInfo::GetHostnameStatic()
{
#ifdef _WIN32
    char hostname[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD size = sizeof(hostname);
    if (GetComputerNameA(hostname, &size)) {
        return std::string(hostname);
    }
#else
    char hostname[1024];
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        return std::string(hostname);
    }
#endif
    return "unknown";
}

std::string SysInfo::GetProcessID()
{
    static std::string pid = std::to_string(SysInfo::GetProcessIDStatic());
    return pid;
}

int32_t SysInfo::GetProcessIDStatic()
{
#ifdef _WIN32
    return _getpid();
#else
    return getpid();
#endif
}

bool SysInfo::IsProcessExists(int32_t pid)
{
    if (pid <= 0) {
        return false;
    }

#ifdef _WIN32
    // Windows实现
    HANDLE process = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (process == NULL) {
        return false;
    }

    DWORD exitCode;
    if (GetExitCodeProcess(process, &exitCode)) {
        CloseHandle(process);
        // 如果退出代码不是STILL_ACTIVE，表示进程已终止
        return (exitCode == STILL_ACTIVE);
    }

    CloseHandle(process);
    return false;
#else
    // Linux/Unix实现
    // 发送0信号给进程，如果返回错误表示进程不存在
    return (kill(pid, 0) == 0);
#endif

    return false;
}

std::string SysInfo::GetUsername()
{
    static std::string name = GetUsernameStatic();
    return name;
}

std::string SysInfo::GetUsernameStatic()
{
#ifdef _WIN32
    char username[UNLEN + 1];
    DWORD size = sizeof(username);
    if (GetUserNameA(username, &size)) {
        return std::string(username);
    }
#else
    struct passwd* pwd = getpwuid(getuid());
    if (pwd) {
        return std::string(pwd->pw_name);
    }
#endif
    return "unknown";
}

std::string SysInfo::GetExecutableFilepath()
{
    static std::string path = GetExecutableFilepathStatic();
    return path;
}

std::string SysInfo::GetExecutableFilepathStatic()
{
#ifdef _WIN32
    char path[MAX_PATH];
    if (GetModuleFileNameA(NULL, path, MAX_PATH)) {
        return std::string(path);
    }
#else
    char path[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", path, PATH_MAX);
    if (count != -1) {
        return std::string(path, count);
    }
#endif
    return "unknown";
}

std::string SysInfo::GetTarget()
{
    static std::string target = GetTargetStatic();
    return target;
}

std::string SysInfo::GetTargetStatic()
{
    std::string identifier;
    // 架构部分
#if defined(__x86_64__) || defined(_M_X64)
    identifier = "x64";
#elif defined(__i386__) || defined(_M_IX86)
    identifier = "i86";
#elif defined(__arm__) || defined(_M_ARM)
    identifier = "ARM";
#elif defined(__aarch64__)
    identifier = "ARM64";
#else
    identifier = "UNK";
#endif

    // 操作系统部分
#if defined(_WIN32)
    identifier += "Win";
#elif defined(__linux__)
    identifier += "Linux";
#else
    identifier += "Unknown";
#endif

    // 位数部分
#if defined(_WIN64) || defined(__x86_64__) || defined(__LP64__)
    identifier += "64";
#else
    identifier += "32";
#endif

    // 编译器部分
#if defined(_MSC_VER)
    identifier += "VS";
    // Visual Studio 版本检测
    if (_MSC_VER == 1600) {
        identifier += "2010";
    }
    else if (_MSC_VER == 1700) {
        identifier += "2012";
    }
    else if (_MSC_VER == 1800) {
        identifier += "2013";
    }
    else if (_MSC_VER == 1900) {
        identifier += "2015";
    }
    else if (_MSC_VER >= 1910 && _MSC_VER <= 1916) {
        identifier += "2017";
    }
    else if (_MSC_VER >= 1920 && _MSC_VER <= 1929) {
        identifier += "2019";
    }
    else if (_MSC_VER >= 1930 && _MSC_VER <= 2000) {
        identifier += "2022";
    }
    else {
        identifier += std::to_string(_MSC_VER);
    }
#elif defined(__GNUC__)
    identifier += "GCC" + std::to_string(__GNUC__) + "." +
        std::to_string(__GNUC_MINOR__) + "." +
        std::to_string(__GNUC_PATCHLEVEL__);
#elif defined(__clang__)
    identifier += "Clang" + std::to_string(__clang_major__) + "." +
        std::to_string(__clang_minor__) + "." +
        std::to_string(__clang_patchlevel__);
#else
    identifier += "UnknownCompiler";
#endif

    return identifier;
}

std::string SysInfo::GetCreationTimestamp()
{
    static std::string Timestamp = GetCreationTimestampStatic();
    return Timestamp;
}

std::string SysInfo::GetCreationTimestampStatic()
{
    static const std::array<const char*, 12> months = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };

    std::istringstream date(__DATE__);
    std::string month_str, day_str, year_str;
    date >> month_str >> day_str >> year_str;

    // 查找月份
    int32_t month = 1;
    for (; month <= 12; ++month) {
        if (months[month - 1] == month_str) break;
    }

    // 构造输出流
    std::ostringstream oss;
    oss << year_str << "-"
        << std::setw(2) << std::setfill('0') << month << "-"
        << std::setw(2) << std::setfill('0') << day_str << " "
        << __TIME__<<"(UTC+8)";

    return oss.str();
}

std::string SysInfo::GetExecutionTimestamp()
{
	static std::string Timestamp = GetExecutionTimestampStatic();
	return Timestamp;
}

std::string SysInfo::GetExecutionTimestampStatic()
{
    static const auto start_time = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(start_time);

    std::ostringstream oss;
    struct tm timeinfo = { };

#ifdef _WIN32
    localtime_s(&timeinfo, &time);
#else
    localtime_r(&time, &timeinfo);
#endif
    oss << std::put_time(&timeinfo, "%Y-%m-%d %H:%M:%S(UTC+8)");
    return oss.str();
}

PropertySeq SysInfo::GetSysInfoProperties()
{
    PropertySeq properties;
    properties.push_back({ "dds.sys_info.hostname", GetHostname() });
	properties.push_back({ "dds.sys_info.process_id", GetProcessID() });
	properties.push_back({ "dds.sys_info.username", GetUsername() });
	properties.push_back({ "dds.sys_info.executable_filepath", GetExecutableFilepath() });
	properties.push_back({ "dds.sys_info.target", GetTarget() });
	properties.push_back({ "dds.sys_info.creation_timestamp", GetCreationTimestamp() });
	properties.push_back({ "dds.sys_info.execution_timestamp", GetExecutionTimestamp() });
    return properties;
}

void ThreadFunction::SetThreadName(const std::string& name)
{
    if (name.empty()) {
        return;
    }
    std::array<char, 16> buffer;
    snprintf(buffer.data(), buffer.size(), "%s", name.data());
    buffer.back() = '\0';  // 确保以null结尾


#ifdef _WIN32
    std::wstringstream stream;
    stream << buffer.data();
    std::wstring wThreadName = stream.str();
    SetThreadDescription(GetCurrentThread(), wThreadName.data());
#else
    // Linux/macOS实现
    pthread_setname_np(pthread_self(), buffer.data());
#endif
}