// Executables must have the following defined if the library contains
// doctest definitions. For builds with this disabled, e.g. code shipped to
// users, this can be left out.
#ifdef ENABLE_DOCTEST_IN_LIBRARY
#define DOCTEST_CONFIG_IMPLEMENT
#include "doctest/doctest.h"
#endif

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

    cls::CallScheduler sched;

    sched
        .add(
            [] {
                std::cout << "Task was done\n";
                return cls::Result::Finished;
            },
            std::chrono::milliseconds(500))
        ->detach();

    std::this_thread::sleep_for(std::chrono::seconds(5));

    return 0;
}
