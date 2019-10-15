#pragma once

#include <stdint.h>


struct Scenario {
	// maximum number of actors that can be controlled by a scenario
	static const int ACTOR_COUNT = 8;

	uint8_t id;
	char name[16];
	uint8_t actorValues[ACTOR_COUNT];
};
