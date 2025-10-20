cmake_minimum_required(VERSION 3.30)
project(Finder)
set(CMAKE_CXX_STANDARD 20)

set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")

set(Boost_USE_STATIC_LIBS        ON)
set(Boost_USE_MULTITHREADED      ON)
set(Boost_USE_STATIC_RUNTIME    OFF)

find_package(Boost 1.86.0 COMPONENTS locale REQUIRED)
find_package(PostgreSQL REQUIRED)

link_directories("C:/vcpkg/installed/x64-windows/lib")
include_directories("C:/vcpkg/installed/x64-windows/include")

set(PROJECT_SOURCES
    finder.h
    finder.cpp
    main.cpp
    ../Spider/database.h
    ../Spider/database.cpp
    ../Spider/ParcerINI.h
    ../Spider/ParcerINI.cpp
)

add_executable(Finder ${PROJECT_SOURCES})

target_link_libraries(Finder
    Boost::locale
    pqxx.lib
)
