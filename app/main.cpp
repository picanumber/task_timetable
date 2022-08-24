#include <chrono>
#include <iostream>
#include <stdlib.h>

#include "configurations.h"
#include "scheduler.h"

/*
 * Simple main program that demontrates how access
 * CMake definitions (here the version number) from source code.
 */
int main()
{
    std::cout << "Hello from main\n";

    ttt::CallScheduler sched(false);

    sched
        .add(
            [value = 1, tp = std::chrono::steady_clock::now()]() mutable {
                auto start = std::chrono::steady_clock::now();

                auto delta =
                    std::chrono::duration_cast<std::chrono::microseconds>(
                        start - tp);
                tp = start;

                std::cout << value++ << ".Execution after " << delta.count()
                          << "us\n";
                return ttt::Result::Repeat;
            },
            std::chrono::milliseconds(500))
        .detach();

    std::this_thread::sleep_for(std::chrono::seconds(5));

    return 0;
}
