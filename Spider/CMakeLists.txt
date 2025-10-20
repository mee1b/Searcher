cmake_minimum_required(VERSION 3.30)
project(Spider)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")

set(PROJECT_SOURCES
    ParcerINI.h
    ParcerINI.cpp
    HTTPclient.h
    HTTPclient.cpp
    ParcerHTML.h
    ParcerHTML.cpp
    database.h
    database.cpp
    indexator.h
    indexator.cpp
    main.cpp
)

add_executable(Spider ${PROJECT_SOURCES})

find_package(Boost 1.88 COMPONENTS locale system thread REQUIRED)
if(Boost_FOUND)
    target_include_directories(Spider PRIVATE ${Boost_INCLUDE_DIRS})
    target_link_libraries(Spider PRIVATE ${Boost_LIBRARIES})
endif()


set(OPENSSL_INCLUDE_DIR "C:/vcpkg/installed/x64-windows/include")
set(OPENSSL_LIB_DIR "C:/vcpkg/installed/x64-windows/lib")
include_directories(${OPENSSL_INCLUDE_DIR})
target_link_libraries(Spider PRIVATE
    "${OPENSSL_LIB_DIR}/libssl.lib"
    "${OPENSSL_LIB_DIR}/libcrypto.lib"
)

set(GUMBO_INCLUDE_DIR "C:/vcpkg/installed/x64-windows/include")
set(GUMBO_LIB_DIR "C:/vcpkg/installed/x64-windows/lib")
include_directories(${GUMBO_INCLUDE_DIR})
target_link_libraries(Spider PRIVATE "${GUMBO_LIB_DIR}/gumbo.lib")


set(PQXX_INCLUDE_DIR "C:/vcpkg/installed/x64-windows/include")
set(PQXX_LIB_DIR "C:/vcpkg/installed/x64-windows/lib")
include_directories(${PQXX_INCLUDE_DIR})
target_link_libraries(Spider PRIVATE "${PQXX_LIB_DIR}/pqxx.lib" "${PQXX_LIB_DIR}/libpq.lib")

add_custom_command(TARGET Spider POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "C:/vcpkg/installed/x64-windows/bin/boost_locale-vc143-mt-x64-1_88.dll"
    $<TARGET_FILE_DIR:Spider>
)
