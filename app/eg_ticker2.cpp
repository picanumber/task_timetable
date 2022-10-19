#include <chrono>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>

#include "configurations.h"
#include "scheduler.h"

using namespace std::chrono_literals;

// This demo generates a "ticker" to invoke an action at an interval specified
// by the user. The time difference of an invocation, from the beginning of
// requests is printed on screen to verify:
// 1. The stability of interval computation.
// 2. The difference between compensating schedulers and non compensating, i.e.
// accounting for the execution time of the user function in the computation of
// the interval vs not accounting.
int main(int argc, [[maybe_unused]] char *argv[])
{
    char vStr[128];
    snprintf(vStr, std::size(vStr), "v%d.%d.%d.%d", PROJECT_VERSION_MAJOR,
             PROJECT_VERSION_MINOR, PROJECT_VERSION_PATCH,
             PROJECT_VERSION_TWEAK);

    std::cout << "Timer demoing task timetable " << vStr << std::endl;

    std::size_t msCount = 500;
    bool compensate = true;

    if (argc < 3)
    {
        std::cout << "Invoke the program as\n"
                  << "\neg_ticker msCount compensate\n"
                  << "\nwhere\n"
                  << "\tmsCount   : interval in milliseconds\n"
                  << "\tcompensate: 0 or 1 to showcase compensation or lack of "
                     "compensation\n";
        return 1;
    }
    else
    {
        msCount = std::stoul(argv[1]);
        compensate = std::stoul(argv[2]);
    }

    ttt::CallScheduler sched(compensate, std::thread::hardware_concurrency());

    auto tk1 = sched.add(
        [value = 1, start = std::chrono::steady_clock::now()]() mutable {
            auto delta = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start);

            std::cout << std::to_string(value++) + "... Reached after " +
                             std::to_string(delta.count()) + "ms\n";

            return ttt::Result::Repeat;
        },
        std::chrono::milliseconds(msCount));

    auto tk2 = sched.add(
        [value = 1, start = std::chrono::steady_clock::now()]() mutable {
            auto delta = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start);

            std::cout << std::to_string(value++) + ">>> Reached after " +
                             std::to_string(delta.count()) + "ms\n";

            return ttt::Result::Repeat;
        },
        std::chrono::milliseconds(2 * msCount));

    std::this_thread::sleep_for(5s);
    std::cout << "Destroying large task\n";
    tk2.~CallToken();

    std::this_thread::sleep_for(5s);
    std::cout << "Destroying small task\n";
    tk1.~CallToken();

    std::this_thread::sleep_for(1s);

    return 0;
}
