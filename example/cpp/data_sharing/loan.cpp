// Copyright(c)[2025][AGIROS][TravoDDS] is licensed under Mulan PSL v2.
#include "demo.h"
using namespace datasharing_demo;
int main(int argc, char** argv) {
    return runExample(argc, argv, {"loan", false, false, 6},
        [](Session& app, Peer& peer) {
            for (uint64_t sequence = 1; sequence <= 6; ++sequence) {
                // Loan write transfers ownership. Ordinary write copies into a slot.
                app.write(0, sequence, HANDLE_NIL, sequence % 2 != 0);
                peer.expect('K');
            }
        },
        [](Session& app, Peer& peer) {
            for (uint64_t sequence = 1; sequence <= 6; ++sequence) {
                app.consume(0, sequence, sequence == 1);
                peer.send('K');
            }
        });
}
