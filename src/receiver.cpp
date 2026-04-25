#include "receiver.h"
#include "utils.h"


namespace transfer {

    Receiver::Receiver(const std::string& save_directory)
        : expected_seq(0)
        , total_chunks(0)
        , file_size(0)
        , save_directory(save_directory)
    {}


    Receiver::~Receiver() {
        if (out_file.is_open()) {
            out_file.close();
            std::remove(temp_path.c_str());
        }
    }


    void Receiver::feed_incoming(const std::vector<Packet>& packets, uint64_t now_ms) {
        if (state == State::ERROR) return;

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
        if (state != State::WAIT_START) {
            if (state == State::RECEIVING) {
                StartAckPacket ack;
                ack.status = Status::OK;
                outgoing.push_back(ack);
            }
            return;
        }

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
            (pkt.file_size + pkt.chunk_size - 1) / pkt.chunk_size);
        if (pkt.total_chunks != expected_chunks) return reject();

        temp_path = save_directory + "/" + pkt.file_name + ".tmp";
        final_path = save_directory + "/" + pkt.file_name;

        out_file.open(temp_path, std::ios::binary | std::ios::trunc);
        if (!out_file.is_open()) return reject();

        file_hasher = picosha2::hash256_one_by_one{};

        file_name = pkt.file_name;
        file_size = pkt.file_size;
        total_chunks = pkt.total_chunks;
        expected_file_hash = pkt.file_hash;
        expected_seq = 0;

        StartAckPacket ack;
        ack.status = Status::OK;
        outgoing.push_back(ack);
        state = State::RECEIVING;
    }


    void Receiver::on_data(const DataPacket& pkt) {
        if (state != State::RECEIVING) return;
        if (pkt.chunk_id == expected_seq && expected_seq < total_chunks) {
            if (pkt.payload.empty()) return;

            std::string actual_hash = sha256_bytes(pkt.payload);
            if (actual_hash != pkt.chunk_hash) return;

            out_file.write(
                reinterpret_cast<const char*>(pkt.payload.data()),
                pkt.payload.size()
            );
            file_hasher.process(pkt.payload.begin(), pkt.payload.end());

            expected_seq++;
            if (expected_seq == total_chunks)
                state = State::WAIT_END;
        }
    }


    void Receiver::on_end(const EndPacket& pkt) {
        if (state == State::DONE) {
            outgoing.push_back(EndAckPacket{});
        }
        if (state != State::WAIT_END) return;
        if (pkt.file_hash.empty()) { state = State::ERROR; return; }
        if (pkt.file_hash != expected_file_hash) { state = State::ERROR; return; }

        out_file.close();

        file_hasher.finish();
        std::string actual_hash = picosha2::get_hash_hex_string(file_hasher);

        if (actual_hash != pkt.file_hash || actual_hash != expected_file_hash) {
            std::remove(temp_path.c_str());
            state = State::ERROR;
            return;
        }

        std::remove(final_path.c_str()); // мб потом убрать?
        if (std::rename(temp_path.c_str(), final_path.c_str()) != 0) {
            std::remove(temp_path.c_str());
            state = State::ERROR;
            return;
        }

        outgoing.push_back(EndAckPacket{});
        state = State::DONE;
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
} 