#include "sender.h"
#include "utils.h"
#include <fstream>
#include <stdexcept>

namespace transfer {

    Sender::Sender(const std::string& file_path,
        uint32_t chunk_size,
        uint32_t window_size)
        : base(0)
        , next_seq(0)
        , total_chunks(0)
        , window_size(window_size)
    {
        std::ifstream file(file_path, std::ios::binary);
        if (!file.is_open())
            throw std::runtime_error("Cannot open file: " + file_path);

        file.seekg(0, std::ios::end);
        uint64_t file_size = static_cast<uint64_t>(file.tellg());
        file.seekg(0, std::ios::beg);

        if (file_size == 0)
            throw std::runtime_error("File is empty: " + file_path);

        picosha2::hash256_one_by_one file_hasher;
        uint32_t id = 0;

        while (!file.eof()) {
            std::vector<uint8_t> payload(chunk_size);
            file.read(reinterpret_cast<char*>(payload.data()), chunk_size);
            size_t bytes_read = static_cast<size_t>(file.gcount());
            if (bytes_read == 0) break;
            payload.resize(bytes_read);

            file_hasher.process(payload.begin(), payload.end());
            std::string chunk_hash = picosha2::hash256_hex_string(payload);

            DataPacket dp;
            dp.chunk_id = id++;
            dp.payload = std::move(payload);
            dp.chunk_hash = std::move(chunk_hash);
            chunks.push_back(std::move(dp));
        }

        file_hasher.finish();
        std::string file_hash = picosha2::get_hash_hex_string(file_hasher);
        total_chunks = static_cast<uint32_t>(chunks.size());

        std::string file_name = file_path.substr(file_path.find_last_of("/\\") + 1);

        StartPacket sp;
        sp.file_name = file_name;
        sp.file_size = file_size;
        sp.chunk_size = chunk_size;
        sp.total_chunks = total_chunks;
        sp.file_hash = file_hash;
        start_packet = sp;
        outgoing.push_back(std::move(sp));

        state = State::WAIT_START_ACK;
    }


    std::vector<Packet> Sender::poll_outgoing() {
        std::vector<Packet> result;
        std::swap(result, outgoing);
        return result;
    }


    void Sender::feed_incoming(const std::vector<Packet>& packets, uint64_t now_ms) {
        if (state == State::ERROR || state == State::DONE) return;
        for (const auto& pkt : packets) {
            std::visit(overloaded{
                [&](const StartAckPacket& p) { on_start_ack(p, now_ms); },
                [&](const AckPacket& p) { on_ack(p, now_ms);       },
                [&](const EndAckPacket& p) { on_end_ack(p);           },
                [&](const auto&) {}
                }, pkt);
        }
    }

    void Sender::on_start_ack(const StartAckPacket& pkt, uint64_t now_ms) {
        if (state != State::WAIT_START_ACK) return;
        if (pkt.status == Status::ERROR) {
            state = State::ERROR;
            return;
        }
        base = 0;
        next_seq = 0;
        state = State::TRANSFERRING;
        fill_window(now_ms);  
    }

    void Sender::on_ack(const AckPacket& pkt, uint64_t now_ms) {
        if (state != State::TRANSFERRING) return;
        if (pkt.ack_id <= base || pkt.ack_id > total_chunks) return;

        base = pkt.ack_id;

        if (base == total_chunks) {
            EndPacket ep;
            ep.file_hash = start_packet.file_hash;
            outgoing.push_back(ep);
            start_timer(now_ms); 
            state = State::WAIT_END_ACK;
            return;
        }

        fill_window(now_ms);

        if (base < next_seq)
            start_timer(now_ms);
        else
            stop_timer();
    }

    void Sender::on_end_ack(const EndAckPacket&) {
        if (state != State::WAIT_END_ACK) return;
        stop_timer();
        state = State::DONE;
    }

    bool  Sender::is_done() const { 
        return state == State::DONE; 
    }

    bool  Sender::is_error() const { 
        return state == State::ERROR; 
    }

    float Sender::get_progress() const {
        if (total_chunks == 0) return 0.0f;
        return static_cast<float>(base) / static_cast<float>(total_chunks);
    }


    void Sender::fill_window(uint64_t now_ms) {
        while (next_seq < base + window_size && next_seq < total_chunks) {
            if (next_seq == base) {
                // отправляем первый неподтверждённый - запускаем таймер
                start_timer(now_ms);
            }
            outgoing.push_back(chunks[next_seq]);
            next_seq++;
        }
    }

    void Sender::retransmit(uint64_t now_ms) {
        for (uint32_t i = base; i < next_seq; i++) {
            outgoing.push_back(chunks[i]);
        }
        start_timer(now_ms);
    }

    // Таймер
    void Sender::start_timer(uint64_t now_ms) {
        timer_start_ms = now_ms;
    }

    void Sender::stop_timer() {
        timer_start_ms = 0;
    }

    void Sender::tick(uint64_t now_ms) {
        if (state == State::ERROR || state == State::DONE) return;

        if (timer_start_ms == 0 && state == State::WAIT_START_ACK) {
            start_timer(now_ms);
            return;
        }
        if (timer_start_ms == 0) return;
        if (now_ms - timer_start_ms < timeout_ms) return;

        if (state == State::TRANSFERRING) {
            retransmit(now_ms);
        }
        else if (state == State::WAIT_START_ACK) {
            outgoing.push_back(start_packet);
            start_timer(now_ms);
        }
        else if (state == State::WAIT_END_ACK) {
            EndPacket ep;
            ep.file_hash = start_packet.file_hash;
            outgoing.push_back(ep);
            start_timer(now_ms);
        }
    }

} 