#pragma once

#include "transfer_agent.h"
#include "transfer/packets.h"
#include <vector>
#include <string>
#include <cstdint>

namespace transfer {

    // Агент получателя файла
    class Receiver : public ITransferAgent {
    public:
        Receiver();

        // Реализация интерфейса ITransferAgent
        void feed_incoming(const std::vector<Packet>& packets) override; 
        std::vector<Packet> poll_outgoing() override;                     
        void on_timeout() override {}                                     

        bool is_done() const override;       // Передача завершена
        bool is_error() const override;      // Ошибка приема
        float get_progress() const override; // Прогресс приема

    private:
        // Обработка конкретных пакетов
        void on_start(const Packet& pkt);
        void on_data(const Packet& pkt);
        void on_end(const Packet& pkt);

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

        std::vector<Packet> outgoing; // Пакеты для отправки
        std::string error;            // Сообщение об ошибке
    };

} 