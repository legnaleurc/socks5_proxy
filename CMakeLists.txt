cmake_minimum_required(VERSION 3.6)

project(socks5_proxy)

find_package(Boost REQUIRED COMPONENTS system program_options coroutine)
find_package(Threads REQUIRED)

set(HEADERS
    "src/exception.hpp"
    "src/global.hpp"
    "src/global_p.hpp"
    "src/server.hpp"
    "src/server_p.hpp"
    "src/session.hpp"
    "src/session_p.hpp")
set(SOURCES
    "src/exception.cpp"
    "src/main.cpp"
    "src/global.cpp"
    "src/server.cpp"
    "src/session.cpp")

add_executable(socks5_proxy ${SOURCES} ${HEADERS})
target_compile_features(socks5_proxy PRIVATE cxx_auto_type)
target_compile_definitions(socks5_proxy PRIVATE BOOST_COROUTINES_NO_DEPRECATION_WARNING BOOST_COROUTINE_NO_DEPRECATION_WARNING)
target_link_libraries(socks5_proxy
    Boost::dynamic_linking
    Boost::disable_autolinking
    Boost::system
    Boost::program_options
    Boost::coroutine
    Threads::Threads)
