#ifndef TESTFRAME_H
#define TESTFRAME_H

#include <iostream>
#include <string>
#include <map>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#endif

#define WAIT_AND_SYNC_CONNECTION(conn) \
    ASSERT_TRUE(TestFrame::getInstance().waitForConnection(conn)); \
    EXPECT_TRUE(TestFrame::getInstance().sendData(conn, "wait_for_completion"))

#define DESTORY_CONNECTION(conn) \
    TestFrame::getInstance().destroyConnection(conn)

#define CREATE_AND_SYNC_CONNECTION(conn) \
    ASSERT_TRUE(TestFrame::getInstance().createConnection(conn)); \
    EXPECT_TRUE(TestFrame::getInstance().waitForData(conn, "wait_for_completion"))

#define NOTIFY_NEGATIVE(passive_name) \
    ASSERT_TRUE(TestFrame::getInstance().sendData("active_connection", "sync", passive_name))

class TestFrame {
public:
    enum class Role {
        ACTIVE,
        PASSIVE
    };

    struct Message {
        std::string connectionId;
        std::string key;
        std::string value;
    };

    static TestFrame& getInstance(Role role = Role::ACTIVE);
    ~TestFrame();

    void start(int port); // 服务端监听
    void start(const std::string& address, int port); // 客户端连接
    void stop();

	void setRole(Role r) { role = r; }

    bool createConnection(const std::string& connectionId);
    bool waitForConnection(const std::string& connectionId, int timeoutMs = -1);
    bool sendData(const std::string& connectionId, const std::string& key, const std::string& value = "default");
    bool waitForData(const std::string& connectionId, const std::string& key, int timeoutMs = -1);
    bool waitForData(const std::string& connectionId, const std::string& key, std::string& value, int timeoutMs = -1);
    void destroyConnection(const std::string& connectionId); // 销毁指定连接

private:
    TestFrame(Role role);
    Role role;
    std::atomic<bool> running{false};
    std::thread listenerThread;

    // Socket related
    int listenSocket = -1;
    std::map<std::string, int> connections; // id -> socket
    std::mutex connectionsMutex;

    // Message queues
    std::map<std::string, std::queue<Message>> messageQueues; // key -> messages
    std::mutex messageQueuesMutex;
    std::condition_variable messageCV;

    // Connection waiting
    std::map<std::string, std::condition_variable> connectionCVs;
    std::mutex connectionCVsMutex;

    // For passive mode
    std::string serverAddress;
    int serverPort = 0;

    void initializeSocket();
    void cleanupSocket();
    void closeSocket(int sock);
    void activeListener(int port);
    void handleConnection(const std::string& connectionId, int sock);
};

#endif // TESTFRAME_H