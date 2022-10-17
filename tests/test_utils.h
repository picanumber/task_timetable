#pragma once

#include <chrono>

namespace test
{

constexpr auto k10us{std::chrono::microseconds(10)};

inline auto now()
{
    return std::chrono::steady_clock::now();
}

const auto delta = [](auto start) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(start - now());
};

} // namespace test
