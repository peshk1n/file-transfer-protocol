#include "transfer/session.h"
#include "fake_channel.h"
#include <cassert>
#include <iostream>
#include <fstream>
#include <string>

using namespace transfer;

void create_test_file(const std::string& path) {
    std::ofstream f(path, std::ios::binary);
    std::string pattern = "1234567890";
    for (int i = 0; i < 2048; ++i) {
        f.write(pattern.c_str(), pattern.size());
    }
    f.close();
}

void test_handshake(const std::string& fname)
{
    TransferSession alice, bob;

    assert(!alice.is_error() && "Sender should not be in error state");
    assert(!alice.is_done() && "Sender should not be done yet");
    assert(!bob.is_error() && "Receiver should not be in error state");
    assert(!bob.is_done() && "Receiver should not be done yet");
    assert((alice.get_progress() == 0.0f) && "Sender progress should be 0");
    assert((bob.get_progress() == 0.0f) && "Receiver progress should be 0");

    alice.init_as_sender(fname);
    bob.init_as_receiver();

    assert(!alice.is_error() && "Sender should not be in error state");
    assert(!alice.is_done() && "Sender should not be done yet");
    assert(!bob.is_error() && "Receiver should not be in error state");
    assert(!bob.is_done() && "Receiver should not be done yet");
    assert((alice.get_progress() == 0.0f) && "Sender progress should be 0");
    assert((bob.get_progress() == 0.0f) && "Receiver progress should be 0");

    test::deliver(alice, bob, 0);
    test::deliver(bob, alice, 0);

    assert(!alice.is_error() && "Sender should not be in error state");
    assert(!alice.is_done() && "Sender should not be done yet");
    assert(!bob.is_error() && "Receiver should not be in error state");
    assert(!bob.is_done() && "Receiver should not be done yet");
}

void test_basic_transfer(const std::string& fname) {
    TransferSession alice, bob;
    alice.init_as_sender(fname);
    bob.init_as_receiver();

    uint64_t time = 0;
    float prev_s = 0.0f;
    float prev_r = 0.0f;

    while (!(alice.is_done() && bob.is_done())) {
        test::deliver(alice, bob, time);
        test::deliver(bob, alice, time);

        alice.tick(time);
        bob.tick(time);

        float s = alice.get_progress();
        float r = bob.get_progress();

        assert((s >= prev_s) && "Sender progress decreases");
        assert((r >= prev_r) && "Receiver progress decreases");

        assert((std::abs(s - r) < 0.5f) && "Sender and Receiver progress should be equal");

        prev_s = s;
        prev_r = r;

        time += 100;
        assert(!alice.is_error() && "Sender error");
        assert(!bob.is_error() && "Receiver error");
        assert(time < 60000 && "Transfer timed out");
    }
    assert((alice.get_progress() > 0.99f) && "Sender progress should be 1");
    assert((bob.get_progress() > 0.99f) && "Receiver progress should be 1");

    assert(alice.is_done() && "Sender must be done at the end");
    assert(bob.is_done() && "Receiver must be done at the end");
}


void test_is_done_consistency(const std::string& fname)
{
    TransferSession alice, bob;
    alice.init_as_sender(fname);
    bob.init_as_receiver();

    uint64_t time = 0;

    while (time < 60000)
    {
        test::deliver(alice, bob, time);
        test::deliver(bob, alice, time);

        alice.tick(time);
        bob.tick(time);

        if (alice.is_done())
        {
            for (int i = 0; i < 10; i++)
            {
                test::deliver(alice, bob, time);
                test::deliver(bob, alice, time);
                alice.tick(time);
                bob.tick(time);
                time += 100;
            }

            assert(bob.is_done() && "Receiver stuck after Sender finished");
            return;
        }

        time += 100;
    }

    assert(false && "Timeout");
}


void test_is_error_invalid_start()
{
    TransferSession bob;
    bob.init_as_receiver();

    StartPacket bad;
    bad.file_name = "test";
    bad.file_size = 0;
    bad.chunk_size = 0;   
    bad.total_chunks = 0;

    bob.feed_incoming({ bad }, 0);

    assert(bob.is_error());
}

