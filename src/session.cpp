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

    void TransferSession::feed_incoming(const std::vector<Packet>& packets, uint64_t now_ms) {
        if (!agent) {
            if (packets.empty()) return;
            if (std::holds_alternative<StartPacket>(packets.front())) {
                agent = std::make_unique<Receiver>();
            }
            else {
                return; // непонятный пакет без агента - игнорим
            }
        }

        agent->feed_incoming(packets, now_ms);
    }

    std::vector<Packet> TransferSession::poll_outgoing() {
        if (!agent) return {};
        return agent->poll_outgoing();
    }

    void TransferSession::tick(uint64_t now_ms) {
        if (!agent) return;
        agent->tick(now_ms);
    }

    float TransferSession::get_progress() const { 
        if (!agent) return 0.0f;
        return agent->get_progress(); 
    }

    bool TransferSession::is_done() const { 
        if (!agent) return false;
        return agent->is_done(); 
    }

    bool TransferSession::is_error() const { 
        if (!agent) return false;
        return agent->is_error(); 
    }

}