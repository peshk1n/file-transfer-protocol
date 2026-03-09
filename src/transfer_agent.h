#pragma once

#include "transfer/packets.h"
#include <vector>

namespace transfer
{
    // Интерфейс агента передачи файлов
    class ITransferAgent {
    public:
        virtual ~ITransferAgent() = default;

        // Передача входящих пакетов агенту
        virtual void feed_incoming(const std::vector<Packet>& packets) = 0;

        // Получение пакетов, готовых к отправке
        virtual std::vector<Packet> poll_outgoing() = 0;

        // Вызывается при срабатывании таймаута
        virtual void on_timeout() = 0;

        // Статус агента
        virtual bool  is_done()      const = 0;  // Передача завершена
        virtual bool  is_error()     const = 0;  // Ошибка передачи
        virtual float get_progress() const = 0;  // Прогресс передачи/приема
    };

} 