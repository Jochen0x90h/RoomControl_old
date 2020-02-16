#pragma once

#include <boost/asio.hpp>
#include <chrono>


using namespace std::chrono_literals;
namespace asio = boost::asio;

// io context (event loop for asynchronous input/output)
using Context = asio::io_service;

// ipv6 address
using Address = asio::ip::address_v6::bytes_type;

// duration of the steady system clock (does not depend on summer/winter clock change)
using SteadyDuration = std::chrono::steady_clock::duration;

// time of the steady system clock (does not depend on summer/winter clock change)
using SteadyTime = std::chrono::steady_clock::time_point;

inline uint32_t toSeconds(SteadyDuration duration) {
	return std::chrono::duration_cast<std::chrono::seconds>(duration).count();
}
