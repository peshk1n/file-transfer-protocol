# File Transfer Protocol

Реализация протокола передачи файлов на C++17 с алгоритмом скользящего окна (Go-Back-N).

## Протокол

Передача файла состоит из трёх фаз:

1. **Установка соединения** — отправитель шлёт `START` с метаданными файла, получатель отвечает `START_ACK`.
2. **Передача данных** — отправитель шлёт чанки в окне размером `window_size`, получатель подтверждает кумулятивными ACK. При потере или повреждении пакета (проверка SHA-256) окно откатывается и данные досылаются заново.
3. **Завершение** — отправитель шлёт `END` с хэшем файла, получатель проверяет целостность и отвечает `END_ACK`.

Все пакеты типизированы через `std::variant`. Сессия не зависит от транспорта, приложение само передаёт входящие и исходящие пакеты.

## Структура проекта
```
file-transfer-protocol/
├── include/transfer/        # Публичное API
│   ├── packets.h            # Типы пакетов
│   └── session.h            # TransferSession
├── src/                     # Реализация 
│   ├── session.cpp
│   ├── sender.cpp / .h
│   ├── receiver.cpp / .h
│   ├── utils.h
│   └── ...
├── simulation/              # Симуляция канала
│   ├── simulation.h / .cpp  # Simulation, SimulationConfig, SimulationResult
│   └── fake_channel.h / .cpp
└── tests/                   # Unit-тесты 
    └── basic_transfer.cpp
```

## Сборка

Требования: CMake 3.16+, C++17.

```bash
git clone https://github.com/peshk1n/file-transfer-protocol.git
cd file-transfer-protocol
cmake -B build
cmake --build build
```

## Запуск тестов

```bash
cd build
ctest
# или напрямую
./tests/test_transfer
```

Тесты проверяют передачу в чистом канале, с потерями пакетов и повреждениями данных. На Android не собираются.
