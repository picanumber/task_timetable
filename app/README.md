## Example applications build with the task timetable library.

### eg_ticker

This demo schedules an invocation at an interval specified by the user. The time difference of an invocation, from the beginning of requests is printed on screen to verify:
1. The stability of interval computation.
2. The difference between compensating schedulers and non compensating, i.e. accounting for the execution time of the user function in the computation of the interval vs not accounting.

Invoke the program as:

```sh
$ eg_ticker msCount compensate
```

Where:

* msCount   : interval in milliseconds
* compensate: Next exeution is calculated as
    * 0 : userFunEndTime   + interval
    * 1 : userFunStartTime + interval

### eg_ticker2

Variation of the ticker demo where:

* Hardware concurrency is used for the workers of the scheduler is used.
* A second task is interleaved at twice the specified user interval.
* The returned tokens are destroyed in fixed time points to demostrate the effects.
* The demo exits after 11 seconds.
