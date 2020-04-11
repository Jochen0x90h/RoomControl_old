#include "SystemTimer.hpp"
#include <iostream>


SystemTimer::~SystemTimer() {
}

SystemTimer::SystemTime SystemTimer::getSystemTime() {
	return std::chrono::steady_clock::now();
}

void SystemTimer::setSystemTimeout(SystemTime time) {
	// https://www.boost.org/doc/libs/1_67_0/doc/html/boost_asio/reference/steady_timer.html
	this->emulatorTimer.expires_at(time);
	this->emulatorTimer.async_wait([this, time] (boost::system::error_code error) {
		if (!error) {
			onSystemTimeout(time);
		}
	});
}

void SystemTimer::stopSystemTimeout() {
	this->emulatorTimer.cancel();
}
