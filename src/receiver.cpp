#include "receiver.h"
#include "utils.h"
#include <iostream>

namespace transfer {

    Receiver::Receiver()
        : expected_seq(0)
        , total_chunks(0)
        , file_size(0)
    {
    }


    void Receiver::feed_incoming(const std::vector<Packet>& packets) {
        bool had_data = false;

        for (const auto& pkt : packets) {
            std::visit(overloaded{
                [&](const StartPacket& p) { on_start(p); },
                [&](const DataPacket& p) { on_data(p); had_data = true; },
                [&](const EndPacket& p) { on_end(p);  },
                [&](const auto&) {}
                }, pkt);
        }

        if (had_data && state == State::RECEIVING) {
            AckPacket ack;
            ack.ack_id = expected_seq;
            outgoing.push_back(ack);
        }
    }


    void Receiver::on_start(const StartPacket& pkt) {
        if (state != State::WAIT_START) return;

        file_name = pkt.file_name;
        file_size = pkt.file_size;
        total_chunks = pkt.total_chunks;
        expected_file_hash = pkt.file_hash;
        expected_seq = 0;

        std::cout << file_name << " " << file_size << " " << total_chunks << " " << expected_file_hash << "\n";

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


    bool  Receiver::is_done() const { 
        return state == State::DONE; 
    }

    bool  Receiver::is_error() const { 
        return state == State::ERROR; 
    }

    float Receiver::get_progress() const {
        if (total_chunks == 0) return 0.0f;
        return static_cast<float>(expected_seq) / static_cast<float>(total_chunks);
    }


    void Receiver::on_data(const DataPacket& pkt) {
        if (state != State::RECEIVING) return;

        if (pkt.chunk_id == expected_seq) {
            std::string actual_hash = std::to_string(
                std::hash<std::string>{}(std::string(pkt.payload.begin(), pkt.payload.end()))
            );

            if (actual_hash != pkt.chunk_hash) {
                return;
            }

            buffer[expected_seq] = pkt.payload;
            expected_seq++;

            if (expected_seq == total_chunks)
                state = State::WAIT_END;
        }
    }


    void Receiver::on_end(const EndPacket& pkt) { /* позже */ }

} 