#pragma once
#include <iostream>
#include <stdint.h>
#include <string>
#include <vector>
#include <map>
#include <sstream>


namespace PerformanceTest {
enum EntityType : uint16_t
{
	SUB = 0,
	PUB
};

enum TestType : uint16_t
{
	DELAY = 0,
	THROUGH
};

struct TestParameter
{
    /* 测试类型 */
    TestType testType;
    /* 发布/订阅  */
    EntityType entityType;
    /* 测试时长 */
	/* 测试数据域 */
	uint32_t domainID;
	/* 测试数据的大小	单位B */
	uint32_t dataSize;
	uint32_t times;
	/* 测试数据数量*/
	uint32_t counts;
    TestParameter() : testType(TestType::DELAY),
        entityType(EntityType::SUB),
        domainID(0),
        dataSize(0),
        times(0),
        counts(0) {}
};

class ArgParser {
private:
    std::vector<std::string> args;
    std::map<std::string, std::string> long_opts;
    std::vector<std::string> short_opts;

    // 参数状态
    bool has_sub = false;
    bool has_pub = false;
    bool has_delay = false;
    bool has_through = false;
    std::string domainID;
    bool has_dataSizeKB = false;
    bool has_dataSizeB = false;
    bool has_timesValue = false;
    bool has_countsValue = false;

    // 存储参数值
    std::string dataSizeKB_value;
    std::string dataSizeB_value;
    std::string timesValue_value;
    std::string countsValue_value;

    // 辅助函数：分割字符串
    std::vector<std::string> split(const std::string& s, char delim) {
        std::vector<std::string> result;
        std::stringstream ss(s);
        std::string item;
        while (std::getline(ss, item, delim)) {
            result.push_back(item);
        }
        return result;
    }

    // 辅助函数：去除首尾空格
    std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(" \t\r\n");
        return str.substr(first, last - first + 1);
    }

    // 解析单个参数
    bool parseLongOption(const std::string& arg, size_t& i) {
        // 格式：--option=value 或 --option value
        std::string opt_name;
        std::string opt_value;

        size_t eq_pos = arg.find('=');
        if (eq_pos != std::string::npos) {
            // --option=value 格式
            opt_name = arg.substr(0, eq_pos);
            opt_value = arg.substr(eq_pos + 1);
        }
        else {
            // --option value 格式
            opt_name = arg;
            if (i + 1 < args.size() && args[i + 1][0] != '-') {
                opt_value = args[++i];
            }
        }

        // 去掉开头的 "--"
        if (opt_name.substr(0, 2) == "--") {
            opt_name = opt_name.substr(2);
        }

        // 处理不同的长选项
        if (opt_name == "domainID") {
            if (opt_value.empty()) return false;
            domainID = opt_value;
        }
        else if (opt_name == "dataSizeKB") {
            has_dataSizeKB = true;
            dataSizeKB_value = opt_value;
        }
        else if (opt_name == "dataSizeB") {
            has_dataSizeB = true;
            dataSizeB_value = opt_value;
        }
        else if (opt_name == "timesValue") {
            has_timesValue = true;
            timesValue_value = opt_value;
        }
        else if (opt_name == "countsValue") {
            has_countsValue = true;
            countsValue_value = opt_value;
        }
        else {
            std::cerr << "Error: Unknown long option --" << opt_name << std::endl;
            return false;
        }

        return true;
    }

    // 解析短选项
    bool parseShortOption(const std::string& arg) {
        if (arg == "-sub") {
            has_sub = true;
        }
        else if (arg == "-pub") {
            has_pub = true;
        }
        else if (arg == "-delay") {
            has_delay = true;
        }
        else if (arg == "-through") {
            has_through = true;
        }
        else {
            std::cerr << "Error: Unknown short option " << arg << std::endl;
            return false;
        }
        return true;
    }

public:
    bool parse(int argc, char* argv[]) {
        // 收集所有参数
        for (int i = 1; i < argc; i++) {
            args.push_back(argv[i]);
        }

        // 解析参数
        for (size_t i = 0; i < args.size(); i++) {
            const std::string& arg = args[i];

            if (arg.substr(0, 2) == "--") {
                // 长选项
                if (!parseLongOption(arg, i)) {
                    return false;
                }
            }
            else if (arg[0] == '-') {
                // 短选项
                if (!parseShortOption(arg)) {
                    return false;
                }
            }
            else {
                std::cerr << "Error: Invalid argument format " << arg << std::endl;
                return false;
            }
        }

        return validate();
    }

