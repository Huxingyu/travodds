// Copyright(c)[2025][AGIROS][TravoDDS] is licensed under Mulan PSL v2.
#include "demo.h"
using namespace datasharing_demo;
int main(int argc, char** argv) {
    return runExample(argc, argv, {"keyed", true, false, 5},
        [](Session& app, Peer& peer) {
            auto a = sample(11, 1), b = sample(22, 2), unknown = sample(33, 0);
            const auto first = app.writer->register_instance(&a);
            const auto second = app.writer->register_instance(&b);
            require(first != HANDLE_NIL && second != HANDLE_NIL && first != second, "register instances failed");
            require(app.writer->lookup_instance(&unknown) == HANDLE_NIL, "Unknown key is registered");
            Sample key{};
            check(app.writer->get_key_value(&key, first), "Writer get_key_value");
            require(key.key == 11, "Writer key snapshot mismatch");
            app.write(11, 1, first); peer.expect('K');
            app.write(22, 2, second); peer.expect('K');
            app.write(11, 3, first, false); peer.expect('K');
            check(app.writer->dispose(nullptr, first), "dispose"); peer.expect('K');
            app.write(11, 4, first); peer.expect('K');
            check(app.writer->unregister_instance(nullptr, second), "unregister"); peer.expect('K');
            require(app.writer->lookup_instance(&b) == HANDLE_NIL, "Unregistered instance remains registered");
            require(app.writer->write(&b, second) == RETCODE_BAD_PARAMETER, "Stale handle accepted");
            require(app.writer->register_instance(&b) == second, "Re-registration changed key identity");
            app.write(22, 5, second); peer.expect('K');
        },
        [](Session& app, Peer& peer) {
            app.consume(11, 1, true); peer.send('K');
            app.consume(22, 2); peer.send('K');
            app.consume(11, 3); peer.send('K');
            app.consume(11, 0, false, false, NOT_ALIVE_DISPOSED_INSTANCE_STATE); peer.send('K');
            auto revived = app.consume(11, 4);
            require(revived.disposed_generation_count == 1, "Disposed generation not advanced"); peer.send('K');
            app.consume(22, 0, false, false, NOT_ALIVE_NO_WRITERS_INSTANCE_STATE); peer.send('K');
            revived = app.consume(22, 5);
            require(revived.no_writers_generation_count == 1, "No-writers generation not advanced"); peer.send('K');
        });
}
