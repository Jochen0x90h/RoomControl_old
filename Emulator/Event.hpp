#pragma once

#include "Action.hpp"


struct Event : public Actions {
	static const int MONDAY = 0x01;
	static const int TUESDAY = 0x02;
	static const int WEDNESDAY = 0x04;
	static const int THURSDAY = 0x08;
	static const int FRIDAY = 0x10;
	static const int SATURDAY = 0x20;
	static const int SUNDAY = 0x40;

	// maximum number of actions per button
	//static const int ACTION_COUNT = 8;

	enum class Type : uint8_t {
		SWITCH = 0,
		TIMER = 1
	};

	// type of event
	Type type;

	// extra value, e.g. switch button or timer weekday flags
	uint8_t value;

	// event input such as local switch, enocean switch (e.g. Eltako FT55) or timer time
	uint32_t input;

/*
	// list of actions, the first feasible action gets executed
	Action actions[ACTION_COUNT];

	int getActionCount() const {
		for (int i = 0; i < ACTION_COUNT; ++i) {
			if (!this->actions[i].isValid())
				return i;
		}
		return ACTION_COUNT;
	}
	*/
};
