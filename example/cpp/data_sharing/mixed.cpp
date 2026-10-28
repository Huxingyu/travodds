// Copyright(c)[2025][AGIROS][TravoDDS] is licensed under Mulan PSL v2.
#include "demo.h"
using namespace datasharing_demo;
int main(int argc, char** argv) {
    return runExample(argc, argv, {"mixed", false, true, 6},
        [](Session& app, Peer& peer) {
            for (uint64_t sequence = 1; sequence <= 6; ++sequence) {
                // One native Reader and one OFF Reader in the same participant.
                // This generates exactly one CDR payload per application write.
                app.write(0, sequence); peer.expect('K');
            }
        },
        [](Session& app, Peer& peer) {
            for (uint64_t sequence = 1; sequence <= 6; ++sequence) {
                // consume verifies borrowed native storage and owned CDR data.
                app.consume(0, sequence, sequence == 1); peer.send('K');
            }
        });
}
