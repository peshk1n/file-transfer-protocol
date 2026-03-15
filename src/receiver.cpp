#include "receiver.h"
#include "utils.h"

namespace transfer {

    Receiver::Receiver()
        : expected_seq(0)
        , total_chunks(0)
        , file_size(0)
    {
    }

    void Receiver::feed_incoming(const std::vector<Packet>& packets) {
        for (const auto& pkt : packets) {
            std::visit(overloaded{
                [&](const StartPacket& p) { on_start(p); },
                [&](const DataPacket& p) { on_data(p);  },
                [&](const EndPacket& p) { on_end(p);   },
                [&](const auto&) {}  
                }, pkt);
        }
    }

    void Receiver::on_start(const StartPacket& pkt) {
        if (state != State::WAIT_START) return;

        file_name = pkt.file_name;
        file_size = pkt.file_size;
        total_chunks = pkt.total_chunks;
        expected_file_hash = pkt.file_hash;
        expected_seq = 0;

        buffer.resize(total_chunks);

        StartAckPacket ack;
        ack.status = Status::OK;
        outgoing.push_back(ack);

        state = State::RECEIVING;
    }

    std::vector<Packet> Receiver::poll_outgoing() {
        std::vector<Packet> result;
        std::swap(result, outgoing);
        return result;
    }

    bool  Receiver::is_done()      const { return state == State::DONE; }
    bool  Receiver::is_error()     const { return state == State::ERROR; }
    float Receiver::get_progress() const {
        if (total_chunks == 0) return 0.0f;
        return static_cast<float>(expected_seq) / static_cast<float>(total_chunks);
    }

    void Receiver::on_data(const DataPacket& pkt) { /* позже */ }
    void Receiver::on_end(const EndPacket& pkt) { /* позже */ }

} 