#pragma once

#include "Context.hpp"


/**
 * A steady system timer that is independent of clock changes
 */
class SteadyTimer {
public:

	SteadyTimer(Context &context);

	virtual ~SteadyTimer();

	/**
	 * @return current time
	 */
	SteadyTime now();

	/**
	 * Set the time when the timer will expire
	 * @param time when the timer will expire
	 */
	void set(SteadyTime time);

	/**
	 * Stop the timer
	 */
	void stop();

	/**
	 * @param current time
	 */
	virtual void onTimeout(SteadyTime time) = 0;

protected:
	
	asio::steady_timer timer;
};
