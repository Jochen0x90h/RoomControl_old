#pragma once

#include "global.hpp"


/**
 * A steady system timer with 3 channels that is independent of summer/winter clock change
 */
class SystemTimer {
public:

	// duration
	using SystemDuration = std::chrono::steady_clock::duration;

	// time point
	using SystemTime = std::chrono::steady_clock::time_point;


	SystemTimer()
		: emulatorTimer1(global::context), emulatorTimer2(global::context), emulatorTimer3(global::context)
	{}

	virtual ~SystemTimer();

	/**
	 * @return current time
	 */
	SystemTime getSystemTime();


	/**
	 * Set the time when timer 1 will expire
	 * @param time when the timer will expire
	 */
	void setSystemTimeout1(SystemTime time);

	/**
	 * Stop timer 1
	 */
	void stopSystemTimeout1();

	/**
	 * Called when a timeout occured
	 * @param current time
	 */
	virtual void onSystemTimeout1(SystemTime time) = 0;


	/**
	 * Set the time when timer 2 will expire
	 * @param time when the timer will expire
	 */
	void setSystemTimeout2(SystemTime time);

	/**
	 * Stop timer 2
	 */
	void stopSystemTimeout2();

	/**
	 * Called when a timeout occured
	 * @param current time
	 */
	virtual void onSystemTimeout2(SystemTime time) = 0;


	/**
	 * Set the time when timer 3 will expire
	 * @param time when the timer will expire
	 */
	void setSystemTimeout3(SystemTime time);

	/**
	 * Stop timer 3
	 */
	void stopSystemTimeout3();

	/**
	 * Called when a timeout occured
	 * @param current time
	 */
	virtual void onSystemTimeout3(SystemTime time) = 0;


	/**
	 * Convert system time to seconds
	 */
	static uint32_t toSeconds(SystemDuration duration) {
		return std::chrono::duration_cast<std::chrono::seconds>(duration).count();
	}

protected:
	
	asio::steady_timer emulatorTimer1;
	asio::steady_timer emulatorTimer2;
	asio::steady_timer emulatorTimer3;
};
