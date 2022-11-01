// © 2022 Nikolaos Athanasiou, github.com/picanumber
#include "configurations.h"
#include "timeline.h"

#include <chrono>
#include <iostream>
#include <optional>
#include <string>
#include <thread>

using namespace std::chrono_literals;

int main([[maybe_unused]] int argc, [[maybe_unused]] char *argv[])
{
    char vStr[128];
    snprintf(vStr, std::size(vStr), "v%d.%d.%d.%d", PROJECT_VERSION_MAJOR,
             PROJECT_VERSION_MINOR, PROJECT_VERSION_PATCH,
             PROJECT_VERSION_TWEAK);

    std::cout << "Demoing timeline class " << vStr << std::endl;

    std::string t1("t1");
    ttt::Timeline timeline;

    if (timeline.timerAdd(
            t1, 500ms, 3'000ms, true, [](ttt::TimerState const &s) {
                std::cout << s.name << "> " << s.remaining.load().count() << "/"
                          << s.duration.count() << std::endl;
            }))
    {
        std::this_thread::sleep_for(5s);
    }

    timeline.timerPause(t1);
    std::cout << "Paused for 3 secs" << std::endl;
    std::this_thread::sleep_for(3s);

    std::cout << "Resuming for 3 secs more" << std::endl;
    timeline.timerResume(t1);
    std::this_thread::sleep_for(3s);

    return 0;
}
