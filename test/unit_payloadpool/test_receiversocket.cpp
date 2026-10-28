#include "gtest/gtest.h"
#include "rtps/transport/recivesocket.h"
#include "rtps/transport/messagetransmittertcp.h"
#include <future>

USING_TRAVODDS_NAMESPACE
namespace {
class TestReceiverSocket : public ReciverSocket {
public:
    TestReceiverSocket() : ReciverSocket("socket.test") {}
    void deleteInCallback(std::shared_ptr<TestReceiverSocket> self,
            std::shared_future<void> gate, std::shared_ptr<std::promise<void>> completed) {
        asio::post(ioContext_, [self = std::move(self), gate, completed]() mutable {
            gate.wait();
            self.reset();
            // The socket has gone, but io_context::run() still has to finish
            // the current handler and its internal completion bookkeeping.
            completed->set_value();
        });
    }
};
}

TEST(ReceiverSocketLifetime, LastReferenceCanBeReleasedInsideItsOwnCallback) {
    for (unsigned round = 0; round < 25; ++round) {
        auto socket = std::make_shared<TestReceiverSocket>();
        std::weak_ptr<TestReceiverSocket> weak = socket;
        std::promise<void> gate;
        auto completed = std::make_shared<std::promise<void>>();
        auto finished = completed->get_future();
        socket->deleteInCallback(socket, gate.get_future().share(), completed);
        socket.reset();
        ASSERT_FALSE(weak.expired());
        gate.set_value();
        ASSERT_EQ(finished.wait_for(std::chrono::seconds(2)), std::future_status::ready);
        EXPECT_TRUE(weak.expired());
    }
}

TEST(ReceiverSocketLifetime, ExternalDestructionStopsAndJoinsIdleWorker) {
    auto socket = std::make_shared<TestReceiverSocket>();
    std::weak_ptr<TestReceiverSocket> weak = socket;
    socket.reset();
    EXPECT_TRUE(weak.expired());
}


TEST(TcpTransmitterLifetime, FailedConnectSetupCanBeStoppedAndDestroyed) {
    asio::io_context context;
    asio::ip::tcp::acceptor occupied(context, {asio::ip::tcp::v4(), 0});
    Locator_t source{}, destination{};
    source.kind = destination.kind = LOCATOR_KIND_TCPv4;
    source.address[12] = destination.address[12] = 127;
    source.address[15] = destination.address[15] = 1;
    source.port = destination.port = occupied.local_endpoint().port();
    for (int i = 0; i < 2; ++i) {
        MessageTransmitterTCPv4 transmitter(nullptr);
        // Setup fails when binding this socket to the occupied local port.
        EXPECT_EQ(transmitter.SendAddrAdd(source, destination), RETCODE_OK);
        if (i == 0) {
            transmitter.SetStop();
            EXPECT_TRUE(transmitter.IsStop());
        }
        // Also exercise destruction directly after failed setup.
    }
}

namespace {
class PausingTcpTransmitter : public MessageTransmitterTCP {
public:
    PausingTcpTransmitter() : MessageTransmitterTCP(nullptr), gate_(release_.get_future()) { kind_ = LOCATOR_KIND_TCPv4; }
    std::future<void> started() { return entered_.get_future(); }
    void release() { release_.set_value(); }
private:
    asio::ip::tcp::endpoint GetEndpoint(const AsioLocator& locator, bool receive) override {
        return {receive ? asio::ip::address_v4::any() : locator.GetAddress<asio::ip::address_v4>(), uint16_t(locator.port)};
    }
    asio::ip::tcp::endpoint GetReciveEndpoint(uint32_t port) override { return {asio::ip::tcp::v4(), uint16_t(port)}; }
    std::shared_ptr<asio::ip::tcp::socket> GetSocketInit() override {
        entered_.set_value();
        gate_.wait();
        // Cancellation already happened while the pending slot had no socket.
        // Socket creation is enough; AsyncConnect must stop before scheduling I/O.
        return std::make_shared<asio::ip::tcp::socket>(context_, asio::ip::tcp::v4());
    }
    asio::io_context context_;
    std::promise<void> entered_, release_;
    std::future<void> gate_;
};
}
TEST(TcpTransmitterLifetime, StopDuringPendingSocketConstructionDoesNotDereferenceNull) {
    PausingTcpTransmitter transmitter;
    auto started = transmitter.started();
    Locator_t source{}, destination{};
    source.kind = destination.kind = LOCATOR_KIND_TCPv4;
    destination.port = 1;
    destination.address[12] = 127; destination.address[15] = 1;
    std::thread connect([&] { EXPECT_EQ(transmitter.SendAddrAdd(source, destination), RETCODE_OK); });
    const auto ready = started.wait_for(std::chrono::seconds(2));
    EXPECT_EQ(ready, std::future_status::ready);
    transmitter.SetStop();
    transmitter.release();
    connect.join();
    EXPECT_TRUE(transmitter.IsStop());
}
