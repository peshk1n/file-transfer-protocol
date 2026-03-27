#include "fake_channel.h"

namespace test {

    void deliver(transfer::TransferSession& from, transfer::TransferSession& to, uint64_t now_ms) {
        auto packets = from.poll_outgoing();
        if (!packets.empty())
            to.feed_incoming(packets, now_ms);
    }

}