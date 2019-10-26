#pragma once

#include "Action.hpp"


struct Timer {
	static const int WEEKDAYS_MASK = 0x7f000000; // flag for each weekday
	static const int MONDAY = 0x01000000;
	static const int TUESDAY = 0x02000000;
	static const int WEDNESDAY = 0x04000000;
	static const int THURSDAY = 0x08000000;
	static const int FRIDAY = 0x10000000;
	static const int SATURDAY = 0x20000000;
	static const int SUNDAY = 0x40000000;

	static const int WEEKDAYS_SHIFT = Clock::WEEKDAY_SHIFT;

	// maximum number of actions per timer
	static const int ACTION_COUNT = 8;

	// trigger time
	uint32_t time;
	
	// list of actions, the first feasible action gets executed
	Action actions[ACTION_COUNT];

	int getActionCount() const {
		for (int i = 0; i < ACTION_COUNT; ++i) {
			if (!this->actions[i].isValid())
				return i;
		}
		return ACTION_COUNT;
	}
};
