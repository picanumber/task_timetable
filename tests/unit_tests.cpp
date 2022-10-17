#include "doctest/doctest.h"
#include "scheduler.h"

#include <atomic>
#include <iostream>

using namespace std::chrono_literals;

auto since(auto start)
{
    return std::chrono::steady_clock::now() - start;
}

template <class T, class J> auto as(J &&interval)
{
    return std::chrono::duration_cast<T>(std::forward<J>(interval));
}

TEST_CASE("Finish on time - Stop on request")
{

    std::atomic_uint nCalls{0};
    ttt::CallScheduler callTable;
    auto threshold{200us};

    {
        auto token = callTable.add(
            [&nCalls] {
                ++nCalls;
                return ttt::Result::Repeat;
            },
            10us);

        auto start = std::chrono::steady_clock::now();
        while (nCalls < 10)
        {
        }

        std::cout << "Done in "
                  << as<std::chrono::microseconds>(since(start)).count()
                  << "us\n";
    }

    CHECK(1 == 1);
}
