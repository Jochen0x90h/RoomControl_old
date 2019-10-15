#pragma once

#include <stdint.h>


struct Actor {
	enum class Type : uint8_t {
		SWITCH,
		BLIND
	};

	char name[16];
	
	// initial value of actor (0-100)
	uint8_t value;
	
	// type of actor (switch, blind)
	Type type;
	
	// speed to reach target value
	uint16_t speed;
	
	// output index
	uint8_t outputIndex;
};
