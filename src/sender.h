#pragma once

#include "transfer/packets.h"
#include "transfer_agent.h"
#include <vector>
#include <string>
#include <cstdint>

namespace transfer {

    // Агент отправителя файла
    class Sender : public ITransferAgent {
    public:
        Sender(const std::string& file_path, uint32_t chunk_size, uint32_t window_size);

        // Реализация интерфейса ITransferAgent
        void feed_incoming(const std::vector<Packet>& packets, uint64_t now_ms) override;
        std::vector<Packet> poll_outgoing() override;                     
        void tick(uint64_t now_ms) override;

        bool is_done() const override;     // Передача завершена
        bool is_error() const override;    // Ошибка передачи
        float get_progress() const override; // Прогресс передачи

    private:
        // Обработка подтверждений
        void on_start_ack(const StartAckPacket& pkt, uint64_t now_ms);
        void on_ack(const AckPacket& pkt, uint64_t now_ms);
        void on_end_ack(const EndAckPacket& pkt);

        // Управление окном передачи
        void fill_window(uint64_t now_ms);   // Добавляет новые DATA-пакеты в outgoing, пока окно не заполнено
        void retransmit(uint64_t now_ms);    // Повторная отправка всех пакетов в диапазоне [base, next_seq)

        void start_timer(uint64_t now_ms);
        void stop_timer();

        // Состояния агента
        enum class State {
            WAIT_START_ACK,
            TRANSFERRING,
            WAIT_END_ACK,
            DONE,
            ERROR
        };
        State state{ State::WAIT_START_ACK };

        // Окно передачи
        uint32_t base;        // Первый неподтверждённый пакет
        uint32_t next_seq;    // Следующий пакет для отправки
        uint32_t total_chunks; // Общее количество DATA-пакетов
        uint32_t window_size;  // Размер окна передачи

        std::vector<Packet> chunks;   // Все DATA-пакеты, нарезанные из файла
        std::vector<Packet> outgoing; // Пакеты, готовые к отправке

        std::uint64_t timer_start_ms{ 0 };   // 0 = таймер не запущен
        std::uint64_t timeout_ms{ 3000 };

        StartPacket start_packet;
    };

} 