void test_ignore_unexpected_ack(const std::string& fname) {
    TransferSession alice;
    alice.init_as_sender(fname);

    transfer::AckPacket fake_ack;
    fake_ack.ack_id = 1;
    alice.feed_incoming({ fake_ack }, 0);

    assert(alice.get_progress() == 0.0f && "Sender must ignore ACK before StartAck");
    assert(!alice.is_error() && "Sender should not crash from unexpected ACK");
}

void test_is_error_sticky(const std::string& fname)
{
    TransferSession alice;
    alice.init_as_receiver();
    StartPacket bad{};
    bad.chunk_size = 0;

    alice.feed_incoming({ bad }, 0);
    assert(alice.is_error());

    float progress_at_error = alice.get_progress();

    alice.feed_incoming({}, 100);

    assert(alice.is_error() && "State must remain ERROR");
    assert(alice.get_progress() == progress_at_error && "Progress must not change after error");

}


void test_ignore_damaged_data(const std::string& fname) {
    TransferSession alice, bob;

    alice.init_as_sender(fname);
    bob.init_as_receiver();

    uint64_t time = 0;
    bool damaged = false;
    while (!(alice.is_done() && bob.is_done())) {
        auto p1 = alice.poll_outgoing();
        if (!damaged && alice.get_progress() > 0.3f && !p1.empty()) {
            for (auto& pkt : p1) {
                if (std::holds_alternative<DataPacket>(pkt)) {
                    auto& dp = std::get<DataPacket>(pkt);
                    if (!dp.payload.empty()) {
                        dp.payload[0] ^= 0xFF; 
                    }
                }
            }
            damaged = true;
        }

        bob.feed_incoming(p1, time);

        auto p2 = bob.poll_outgoing();
        alice.feed_incoming(p2, time);

        alice.tick(time);
        bob.tick(time);

        time += 100;
        assert(time < 60000 && "Transfer timed out");
    }

    assert(alice.is_done() && "Sender must be done");
    assert(bob.is_done() && "Receiver must be done");
    assert(bob.get_progress() > 0.99f && "Receiver progress should be 1");
    assert(damaged && "damage was never met");
}

void test_packet_loss(const std::string& fname) {
    TransferSession alice, bob;
    alice.init_as_sender(fname);
    bob.init_as_receiver();

    uint64_t time = 0;
    bool loss = false;

    while (!(alice.is_done() && bob.is_done())) {
        auto p1 = alice.poll_outgoing();
        if (!loss && alice.get_progress() > 0.3f && !p1.empty()) {
            loss = true;
        }
        else {
            bob.feed_incoming(p1, time);
        }
        auto p2 = bob.poll_outgoing();
        alice.feed_incoming(p2, time);

        alice.tick(time);
        bob.tick(time);

        time += 100;
        assert(time < 60000 && "Transfer timed out");
    }

    assert(alice.is_done() && "Sender must be done");
    assert(bob.is_done() && "Receiver must be done");
    assert(loss && "packet loss not met");
    assert(bob.get_progress() > 0.99f && "Receiver progress should be 1");
}



int main() {
   
    //std::string path = std::string(TEST_DATA_DIR) + "/test1.txt";
    //create_test_file(path);

    std::string path = std::string(TEST_DATA_DIR) + "/test.txt";

    test_handshake(path);
    std::cout << "test_handshake PASSED\n";

    test_basic_transfer(path);
    std::cout << "test_basic_transfer PASSED\n";

    test_is_done_consistency(path);
    std::cout << "test_is_done_consistency PASSED\n";

    test_ignore_unexpected_ack(path);
    std::cout << "test_ignore_unexpected_ack PASSED\n";

    test_is_error_sticky(path);
    std::cout << "test_is_error_sticky PASSED\n";

    test_is_error_invalid_start();
    std::cout << "test_is_error_invalid_start PASSED\n";
    
    test_ignore_damaged_data(path);
    std::cout << "test_ignore_damaged_data PASSED\n";

    test_packet_loss(path);
    std::cout << "test_ignore_damaged_data PASSED\n";


    return 0;
}