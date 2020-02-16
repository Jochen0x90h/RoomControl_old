#include "SteadyTimer.hpp"
#include <iostream>


SteadyTimer::SteadyTimer(Context &context)
	: timer(context)
{
}

SteadyTimer::~SteadyTimer() {
}

SteadyTime SteadyTimer::now() {
	return std::chrono::steady_clock::now();
}

void SteadyTimer::set(SteadyTime time) {
	// https://www.boost.org/doc/libs/1_67_0/doc/html/boost_asio/reference/steady_timer.html
	this->timer.expires_at(time);
	this->timer.async_wait([this, time] (boost::system::error_code error) {
		if (!error) {
			onTimeout(time);
		}
	});
}

void SteadyTimer::stop() {
	this->timer.cancel();
}
