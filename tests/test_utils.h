// © 2022 Nikolaos Athanasiou, github.com/picanumber
#pragma once

#include <chrono>
#include <string>

namespace test
{

constexpr auto k10us{std::chrono::microseconds(10)};

inline auto now()
{
    return std::chrono::steady_clock::now();
}

template <class D = std::chrono::milliseconds,
          class T = std::chrono::steady_clock::time_point>
auto delta(T const &start)
{
    return std::chrono::duration_cast<D>(now() - start);
}

template <class D = std::chrono::milliseconds,
          class T = std::chrono::steady_clock::time_point>
auto delta(T const &start, T const &end)
{
    return std::chrono::duration_cast<D>(end - start);
}

#define FAILED_REQUIREMENT(msg)                                                \
    REQUIRE_MESSAGE(false == true, (std::string("Requirement failed: ") + msg))

} // namespace test
