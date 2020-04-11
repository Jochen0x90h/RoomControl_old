#pragma once

#include <boost/asio.hpp>
#include <chrono>


using namespace std::chrono_literals;
namespace asio = boost::asio;

namespace global {

	// io context (event loop for asynchronous input/output)
	extern asio::io_service context;
};
