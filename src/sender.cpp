#include "sender.h"
#include "utils.h"
#include <fstream>
#include <stdexcept>
#include <functional>

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

        std::vector<uint8_t> file_data{
            std::istreambuf_iterator<char>(file),
            std::istreambuf_iterator<char>()
        };

        std::string file_hash = std::to_string(
            std::hash<std::string>{}(std::string(file_data.begin(), file_data.end()))
        );

        uint32_t id = 0;
        for (size_t offset = 0; offset < file_data.size(); offset += chunk_size) {
            size_t end = std::min(offset + chunk_size, file_data.size());
            std::vector<uint8_t> payload(file_data.begin() + offset, file_data.begin() + end);

            std::string chunk_hash = std::to_string(
                std::hash<std::string>{}(std::string(payload.begin(), payload.end()))
            );

            DataPacket dp;
            dp.chunk_id = id++;
            dp.payload = std::move(payload);
            dp.chunk_hash = std::move(chunk_hash);
            chunks.push_back(std::move(dp));
        }

        total_chunks = static_cast<uint32_t>(chunks.size());

        std::string file_name = file_path.substr(file_path.find_last_of("/\\") + 1);

        StartPacket sp;
        sp.file_name = file_name;
        sp.file_size = static_cast<uint64_t>(file_data.size());
        sp.chunk_size = chunk_size;
        sp.total_chunks = total_chunks;
        sp.file_hash = file_hash;
        outgoing.push_back(std::move(sp));

        state = State::WAIT_START_ACK;
    }


    std::vector<Packet> Sender::poll_outgoing() {
        std::vector<Packet> result;
        std::swap(result, outgoing);
        return result;
    }

    void Sender::on_timeout()
    {
        /* позже */
    }


    void Sender::feed_incoming(const std::vector<Packet>& packets) {
        for (const auto& pkt : packets) {
            std::visit(overloaded{
                [&](const StartAckPacket& p) { on_start_ack(p); },
                [&](const AckPacket& p) { on_ack(p);       },
                [&](const EndAckPacket& p) { on_end_ack(p);   },
                [&](const auto&) {}  // неожиданный тип 
                }, pkt);
        }
    }


    void Sender::on_start_ack(const StartAckPacket& pkt) {
        if (state != State::WAIT_START_ACK) return;

        if (pkt.status == Status::ERROR) {
            state = State::ERROR;
            return;
        }

        base = 0;
        next_seq = 0;
        state = State::TRANSFERRING;

        fill_window(); 
    }


    bool  Sender::is_done()      const { return state == State::DONE; }
    bool  Sender::is_error()     const { return state == State::ERROR; }
    float Sender::get_progress() const {
        if (total_chunks == 0) return 0.0f;
        return static_cast<float>(base) / static_cast<float>(total_chunks);
    }

    void Sender::on_ack(const AckPacket& pkt)
    { 
        /* позже */ 
    }

    void Sender::on_end_ack(const EndAckPacket& pkt)
    {
        /* позже */
    }

    void Sender::fill_window()
    {
        /* позже */
    }

    void Sender::retransmit()
    {
        /* позже */
    }

} 