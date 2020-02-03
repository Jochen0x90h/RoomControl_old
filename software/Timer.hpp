#pragma once

#include "Action.hpp"


struct Timer {
	static const uint8_t MONDAY = 0x01;
	static const uint8_t TUESDAY = 0x02;
	static const uint8_t WEDNESDAY = 0x04;
	static const uint8_t THURSDAY = 0x08;
	static const uint8_t FRIDAY = 0x10;
	static const uint8_t SATURDAY = 0x20;
	static const uint8_t SUNDAY = 0x40;

	// timer time
	uint32_t time;

	// weekday flags to indicate on which days the timer is active
	uint8_t weekdays;

	// the timer executes the first executable action when it triggers
	// note: must be last in struct!
	Actions actions;
	
	int byteSize() const {return offsetof(Timer, actions) + this->actions.byteSize();}
};
