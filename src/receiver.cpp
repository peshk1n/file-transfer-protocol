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

    void Receiver::feed_incoming(const std::vector<Packet>& packets, uint64_t now_ms) {
        if (state == State::ERROR || state == State::DONE) return;

        bool had_data = false;
        for (const auto& pkt : packets) {
            std::visit(overloaded{
                [&](const StartPacket& p) { on_start(p); },
                [&](const DataPacket& p) { on_data(p); had_data = true; },
                [&](const EndPacket& p) { on_end(p);  },
                [&](const auto&) {}
                }, pkt);
        }

        if (had_data && (state == State::RECEIVING || state == State::WAIT_END)) {
            AckPacket ack;
            ack.ack_id = expected_seq;
            outgoing.push_back(ack);
        }
    }


    void Receiver::on_start(const StartPacket& pkt) {
        if (state != State::WAIT_START) return;

        auto reject = [&]() {
            StartAckPacket ack;
            ack.status = Status::ERROR;
            outgoing.push_back(ack);
            state = State::ERROR;
            };

        if (pkt.file_name.empty()) return reject();
        if (pkt.file_hash.empty()) return reject();
        if (pkt.file_size == 0)    return reject();
        if (pkt.chunk_size == 0)   return reject();
        if (pkt.total_chunks == 0) return reject();

        uint32_t expected_chunks = static_cast<uint32_t>(
            (pkt.file_size + pkt.chunk_size - 1) / pkt.chunk_size
            );
        if (pkt.total_chunks != expected_chunks) return reject();

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
            if (pkt.payload.empty()) return;

            std::string actual_hash = sha256_bytes(pkt.payload);

            if (actual_hash != pkt.chunk_hash) {
                return;
            }

            buffer[expected_seq] = pkt.payload;
            expected_seq++;

            if (expected_seq == total_chunks)
                state = State::WAIT_END;
        }
    }

    void Receiver::on_end(const EndPacket& pkt) {
        if (state != State::WAIT_END) return;

        std::vector<uint8_t> full_file;
        full_file.reserve(file_size);
        for (const auto& chunk : buffer) {
            full_file.insert(full_file.end(), chunk.begin(), chunk.end());
        }

        std::string actual_hash = sha256_bytes(full_file);
        if (actual_hash != pkt.file_hash || actual_hash != expected_file_hash) {
            state = State::ERROR;
            return;
        }

        outgoing.push_back(EndAckPacket{});
        state = State::DONE;
    }
} 