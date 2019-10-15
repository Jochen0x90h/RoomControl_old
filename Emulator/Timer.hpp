#pragma once


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

	// maximum number of scenarios per button that can be cycled
	static const int SCENARIO_COUNT = 8;

	// trigger time
	uint32_t time;
	
	// ids of scenarios that are cycled when pressing the button
	uint8_t scenarios[SCENARIO_COUNT];

	int getScenarioCount() const {
		for (int i = 0; i < SCENARIO_COUNT; ++i) {
			if (this->scenarios[i] == 0xff)
				return i;
		}
		return SCENARIO_COUNT;
	}
};
