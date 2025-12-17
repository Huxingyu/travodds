#include "TestFrame.h"

TestFrame& TestFrame::getInstance(Role role) {
    static TestFrame instance(role);
    return instance;
}

TestFrame::TestFrame(Role role) : role(role) {
    initializeSocket();
}

TestFrame::~TestFrame() {
    stop();
    cleanupSocket();
}

void TestFrame::initializeSocket() {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed with error: " << WSAGetLastError() << std::endl;
        throw std::runtime_error("WSAStartup failed");
    }
#endif
}

void TestFrame::cleanupSocket() {
#ifdef _WIN32
    WSACleanup();
#endif
}

void TestFrame::closeSocket(int sock) {
    if (sock != -1) {
#ifdef _WIN32
        closesocket(sock);
#else
        close(sock);
#endif
    }
}

void TestFrame::start(int port) {
    if (running) return;

    running = true;
    if (role == Role::PASSIVE) {
        listenerThread = std::thread(&TestFrame::activeListener, this, port); // 服务端监听
    }
}

void TestFrame::start(const std::string& address, int port) {
    if (running) return;

    running = true;
    if (role == Role::ACTIVE) {
        serverAddress = address;
        serverPort = port;
        createConnection("active_connection"); // 客户端连接到服务端
    }
}

void TestFrame::stop() {
    running = false;


	if (-1 != listenSocket)
	{
		closesocket(listenSocket);
		listenSocket = -1;
	}
    if (listenerThread.joinable()) {
        listenerThread.join();
    }

	//listenerThread.

    {
        std::lock_guard<std::mutex> lock(connectionsMutex);
        for (auto& conn : connections) {
            closeSocket(conn.second);
        }
        connections.clear();
    }
}

