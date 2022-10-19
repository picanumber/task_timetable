# C++ task scheduler

[![Project Status: Active – The project has reached a stable, usable state and is being actively developed.](http://www.repostatus.org/badges/latest/active.svg)](http://www.repostatus.org/#active)
[![](https://tokei.rs/b1/github/picanumber/task_timetable)](https://github.com/XAMPPRocky/tokei)
[![license](https://img.shields.io/hexpm/l/plug)](https://github.com/picanumber/task_timetable/blob/a7b8eb6eed728255221909583d9e757b4e345a5a/LICENSE)

[![CI](https://github.com/picanumber/task_timetable/actions/workflows/ci.yml/badge.svg)](https://github.com/picanumber/task_timetable/actions/workflows/ci.yml)
[![Memory](https://github.com/picanumber/task_timetable/actions/workflows/asan.yml/badge.svg)](https://github.com/picanumber/task_timetable/actions/workflows/asan.yml)
[![Threading](https://github.com/picanumber/task_timetable/actions/workflows/tsan.yml/badge.svg)](https://github.com/picanumber/task_timetable/actions/workflows/tsan.yml)
[![CodeQL](https://github.com/picanumber/task_timetable/actions/workflows/codeql.yml/badge.svg)](https://github.com/picanumber/task_timetable/actions/workflows/codeql.yml)
[![Style](https://github.com/picanumber/task_timetable/actions/workflows/style.yml/badge.svg)](https://github.com/picanumber/task_timetable/actions/workflows/style.yml)

A facility to defer tasks for __execution or repetition at specified intervals__.

## Usage

The functionality is available through the scheduler class:

```cpp
#include "scheduler.h"

{

    bool compensate = true; // Whether to calculate the time point of task
                            // executions by adding the interval to the:
                            // true: task start / end: task finish.

    unsigned nWorkers = 1;  // Number of workers that run tasks.

    ttt::CallScheduler plan(compensate, nWorkers);
}
```

Adding a task to the scheduler is done using the `add` method:

```cpp
{
    auto myTask = []{
        ttt::Result ret{ ttt::Result::Repeat };

        if ( /*user logic*/ )
        {
            ret = ttt::Result::Finish;
        }

        return ret; // A user task returns whether to repeat or not.
    };

    auto token = plan.add(
        myTask, // User tasks are std::function<ttt::Result()>
        500ms,  // Interval for execution or repetition
        false); // Whether to execute the task immediately
}
```

As shown above, the addition of a task is associated with a token. Additionally the returned token is marked `[[no_discard]]` to force some decision making to the user. Tokens are 1-1 associated with
tasks added to the scheduler. Due to this association three things can happen:

1. The token is __alive__    : Task is allowed to run.
2. The token is __destroyed__: Tasks are prevented from running after token destruction and are removed from the scheduler.
3. The token is __detached__ : Task is independent from the token state.

A detached token is created by calling the `detach()` method. This implies that if there is no need to ever cancel an added task, e.g. it dies with the scheduler, the following syntax can be used to ignore the token upon creation:

```cpp
plan.add(myTask, interval, compensate).detach();
```

## Building

Build by making a build directory (i.e. `build/`), run `cmake` in that dir, and then use `make` to build the desired target.

Example:

``` bash
> mkdir build && cd build
> cmake .. -DCMAKE_BUILD_TYPE=[Debug | Coverage | Release]
> make
> ./main
> make test      # Makes and runs the tests.
> make coverage  # Generate a coverage report.
> make doc       # Generate html documentation.
```

A convenience script `rebuild_all.sh` is provided for users that want to generate all types of build, i.e. release, sanitizers (thread & address) and debug.
