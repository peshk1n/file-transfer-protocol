#include "transfer/session.h"
#include "simulation.h"
#include "fake_channel.h"
#include <cassert>
#include <iostream>
#include <fstream>
#include <string>
#include <cmath>

using namespace transfer;

static const std::string TEST_FILE = std::string(TEST_DATA_DIR) + "/test.txt";
static const std::string SAVE_DIR = ".";

void create_test_file(const std::string& path) {
    std::ofstream f(path, std::ios::binary);
    std::string pattern = "1234567890";
    for (int i = 0; i < 102400; ++i)
        f.write(pattern.c_str(), pattern.size());
}

// ---- helpers ----

static test::SimulationConfig make_config(test::ChannelType channel = test::ChannelType::CLEAN) {
    test::SimulationConfig cfg;
    cfg.file_path = TEST_FILE;
    cfg.save_dir = SAVE_DIR;
    cfg.channel = channel;
    return cfg;
}

// ---- tests ----

void test_handshake() {
    TransferSession alice;
    TransferSession bob(SAVE_DIR);

    assert(!alice.is_error() && "Sender should not be in error state");
    assert(!alice.is_done() && "Sender should not be done yet");
    assert(!bob.is_error() && "Receiver should not be in error state");
    assert(!bob.is_done() && "Receiver should not be done yet");
    assert(alice.get_progress() == 0.0f && "Sender progress should be 0");
    assert(bob.get_progress() == 0.0f && "Receiver progress should be 0");

    alice.init_as_sender(TEST_FILE, 4096, 4, 0);
    bob.init_as_receiver();

    assert(!alice.is_error() && "Sender should not be in error state");
    assert(!alice.is_done() && "Sender should not be done yet");
    assert(!bob.is_error() && "Receiver should not be in error state");
    assert(!bob.is_done() && "Receiver should not be done yet");
    assert(alice.get_progress() == 0.0f && "Sender progress should be 0");
    assert(bob.get_progress() == 0.0f && "Receiver progress should be 0");

    test::deliver_packets(alice, bob, 0);
    test::deliver_packets(bob, alice, 0);

    assert(!alice.is_error() && "Sender should not be in error state after handshake");
    assert(!alice.is_done() && "Sender should not be done after handshake");
    assert(!bob.is_error() && "Receiver should not be in error state after handshake");
    assert(!bob.is_done() && "Receiver should not be done after handshake");
}

void test_basic_transfer() {
    test::Simulation sim(make_config());

    float prev_s = 0.0f, prev_r = 0.0f;

    while (sim.step()) {
        auto r = sim.get_result();

        assert(r.sender_progress >= prev_s && "Sender progress decreases");
        assert(r.receiver_progress >= prev_r && "Receiver progress decreases");

        prev_s = r.sender_progress;
        prev_r = r.receiver_progress;

        assert(!r.is_error && "Error during transfer");
        assert(r.elapsed_ms < 60000 && "Transfer timed out");
    }

    auto r = sim.get_result();
    assert(r.sender_progress > 0.99f && "Sender progress should be 1");
    assert(r.receiver_progress > 0.99f && "Receiver progress should be 1");
    assert(r.is_done && "Must be done at the end");
}

void test_is_done_consistency() {
    test::Simulation sim(make_config());

    while (true) {
        auto r = sim.get_result();
        assert(r.elapsed_ms < 60000 && "Timeout");

        if (r.is_done) {
            for (int i = 0; i < 10; i++) sim.step();
            auto r2 = sim.get_result();
            assert(r2.is_done && "Done state lost after extra steps");
            return;
        }

        sim.step();
    }
}

void test_is_error_invalid_start() {
    TransferSession bob(SAVE_DIR);
    bob.init_as_receiver();

    StartPacket bad;
    bad.file_name = "test";
    bad.file_size = 0;
    bad.chunk_size = 0;
    bad.total_chunks = 0;
    bob.feed_incoming({ bad }, 0);

    assert(bob.is_error() && "Receiver must be in error state after invalid START");
}

void test_ignore_unexpected_ack() {
    TransferSession alice;
    alice.init_as_sender(TEST_FILE, 4096, 4, 0);

    AckPacket fake_ack;
    fake_ack.ack_id = 1;
    alice.feed_incoming({ fake_ack }, 0);

    assert(alice.get_progress() == 0.0f && "Sender must ignore ACK before StartAck");
    assert(!alice.is_error() && "Sender should not crash from unexpected ACK");
}

void test_is_error_sticky() {
    TransferSession alice(SAVE_DIR);
    alice.init_as_receiver();

    StartPacket bad{};
    bad.chunk_size = 0;
    alice.feed_incoming({ bad }, 0);
    assert(alice.is_error() && "Must be in error state");

    float progress_at_error = alice.get_progress();
    alice.feed_incoming({}, 100);

    assert(alice.is_error() && "State must remain ERROR");
    assert(alice.get_progress() == progress_at_error && "Progress must not change after error");
}

void test_packet_loss() {
    test::Simulation sim(make_config(test::ChannelType::LOSSY));

    while (sim.step()) {
        auto r = sim.get_result();
        assert(!r.is_error && "Error during transfer");
        assert(r.elapsed_ms < 60000 && "Transfer timed out");
    }

    auto r = sim.get_result();
    assert(r.is_done && "Sender must be done");
    assert(r.lost_packets > 0 && "No packets were lost");
    assert(r.receiver_progress > 0.99f && "Receiver progress should be 1");
}

void test_ignore_damaged_data() {
    test::Simulation sim(make_config(test::ChannelType::LOSSY_CORRUPTED));

    while (sim.step()) {
        auto r = sim.get_result();
        assert(!r.is_error && "Error during transfer");
        assert(r.elapsed_ms < 60000 && "Transfer timed out");
    }

    auto r = sim.get_result();
    assert(r.is_done && "Must be done");
    assert(r.corrupted_packets > 0 && "No packets were corrupted");
    assert(r.receiver_progress > 0.99f && "Receiver progress should be 1");
}

int main() {
    create_test_file(TEST_FILE);

    test_handshake();
    std::cout << "test_handshake PASSED\n";

    test_basic_transfer();
    std::cout << "test_basic_transfer PASSED\n";

    test_is_done_consistency();
    std::cout << "test_is_done_consistency PASSED\n";

    test_ignore_unexpected_ack();
    std::cout << "test_ignore_unexpected_ack PASSED\n";

    test_is_error_sticky();
    std::cout << "test_is_error_sticky PASSED\n";

    test_is_error_invalid_start();
    std::cout << "test_is_error_invalid_start PASSED\n";

    test_packet_loss();
    std::cout << "test_packet_loss PASSED\n";

    test_ignore_damaged_data();
    std::cout << "test_ignore_damaged_data PASSED\n";

    return 0;
}