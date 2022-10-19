cmake_minimum_required(VERSION 3.14)

# Version numbers are available by including "exampleConfig.h" in
# the source. See exampleConfig.h.in for some more details.
project(TASK_TIMETABLE VERSION 1.0.0.0 LANGUAGES CXX)


# Options: Things you can set via commandline options to cmake (e.g. -DENABLE_LTO=[ON|OFF])
option(ENABLE_WARNINGS_SETTINGS "Allow target_set_warnings to add flags and defines.
                                 Set this to OFF if you want to provide your own warning parameters." ON)
option(ENABLE_LTO "Enable link time optimization" ON)
option(ENABLE_DOCTESTS "Include tests in the library. Setting this to OFF will remove all doctest related code.
                        Tests in tests/*.cpp will still be enabled." ON)

# Include stuff. No change needed.
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
include(ConfigSafeGuards)
include(Colors)
include(CTest)
include(Doctest)
include(Documentation)
include(LTO)
include(Misc)
include(Warnings)

# Check for LTO support.
find_lto(CXX)

# --------------------------------------------------------------------------------
#                         Locate files
# --------------------------------------------------------------------------------
set(SOURCES          # All .cpp files in src/
    src/scheduler.cpp
)
set(TESTFILES        # All .cpp files in tests/
    tests/main.cpp
)
set(LIBRARY_NAME tttable)  # Default name for the library built from src/*.cpp

# --------------------------------------------------------------------------------
#                            Build!
# --------------------------------------------------------------------------------
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

SET(USED_LIBS
    Threads::Threads
)

# Compile all sources into a library.
add_library(${LIBRARY_NAME} OBJECT ${SOURCES})
target_include_directories(${LIBRARY_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(${LIBRARY_NAME} PUBLIC ${USED_LIBS})

# Set the compile options you want (change as needed).
target_set_warnings(${LIBRARY_NAME} ENABLE ALL AS_ERROR ALL DISABLE Annoying)

# Add app executables
# eg_ticker
add_executable(eg_ticker app/eg_ticker.cpp)  
target_link_libraries(eg_ticker PRIVATE ${LIBRARY_NAME})
target_set_warnings(eg_ticker ENABLE ALL AS_ERROR ALL DISABLE Annoying)
target_enable_lto(eg_ticker optimized)  # enable lto if available for non-debug configurations
# eg_ticker2
add_executable(eg_ticker2 app/eg_ticker2.cpp)  
target_link_libraries(eg_ticker2 PRIVATE ${LIBRARY_NAME})
target_set_warnings(eg_ticker2 ENABLE ALL AS_ERROR ALL DISABLE Annoying)
target_enable_lto(eg_ticker2 optimized)  # enable lto if available for non-debug configurations

set_target_properties(
    ${LIBRARY_NAME} 
    eg_ticker 
    eg_ticker2
      PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
)

# Set up tests (see tests/CMakeLists.txt).
add_subdirectory(tests)
