// © 2022 Nikolaos Athanasiou, github.com/picanumber
#include "configurations.h"
#include "task_timetable/scheduler.h"

#include <chrono>
#include <iostream>
#include <optional>
#include <string>
#include <thread>

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

    std::optional<ttt::CallToken> tk1 = sched.add(
        [value = 1, start = std::chrono::steady_clock::now()]() mutable {
            auto delta = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start);

            std::cout << std::to_string(value++) + "... Reached after " +
                             std::to_string(delta.count()) + "ms\n";

            return ttt::Result::Repeat;
        },
        std::chrono::milliseconds(msCount));

    std::optional<ttt::CallToken> tk2 = sched.add(
        [value = 1, start = std::chrono::steady_clock::now()]() mutable {
            auto delta = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start);

            std::cout << std::to_string(value++) + ">>> Reached after " +
                             std::to_string(delta.count()) + "ms\n";

            return ttt::Result::Repeat;
        },
        std::chrono::milliseconds(2 * msCount));

    // Execute tasks for 5s then kill the large interval task.
    std::this_thread::sleep_for(5s);
    tk2.reset();
    std::cout << "tk2 out of scope: Destroyed large task\n";

    // Execute the small task for another 5s then kill it.
    std::this_thread::sleep_for(5s);
    tk1.reset();
    std::cout << "tk1 out of scope: Destroyed small task\n";
    // Show the task is not running by waiting 1s for messages.
    std::this_thread::sleep_for(1s);

    return 0;
}