    bool validate() {
        // 1. -sub 和 -pub 必须有其一（且互斥）
        if (!has_sub && !has_pub) {
            std::cerr << "Error: Must specify either -sub or -pub" << std::endl;
            return false;
        }

        if (has_sub && has_pub) {
            std::cerr << "Error: -sub and -pub cannot be used together" << std::endl;
            return false;
        }

        // 2. -delay 和 -through 必须有其一
        if (!has_delay && !has_through) {
            std::cerr << "Error: Must specify either -delay or -through" << std::endl;
            return false;
        }

        // 3. --domainID 必须携带
        if (domainID.empty()) {
            std::cerr << "Error: --domainID is required" << std::endl;
            return false;
        }

        // 4. -pub 模式下的附加检查
        if (has_pub) {
            if (!has_dataSizeKB && !has_dataSizeB) {
                std::cerr << "Error: In -pub mode, --dataSizeKB or --dataSizeB is required" << std::endl;
                return false;
            }

            if (!has_timesValue && !has_countsValue) {
                std::cerr << "Error: In -pub mode, --timesValue or --countsValue is required" << std::endl;
                return false;
            }

            // 可选：验证数值有效性
            if (has_dataSizeKB && !isValidNumber(dataSizeKB_value)) {
                std::cerr << "Error: --dataSizeKB must be a valid number" << std::endl;
                return false;
            }

            if (has_dataSizeB && !isValidNumber(dataSizeB_value)) {
                std::cerr << "Error: --dataSizeB must be a valid number" << std::endl;
                return false;
            }
        }

        return true;
    }

    // 验证是否为有效数字
    bool isValidNumber(const std::string& str) {
        if (str.empty()) return false;

        // 允许小数点和负号
        bool has_decimal = false;
        for (size_t i = 0; i < str.length(); i++) {
            char c = str[i];
            if (c == '.' && !has_decimal) {
                has_decimal = true;
                continue;
            }
            if (c == '-' && i == 0) continue;
            if (!std::isdigit(c)) return false;
        }
        return true;
    }

    void printUsage() {
        std::cout << "\nUsage:" << std::endl;
        std::cout << std::endl;
        std::cout << "  Required Options:" << std::endl;
        std::cout << "    -sub or -pub                    (choose one)" << std::endl;
        std::cout << "    -delay or -through              (choose one)" << std::endl;
        std::cout << "    --domainID <ID>                 (required)" << std::endl;
        std::cout << std::endl;
        std::cout << "  Additional for -pub mode:" << std::endl;
        std::cout << "    --dataSizeKB <KB> or --dataSizeB <Bytes>  (choose one)" << std::endl;
        std::cout << "    --timesValue <count> or --countsValue <number> (choose one)" << std::endl;
        std::cout << std::endl;
        std::cout << "Examples:" << std::endl;
        std::cout << "  Valid commands:" << std::endl;
        std::cout << "    program -pub --domainID 123 --dataSizeKB 10 --timesValue 5 -delay" << std::endl;
        std::cout << "    program -sub --domainID 456 -through" << std::endl;
        std::cout << "    program -pub --domainID 789 --dataSizeB 1024 --countsValue 100 -delay" << std::endl;
        std::cout << std::endl;
        std::cout << "  Invalid commands:" << std::endl;
        std::cout << "    program --domainID 789 -delay                    (missing -sub/-pub)" << std::endl;
        std::cout << "    program -pub --domainID 111 --timesValue 3 -through (missing dataSize)" << std::endl;
        std::cout << "    program -pub -sub --domainID 222 --dataSizeKB 10 --timesValue 5 (conflicting options)" << std::endl;
    }

    // Getter 方法（供业务逻辑使用）
    TestParameter getParameter()
    {
        TestParameter ret_value;

        ret_value.testType = has_delay ? TestType::DELAY : TestType::THROUGH;
        ret_value.entityType = has_sub ? EntityType::SUB : EntityType::PUB;
        ret_value.domainID = std::stoul(domainID);

        if (has_dataSizeKB) {
            ret_value.dataSize = std::stoul(dataSizeKB_value) * 1024;
        }
        else if (has_dataSizeB) {
            ret_value.dataSize = std::stoul(dataSizeB_value);
        }
        else {
            ret_value.dataSize = 0;
        }

        if (has_timesValue) {
            ret_value.times = std::stoul(timesValue_value);
        }
        else {
            ret_value.times = -1;
        }

        if (has_countsValue)
        {
            ret_value.counts = std::stoul(countsValue_value);
        }
        else {
            ret_value.counts = -1;
        }
        return ret_value;
    }
};


};
