#include "fake_channel.h"

namespace test {

    void deliver(transfer::TransferSession& from, transfer::TransferSession& to) {
        auto packets = from.poll_outgoing();
        if (!packets.empty())
            to.feed_incoming(packets);
    }

}