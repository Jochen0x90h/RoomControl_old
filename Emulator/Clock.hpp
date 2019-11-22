
#pragma once

#include <stdint.h>


class Clock {
public:

	static const int SECONDS_MASK = 0x0000007f; // secons as bcd
	static const int MINUTES_MASK = 0x00007f00; // minutes as bcd
	static const int HOURS_MASK   = 0x003f0000; // hours as bcd
	static const int WEEKDAY_MASK = 0x07000000; // 0 (Monday) to 6 (Sunday)

	static const int MINUTES_SHIFT = 8;
	static const int HOURS_SHIFT   = 16;
	static const int WEEKDAY_SHIFT = 24;
	
	Clock() {
	}
	
	/**
	 * Get weekday and time packed into one int, see mask and shift constants
	 */
	uint32_t getTime();


	// convert time to to bcd
	static constexpr uint32_t time(int hours, int minutes, int seconds = 0) {
		return seconds % 10 | seconds / 10 << 4
			| (minutes % 10 | minutes / 10 << 4) << 8
			| (hours % 10 | hours / 10 << 4) << 16;
	}
	
	static int addTime(int time, int delta, int maxval) {
		int t0 = (time & 0x0f) + delta;
		while (t0 >= 10) {
			t0 -= 10;
			time += 0x10;
		}
		while (t0 < 0) {
			t0 += 10;
			time -= 0x10;
		}
		time = (time & ~0x0f) | t0;
		return time < 0 ? maxval : (time > maxval ? 0 : time);
	}
};
