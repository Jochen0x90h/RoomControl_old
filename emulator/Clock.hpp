#pragma once

#include "global.hpp"
#include <boost/date_time/posix_time/posix_time.hpp>
#include <stdint.h>


class Clock {
public:

	static const uint32_t SECONDS_MASK = 0x0000003f; // secons
	static const uint32_t MINUTES_MASK = 0x00003f00; // minutes
	static const uint32_t HOURS_MASK   = 0x001f0000; // hours
	static const uint32_t WEEKDAY_MASK = 0x07000000; // week day: 0 (Monday) to 6 (Sunday)

	static const int SECONDS_SHIFT = 0;
	static const int MINUTES_SHIFT = 8;
	static const int HOURS_SHIFT   = 16;
	static const int WEEKDAY_SHIFT = 24;
	
	struct ClockTime {
	
		constexpr ClockTime(int weekday, int hours, int minutes, int seconds = 0)
			: time(seconds | minutes << MINUTES_SHIFT | hours << HOURS_SHIFT | weekday << WEEKDAY_SHIFT) {
		}

		int getSeconds() const {return (this->time & SECONDS_MASK) >> SECONDS_SHIFT;}
		int getMinutes() const {return (this->time & MINUTES_MASK) >> MINUTES_SHIFT;}
		int getHours() const {return (this->time & HOURS_MASK) >> HOURS_SHIFT;}
		int getWeekday() const {return (this->time & WEEKDAY_MASK) >> WEEKDAY_SHIFT;}

		void addSeconds(int delta);
		void addMinutes(int delta);
		void addHours(int delta);

		uint32_t time;
	};
	
	Clock();

	virtual ~Clock();

	/**
	 * Get time and weekday packed into one int, see mask and shift constants
	 */
	ClockTime getClockTime();

	/**
	 * Internal use only
	 */
	void setClockTimeout(boost::posix_time::ptime utc);

	/**
	 * Called when a second has elapsed
	 */
	virtual void onSecondElapsed() = 0;

/*
	// pack time into an int
	static constexpr uint32_t time(int hours, int minutes, int seconds = 0) {
		return seconds
			| (minutes) << MINUTES_SHIFT
			| (hours) << HOURS_SHIFT;
	}
	
	/ **
	 * Add delta to seconds, minutes or hours
	 * @param t time value
	 * @param delta value
	 * @param maxValue maximum value, e.g. 60 for seconds and minutes and 24 for hours
	 * /
	static int add(int t, int delta, int maxValue) {
		t += delta;
		while (t < 0) t += maxValue;
		while (t > maxVal) t -= maxValue
		return t;
	}
	*/
	
	asio::deadline_timer emulatorClock;
};
