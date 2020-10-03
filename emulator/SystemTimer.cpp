#include "SystemTimer.hpp"
#include <iostream>


SystemTimer::~SystemTimer() {
}

SystemTime SystemTimer::getSystemTime() {
	return std::chrono::steady_clock::now();
}


void SystemTimer::setSystemTimeout1(SystemTime time) {
	// https://www.boost.org/doc/libs/1_67_0/doc/html/boost_asio/reference/steady_timer.html
	this->emulatorTimer1.expires_at(time);
	this->emulatorTimer1.async_wait([this, time] (boost::system::error_code error) {
		if (!error) {
			onSystemTimeout1(time);
		}
	});
}

void SystemTimer::stopSystemTimeout1() {
	this->emulatorTimer1.cancel();
}


void SystemTimer::setSystemTimeout2(SystemTime time) {
	this->emulatorTimer2.expires_at(time);
	this->emulatorTimer2.async_wait([this, time] (boost::system::error_code error) {
		if (!error) {
			onSystemTimeout2(time);
		}
	});
}

void SystemTimer::stopSystemTimeout2() {
	this->emulatorTimer2.cancel();
}


void SystemTimer::setSystemTimeout3(SystemTime time) {
	this->emulatorTimer3.expires_at(time);
	this->emulatorTimer3.async_wait([this, time] (boost::system::error_code error) {
		if (!error) {
			onSystemTimeout3(time);
		}
	});
}

void SystemTimer::stopSystemTimeout3() {
	this->emulatorTimer3.cancel();
}
