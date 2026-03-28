#pragma once

#include "transfer_agent.h"
#include "transfer/packets.h"
#include "utils.h"

#include <vector>
#include <string>
#include <cstdint>
#include <fstream>
//#include <iostream>

namespace transfer {

    // Агент получателя файла
    class Receiver : public ITransferAgent {
    public:
        Receiver(const std::string& save_directory);
        ~Receiver();

        // Реализация интерфейса ITransferAgent
        void feed_incoming(const std::vector<Packet>& packets, uint64_t now_ms) override;
        std::vector<Packet> poll_outgoing() override;                     
        void tick(uint64_t now_ms) override {}

        bool is_done() const override;       // Передача завершена
        bool is_error() const override;      // Ошибка приема
        float get_progress() const override; // Прогресс приема

    private:
        // Обработка конкретных пакетов
        void on_start(const StartPacket& pkt);
        void on_data(const DataPacket& pkt);
        void on_end(const EndPacket& pkt);

        // Состояния агента
        enum class State {
            WAIT_START,
            RECEIVING,
            WAIT_END,
            DONE,
            ERROR
        };
        State state{ State::WAIT_START };

        uint32_t expected_seq; // Следующий ожидаемый пакет
        uint32_t total_chunks; // Общее количество пакетов

        std::string expected_file_hash;          // Ожидаемый хэш файла
        std::string file_name;                   // Имя файла
        uint64_t file_size{};                     // Размер файла
        std::vector<std::vector<uint8_t>> buffer; // Сборка чанков файла

        std::string save_directory;
        std::string temp_path;
        std::string final_path;
        std::ofstream out_file;

        std::vector<Packet> outgoing; // Пакеты для отправки
        std::string error;            // Сообщение об ошибке

        picosha2::hash256_one_by_one file_hasher;
    };

} 