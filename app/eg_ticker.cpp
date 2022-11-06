// © 2022 Nikolaos Athanasiou, github.com/picanumber
#include <chrono>
#include <cstring>
#include <iostream>
#include <string>

#include "configurations.h"
#include "task_timetable/scheduler.h"

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

    ttt::CallScheduler sched(compensate); // Default = 1 workers used.

    sched
        .add(
            [value = 1, start = std::chrono::steady_clock::now()]() mutable {
                auto delta =
                    std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - start);
                std::cout << value++ << ". Reached after " << delta.count()
                          << "ms\n";

                return ttt::Result::Repeat;
            },
            std::chrono::milliseconds(msCount))
        .detach();

    std::string input;
    std::cout << "Enter input to exit\n";
    std::cin >> input;

    return 0;
}
