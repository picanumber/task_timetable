# Task scheduling  made easy

[![Project Status: Active – The project has reached a stable, usable state and is being actively developed.](http://www.repostatus.org/badges/latest/active.svg)](http://www.repostatus.org/#active)
[![](https://tokei.rs/b1/github/picanumber/task_timetable)](https://github.com/picanumber/task_timetable)
[![license](https://img.shields.io/hexpm/l/plug)](https://github.com/picanumber/task_timetable/blob/a7b8eb6eed728255221909583d9e757b4e345a5a/LICENSE)

[![CI](https://github.com/picanumber/task_timetable/actions/workflows/ci.yml/badge.svg)](https://github.com/picanumber/task_timetable/actions/workflows/ci.yml)
[![Memory](https://github.com/picanumber/task_timetable/actions/workflows/asan.yml/badge.svg)](https://github.com/picanumber/task_timetable/actions/workflows/asan.yml)
[![Threading](https://github.com/picanumber/task_timetable/actions/workflows/tsan.yml/badge.svg)](https://github.com/picanumber/task_timetable/actions/workflows/tsan.yml)

The goal of this library

## Structure




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

## Setup
