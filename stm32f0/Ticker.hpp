#pragma once

/**
 * Provides a monotinic ticker clock
 */
class Ticker {
public:
	
	Ticker() {
	}

	/**
	 * current time since system start in milliseconds
	 */
	uint32_t getTicks() {
		return 0;
	}
	
protected:
};
