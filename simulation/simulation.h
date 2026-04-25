#pragma once
#include "fake_channel.h"
#include "transfer/session.h"
#include <string>
#include <random>
#include <cstdint>

namespace test {

    struct SimulationConfig {
        float loss_prob = 0.1f;
        float corrupt_prob = 0.05f;
        uint32_t chunk_size = 4096;
        uint32_t window_size = 4;
        uint32_t rng_seed = 1782;
        uint64_t timeout_ms = 3000;
        ChannelType channel = ChannelType::CLEAN;
        std::string file_path;
        std::string save_dir;
    };

    struct SimulationResult {
        float sender_progress = 0.0f;
        float receiver_progress = 0.0f;
        int lost_packets = 0;
        int corrupted_packets = 0;
        int total_packets = 0;
        uint64_t elapsed_ms = 0;
        bool is_done = false;
        bool is_error = false;
    };

    class Simulation {
    public:
        explicit Simulation(const SimulationConfig& config);
        bool step();
        SimulationResult get_result() const;
        void reset(const SimulationConfig& config);

    private:
        SimulationConfig config;
        std::mt19937 rng;
        transfer::TransferSession alice;
        transfer::TransferSession bob;
        ChannelStats stats;
        uint64_t time_ms = 0;
    };

}