#pragma once


struct Button {
	// maximum number of scenarios per button that can be cycled
	static const int SCENARIO_COUNT = 8;

	// enocean id of button
	uint32_t id;
	
	// button state
	uint8_t state;
	
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