void TestFrame::activeListener(int port) {
    struct sockaddr_in serverAddr;
    listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocket == -1) {
#ifdef _WIN32
        std::cerr << "Failed to create socket, error: " << WSAGetLastError() << std::endl;
#else
        std::cerr << "Failed to create socket, error: " << errno << std::endl;
#endif
        return;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (bind(listenSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
#ifdef _WIN32
        std::cerr << "Bind failed, error: " << WSAGetLastError() << std::endl;
#else
        std::cerr << "Bind failed, error: " << errno << std::endl;
#endif
        closeSocket(listenSocket);
        return;
    }

    if (listen(listenSocket, 5) < 0) {
#ifdef _WIN32
        std::cerr << "Listen failed, error: " << WSAGetLastError() << std::endl;
#else
        std::cerr << "Listen failed, error: " << errno << std::endl;
#endif
        closeSocket(listenSocket);
        return;
    }

    while (running) {
        struct sockaddr_in clientAddr;
#ifdef _WIN32
        int clientLen = sizeof(clientAddr);
#else
        socklen_t clientLen = sizeof(clientAddr);
#endif

        int clientSocket = accept(listenSocket, (struct sockaddr*)&clientAddr, &clientLen);
        if (clientSocket == -1) {
            if (!running) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        // First receive the connection ID from client
        char idBuffer[256] = {0};
        int bytesReceived = recv(clientSocket, idBuffer, sizeof(idBuffer) - 1, 0);
        if (bytesReceived <= 0) {
            closeSocket(clientSocket);
            continue;
        }

        std::string connectionId(idBuffer);

        {
            std::lock_guard<std::mutex> lock(connectionsMutex);
            connections[connectionId] = clientSocket;
        }

        // Notify anyone waiting for this connection
        {
            std::lock_guard<std::mutex> lock(connectionCVsMutex);
            if (connectionCVs.find(connectionId) != connectionCVs.end()) {
                connectionCVs[connectionId].notify_all();
            }
        }

        // Start a thread to handle this connection
        std::thread([this, connectionId, clientSocket]() {
            handleConnection(connectionId, clientSocket);
        }).detach();
    }

    closeSocket(listenSocket);
}

void TestFrame::handleConnection(const std::string& connectionId, int sock) {
    while (running) {
        // Simple protocol: first 4 bytes is key length, then key, then 4 bytes value length, then value
        uint32_t keyLen = 0;
        int bytesReceived = recv(sock, (char*)&keyLen, sizeof(keyLen), 0);
        if (bytesReceived <= 0) {
            break;
        }

        keyLen = ntohl(keyLen);
        std::string key(keyLen, '\0');
        bytesReceived = recv(sock, &key[0], keyLen, 0);
        if (bytesReceived <= 0) {
            break;
        }

        uint32_t valueLen = 0;
        bytesReceived = recv(sock, (char*)&valueLen, sizeof(valueLen), 0);
        if (bytesReceived <= 0) {
            break;
        }

        valueLen = ntohl(valueLen);
        std::string value(valueLen, '\0');
        bytesReceived = recv(sock, &value[0], valueLen, 0);
        if (bytesReceived <= 0) {
            break;
        }

        {
            std::lock_guard<std::mutex> lock(messageQueuesMutex);
            messageQueues[key].push({connectionId, key, value});
        }
        messageCV.notify_all();
    }

    // Connection closed
    {
        std::lock_guard<std::mutex> lock(connectionsMutex);
		//if(0 != connections.count(connectionId))
			connections.erase(connectionId);
    }
    closeSocket(sock);
}

bool TestFrame::createConnection(const std::string& connectionId) {
    if (role != Role::ACTIVE) return false;

    std::cout << "Creating connection with ID: " << connectionId << std::endl;
    //std::cout << "Server Address: " << serverAddress << ", Server Port: " << serverPort << std::endl; // 添加日志

    struct sockaddr_in serverAddr;
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
#ifdef _WIN32
        std::cerr << "Failed to create socket, error: " << WSAGetLastError() << std::endl;
#else
        std::cerr << "Failed to create socket, error: " << errno << std::endl;
#endif
        return false;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(serverPort);
#ifdef _WIN32
    if (inet_pton(AF_INET, serverAddress.c_str(), &serverAddr.sin_addr) <= 0) { // 检查地址解析
        std::cerr << "Invalid server address: " << serverAddress << std::endl;
        closeSocket(sock);
        return false;
    }
#else
    if (inet_aton(serverAddress.c_str(), &serverAddr.sin_addr) == 0) { // 检查地址解析
        std::cerr << "Invalid server address: " << serverAddress << std::endl;
        closeSocket(sock);
        return false;
    }
#endif

    // 多次尝试连接
    const int maxRetries = 5;
    const int retryIntervalMs = 1000; // 每次重试间隔 1 秒
    int attempt = 0;
    while (attempt < maxRetries) {
        if (connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == 0) {
            std::cout << "Connection established on attempt " << (attempt + 1) << std::endl;
            break;
        }
#ifdef _WIN32
        std::cerr << "Connect failed on attempt " << (attempt + 1) << ", error: " << WSAGetLastError() << std::endl;
#else
        std::cerr << "Connect failed on attempt " << (attempt + 1) << ", error: " << errno << std::endl;
#endif
        std::this_thread::sleep_for(std::chrono::milliseconds(retryIntervalMs));
        attempt++;
    }

    if (attempt == maxRetries) {
        std::cerr << "Failed to connect after " << maxRetries << " attempts." << std::endl;
        closeSocket(sock);
        return false;
    }

    // Send connection ID first
    if (send(sock, connectionId.c_str(), connectionId.size(), 0) <= 0) {
#ifdef _WIN32
        std::cerr << "Send failed (connection_id), error: " << WSAGetLastError() << std::endl;
#else
        std::cerr << "Send failed (connection_id), error: " << errno << std::endl;
#endif
        closeSocket(sock);
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(connectionsMutex);
        connections[connectionId] = sock;
    }

    // Start a thread to handle this connection
    std::thread([this, connectionId, sock]() {
        handleConnection(connectionId, sock);
    }).detach();

    //std::cout << "Connection established with ID: " << connectionId << std::endl;

    return true;
}

bool TestFrame::waitForConnection(const std::string& connectionId, int timeoutMs) {
    if (role != Role::PASSIVE) return false;

    std::unique_lock<std::mutex> lock(connectionCVsMutex);
    if (connections.find(connectionId) != connections.end()) {
        std::cout << "Connection already established with ID: " << connectionId << std::endl;
        return true;
    }

    std::cout << "Waiting for connection with ID: " << connectionId << std::endl;

    auto& cv = connectionCVs[connectionId];
    if (timeoutMs < 0) {
        cv.wait(lock, [this, connectionId] {
            return connections.find(connectionId) != connections.end() || !running;
        });
    } else {
        cv.wait_for(lock, std::chrono::milliseconds(timeoutMs), [this, connectionId] {
            return connections.find(connectionId) != connections.end() || !running;
        });
    }

    std::cout << "Connection wait completed for ID: " << connectionId << std::endl;

    connectionCVs.erase(connectionId);
    return connections.find(connectionId) != connections.end();
}

bool TestFrame::sendData(const std::string& connectionId, const std::string& key, const std::string& value) {
    std::string _value = value == "default" ? value: "custom";
    std::cout << "Sending data to connection ID: " << connectionId << ", key: " << key << ", value: " << _value << std::endl;

    std::lock_guard<std::mutex> lock(connectionsMutex);
    auto it = connections.find(connectionId);
    if (it == connections.end()) {
        std::cout << "connections do not find" << std::endl;
        return false;
    }

    int sock = it->second;

    // Simple protocol: first 4 bytes is key length, then key, then 4 bytes value length, then value
    uint32_t keyLen = htonl(key.size());
    uint32_t valueLen = htonl(value.size());

    if (send(sock, (char*)&keyLen, sizeof(keyLen), 0) <= 0) {
#ifdef _WIN32
        std::cerr << "Send failed (key length), error: " << WSAGetLastError() << std::endl;
#else
        std::cerr << "Send failed (key length), error: " << errno << std::endl;
#endif
        return false;
    }
    if (send(sock, key.c_str(), key.size(), 0) <= 0) {
#ifdef _WIN32
        std::cerr << "Send failed (key), error: " << WSAGetLastError() << std::endl;
#else
        std::cerr << "Send failed (key), error: " << errno << std::endl;
#endif
        return false;
    }
    if (send(sock, (char*)&valueLen, sizeof(valueLen), 0) <= 0) {
#ifdef _WIN32
        std::cerr << "Send failed (value length), error: " << WSAGetLastError() << std::endl;
#else
        std::cerr << "Send failed (value length), error: " << errno << std::endl;
#endif
        return false;
    }
    if (send(sock, value.c_str(), value.size(), 0) <= 0) {
#ifdef _WIN32
        std::cerr << "Send failed (value), error: " << WSAGetLastError() << std::endl;
#else
        std::cerr << "Send failed (value), error: " << errno << std::endl;
#endif
        return false;
    }

    return true;
}

bool TestFrame::waitForData(const std::string& connectionId, const std::string& key, int timeoutMs) {
    std::string value;
    return waitForData(connectionId, key, value, timeoutMs);
}

bool TestFrame::waitForData(const std::string& connectionId, const std::string& key, std::string& value, int timeoutMs) {

    std::unique_lock<std::mutex> lock(messageQueuesMutex);

    // 检查是否已经有符合条件的消息
    auto checkMessage = [this, &connectionId, &key]() -> bool {
        if (messageQueues.find(key) != messageQueues.end() && !messageQueues[key].empty()) {
            // 检查队列中是否有来自指定连接的消息
            auto& queue = messageQueues[key];
            for (size_t i = 0; i < queue.size(); ++i) {
                if (queue.front().connectionId == connectionId) {
                    return true;
                }
                // 将不符合条件的消息移到队列末尾
                queue.push(queue.front());
                queue.pop();
            }
        }
        return false;
    };

    if (checkMessage()) {
        value = messageQueues[key].front().value;
        messageQueues[key].pop();
        std::string _value = value == "default" ? value : "custom";
        std::cout << "Data received from connection ID: " << connectionId << ", key: " << key << ", value: " << _value << std::endl;
        return true;
    }

    std::cout << "Waiting for data from connection ID: " << connectionId << ", key: " << key << std::endl;

    if (timeoutMs < 0) {
        messageCV.wait(lock, [this, &checkMessage] {
            return checkMessage() || !running;
        });
    } else {
        messageCV.wait_for(lock, std::chrono::milliseconds(timeoutMs), [this, &checkMessage] {
            return checkMessage() || !running;
        });
    }

    if (checkMessage()) {
        value = messageQueues[key].front().value;
        messageQueues[key].pop();
        return true;
    }

    return false;
}

void TestFrame::destroyConnection(const std::string& connectionId) {
    std::lock_guard<std::mutex> lock(connectionsMutex);
    auto it = connections.find(connectionId);
    if (it != connections.end()) {
        closeSocket(it->second); // 关闭 socket
        connections.erase(it);  // 移除连接
        std::cout << "Connection with ID: " << connectionId << " has been destroyed." << std::endl;
    } else {
        std::cerr << "Connection with ID: " << connectionId << " not found." << std::endl;
    }
}