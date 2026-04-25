#include "fake_channel.h"

namespace test {

    void deliver_packets(
        transfer::TransferSession& from,
        transfer::TransferSession& to,
        uint64_t now_ms,
        ChannelType channel,
        float loss_prob,
        float corrupt_prob,
        std::mt19937& rng,
        ChannelStats& stats)
    {
        auto packets = from.poll_outgoing();
        if (packets.empty()) return;

        std::vector<transfer::Packet> delivered;
        delivered.reserve(packets.size());
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);

        for (auto& pkt : packets) {
            stats.total_packets++;

            if (channel == ChannelType::LOSSY || channel == ChannelType::LOSSY_CORRUPTED) {
                if (dist(rng) < loss_prob) {
                    stats.lost_packets++;
                    continue;
                }
            }

            if (channel == ChannelType::LOSSY_CORRUPTED) {
                if (dist(rng) < corrupt_prob) {
                    if (std::holds_alternative<transfer::DataPacket>(pkt)) {
                        auto& dp = std::get<transfer::DataPacket>(pkt);
                        if (!dp.payload.empty()) {
                            dp.payload[0] ^= 0xFF;
                            stats.corrupted_packets++;
                        }
                    }
                }
            }

            delivered.push_back(std::move(pkt));
        }

        if (!delivered.empty())
            to.feed_incoming(delivered, now_ms);
    }

    void deliver_packets(
        transfer::TransferSession& from,
        transfer::TransferSession& to,
        uint64_t now_ms)
    {
        auto packets = from.poll_outgoing();
        if (!packets.empty())
            to.feed_incoming(packets, now_ms);
    }
}