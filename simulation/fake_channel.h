#pragma once
#include "transfer/session.h"
#include <random>
#include <cstdint>

namespace test {

    enum class ChannelType {
        CLEAN,
        LOSSY,
        LOSSY_CORRUPTED
    };

    struct ChannelStats {
        int total_packets = 0;
        int lost_packets = 0;
        int corrupted_packets = 0;
    };

    // доставка с потерями
    void deliver_packets(
        transfer::TransferSession& from,
        transfer::TransferSession& to,
        uint64_t now_ms,
        ChannelType channel,
        float loss_prob,
        float corrupt_prob,
        std::mt19937& rng,
        ChannelStats& stats);

    // доставка без потерь
    void deliver_packets(
        transfer::TransferSession& from,
        transfer::TransferSession& to,
        uint64_t now_ms);
}