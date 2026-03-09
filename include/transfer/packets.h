#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <variant>

namespace transfer
{
    enum class PacketType : uint8_t {
        START = 1,
        START_ACK = 2,
        DATA = 3,
        ACK = 4,
        END = 5,
        END_ACK = 6
    };

    enum class Status : uint8_t {
        OK = 0,
        ERROR = 1
    };

    struct StartPacket {
        std::string file_name;
        uint64_t file_size;
        uint32_t chunk_size;
        uint32_t total_chunks;
        std::string file_hash;
    };

    struct StartAckPacket {
        Status status; 
    };

    struct DataPacket {
        uint32_t chunk_id;
        std::vector<uint8_t> payload;
        std::string chunk_hash;
    };

    struct AckPacket {
        uint32_t ack_id;
    };

    struct EndPacket {
        std::string file_hash;
    };

    struct EndAckPacket {};

    using Packet = std::variant<
        StartPacket,
        StartAckPacket,
        DataPacket,
        AckPacket,
        EndPacket,
        EndAckPacket
    >;
}