#pragma once

#include "global.hpp"


/**
 * A steady system timer that is independent of summer/winter clock change
 */
class SystemTimer {
public:

	// duration
	using SystemDuration = std::chrono::steady_clock::duration;

	// time point
	using SystemTime = std::chrono::steady_clock::time_point;


	SystemTimer()
		: emulatorTimer(global::context)
	{}

	virtual ~SystemTimer();

	/**
	 * @return current time
	 */
	SystemTime getSystemTime();

	/**
	 * Set the time when the timer will expire
	 * @param time when the timer will expire
	 */
	void setSystemTimeout(SystemTime time);

	/**
	 * Stop the timer
	 */
	void stopSystemTimeout();

	/**
	 * @param current time
	 */
	virtual void onSystemTimeout(SystemTime time) = 0;

	static uint32_t toSeconds(SystemDuration duration) {
		return std::chrono::duration_cast<std::chrono::seconds>(duration).count();
	}

protected:
	
	asio::steady_timer emulatorTimer;
};
