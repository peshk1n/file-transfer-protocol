#include "transfer/session.h"
#include "fake_channel.h"
#include <cassert>
#include <iostream>

using namespace transfer;


void test_handshake(const std::string& fname)
{
    TransferSession alice, bob;
    alice.init_as_sender(fname);
    bob.init_as_receiver();

    test::deliver(alice, bob);
    test::deliver(bob, alice);

    assert(!alice.is_error() && "Sender should not be in error state");
    assert(!alice.is_done() && "Sender should not be done yet");
    assert(!bob.is_error() && "Receiver should not be in error state");
    assert(!bob.is_done() && "Receiver should not be done yet");
}

void test_basic_transfer(const std::string& fname)
{
    TransferSession alice, bob;
    alice.init_as_sender(fname);
    bob.init_as_receiver();

    uint64_t time = 0;
    while (!(alice.is_done() && bob.is_done())) {
        test::deliver(alice, bob);
        test::deliver(bob, alice);
        alice.tick(time);
        bob.tick(time);
        time += 100;

        assert(!alice.is_error() && "Sender error");
        assert(!bob.is_error() && "Receiver error");
        assert(time < 60000 && "Transfer timed out"); 
    }

    assert(alice.is_done());
    assert(bob.is_done());
}

int main() {
    test_handshake("data/test.txt");
    std::cout << "test_handshake PASSED\n";

    test_basic_transfer("data/test.txt");
    std::cout << "test_basic_transfer PASSED\n";

    return 0;
}