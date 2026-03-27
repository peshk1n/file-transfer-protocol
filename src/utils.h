#pragma once
#include "picosha2.h"
#include <vector>
#include <string>
#include <cstdint>

template<class... Ts>
struct overloaded : Ts... { using Ts::operator()...; };

template<class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

inline std::string sha256_bytes(const std::vector<uint8_t>& data) {
    return picosha2::hash256_hex_string(data);
}