#include <iostream>
#include <csignal>
#include <cstdlib>
#include <functional>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include "testparameter.h"
#include "throughputtest.h"
#include "delaytest.h"


class SignalHandler {
public:
    using Callback = std::function<void()>;

    static SignalHandler& getInstance() {
        static SignalHandler instance;
        return instance;
    }

    // 注册回调函数，当收到 SIGINT 时调用
    void registerCallback(Callback callback) {
        callback_ = callback;
    }

    // 检查是否收到 SIGINT
    bool isSigIntReceived() const {
        return received_sigint_;
    }

    // 重置信号标志（如果需要重新使用）
    void reset() {
        received_sigint_ = false;
        sigint_count_ = 0;
    }

private:
    SignalHandler() : received_sigint_(false), sigint_count_(0) {
        setupHandlers();
    }

    ~SignalHandler() = default;

    SignalHandler(const SignalHandler&) = delete;
    SignalHandler& operator=(const SignalHandler&) = delete;

    void setupHandlers() {
#ifdef _WIN32
        if (!SetConsoleCtrlHandler(consoleCtrlHandler, TRUE)) {
            std::cerr << "Failed to set console control handler." << std::endl;
        }
#else
        struct sigaction sa;
        sa.sa_handler = signalHandler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        sigaction(SIGINT, &sa, nullptr);
#endif
    }

    static void onSignalReceived() {
        SignalHandler& instance = getInstance();

        // 增加计数
        instance.sigint_count_++;

        // 第二次收到信号，直接结束进程
        if (instance.sigint_count_ >= 2) {
            std::cerr << "\nSecond SIGINT received. Forcing exit..." << std::endl;
            std::exit(EXIT_FAILURE);  // 立即退出，不调用析构函数
        }

        // 第一次收到信号
        if (!instance.received_sigint_) {
            instance.received_sigint_ = true;

            // 调用注册的回调函数
            if (instance.callback_) {
                instance.callback_();
            }
        }
    }

#ifdef _WIN32
    static BOOL WINAPI consoleCtrlHandler(DWORD ctrlType) {
        if (ctrlType == CTRL_C_EVENT) {
            onSignalReceived();
            return TRUE;
        }
        return FALSE;
    }
#else
    static void signalHandler(int signum) {
        if (signum == SIGINT) {
            onSignalReceived();
        }
    }
#endif

private:
    volatile sig_atomic_t received_sigint_;
    volatile sig_atomic_t sigint_count_;  // 记录收到 SIGINT 的次数
    Callback callback_;
};


using namespace PerformanceTest;

int main(int argc, char* argv[]) {
    ArgParser parser;

    if (!parser.parse(argc, argv)) {
        parser.printUsage();
        return 1;
    }

    TestParameter param = parser.getParameter();

    TestEntity* test = nullptr;

    if (param.testType == TestType::DELAY)
    {
        test  = new DelayTest(param);
    } else if (param.testType == TestType::THROUGH)
    {
        test = new ThroughputTest(param);
    }
    SignalHandler::getInstance().registerCallback([test]() {
        test->stop();
        });
    test->start_test();


    delete test;

    return 0;
}