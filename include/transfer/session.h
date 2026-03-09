#pragma once

#include "packets.h"
#include <string>
#include <memory>
#include <vector>

namespace transfer
{
    class ITransferAgent;

    // Основной интерфейс сессии передачи файлов
    class TransferSession
    {
    public:
        TransferSession() = default;
        ~TransferSession() = default;

        // Инициализация сессии в режиме отправителя
        void init_as_sender(const std::string& file_path,
            uint32_t chunk_size = 4096,
            uint32_t window_size = 4);

        // Инициализация сессии в режиме получателя
        void init_as_receiver();

        // Передача входящих пакетов в сессию
        void feed_incoming(const std::vector<Packet>& packets);

        // Извлечение пакетов, готовых к отправке
        std::vector<Packet> poll_outgoing();

        // Обновление состояния сессии
        void tick(uint64_t now_ms);

        // Статус сессии
        float get_progress() const;   // Прогресс передачи/приема
        bool is_done() const;         // Передача завершена
        bool is_error() const;        // Сессия в состоянии ошибки

    private:
        std::unique_ptr<ITransferAgent> agent;  // Агент (отправитель или получатель)
        uint64_t last_packet_ms{};              // Время последнего пакета
        uint64_t timeout_ms{ 3000 };            // Таймаут ожидания (ms)
    };

} 