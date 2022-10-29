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

    ttt::Timeline timeline;

    if (timeline.addTimer(
            "t1", 500ms, 10'000ms, true, [](ttt::TimerState const &s) {
                std::cout << s.name << "> " << s.remaining.count() << "/"
                          << s.duration.count() << std::endl;
            }))
    {
        std::this_thread::sleep_for(15s);
    }

    return 0;
}
