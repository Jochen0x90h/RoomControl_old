#pragma once

#include <chrono>
#include "util.hpp"

/**
 * Provides a monotinic ticker clock
 */
class Ticker {
public:
	
	Ticker() {
		this->start = std::chrono::steady_clock::now();
	}

	/**
	 * current time since system start in milliseconds
	 */
	uint32_t getTicks() {
		return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - this->start).count();
	}
	
protected:
	std::chrono::steady_clock::time_point start;
};
