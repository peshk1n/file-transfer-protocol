#include "simulation.h"

namespace test {

    Simulation::Simulation(const SimulationConfig& config) {
        reset(config);
    }

    void Simulation::reset(const SimulationConfig& config) {
        this->config = config;
        rng = std::mt19937(config.rng_seed);
        stats = ChannelStats{};
        time_ms = 0;

        alice = transfer::TransferSession();
        bob = transfer::TransferSession(config.save_dir);
        alice.init_as_sender(config.file_path, config.chunk_size, config.window_size, time_ms);
        bob.init_as_receiver();
    }

    bool Simulation::step() {
        if (alice.is_done() && bob.is_done()) return false;
        if (alice.is_error() || bob.is_error()) return false;

        deliver_packets(alice, bob, time_ms, config.channel, config.loss_prob, config.corrupt_prob, rng, stats);
        deliver_packets(bob, alice, time_ms, config.channel, config.loss_prob, config.corrupt_prob, rng, stats);

        alice.tick(time_ms);
        bob.tick(time_ms);

        time_ms += 100;
        return true;
    }

    SimulationResult Simulation::get_result() const {
        SimulationResult r;
        r.sender_progress = alice.get_progress();
        r.receiver_progress = bob.get_progress();
        r.lost_packets = stats.lost_packets;
        r.corrupted_packets = stats.corrupted_packets;
        r.total_packets = stats.total_packets;
        r.elapsed_ms = time_ms;
        r.is_done = alice.is_done() && bob.is_done();
        r.is_error = alice.is_error() || bob.is_error();
        return r;
    }
}