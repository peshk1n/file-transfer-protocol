#include "transfer/session.h"
#include "transfer_agent.h"
#include "sender.h"
#include "receiver.h"

namespace transfer {

	TransferSession::TransferSession() = default;
	TransferSession::~TransferSession() = default;

    void TransferSession::init_as_sender(const std::string& file_path,
        uint32_t chunk_size,
        uint32_t window_size) {
        agent = std::make_unique<Sender>(file_path, chunk_size, window_size);
    }

    void TransferSession::init_as_receiver() {
        agent = std::make_unique<Receiver>();
    }

    void TransferSession::feed_incoming(const std::vector<Packet>& packets) {
        if (!agent) {
            if (packets.empty()) return;
            if (std::holds_alternative<StartPacket>(packets.front())) {
                agent = std::make_unique<Receiver>();
            }
            else {
                return; // непонятный пакет без агента - игнорим
            }
        }

        agent->feed_incoming(packets);
    }

    std::vector<Packet> TransferSession::poll_outgoing() {
        return agent->poll_outgoing();
    }

    void TransferSession::tick(uint64_t now_ms) {
        if (!agent) 
            return;
        if (last_packet_ms == 0) {
            last_packet_ms = now_ms; 
            return;
        }
        if (now_ms - last_packet_ms >= timeout_ms)
            agent->on_timeout();

        last_packet_ms = now_ms;
    }

    float TransferSession::get_progress() const { return agent->get_progress(); }
    bool  TransferSession::is_done() const { return agent->is_done(); }
    bool  TransferSession::is_error() const { return agent->is_error(); }

}