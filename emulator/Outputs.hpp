#pragma once

#include "global.hpp"


/**
 * Binary outputs such as relays
 */
class Outputs {
public:
		
	Outputs() {}

	virtual ~Outputs();

	/**
	 * Set output state
	 * @param index index of output to set
	 * @param state state of output
	 */
	void setOutput(int index, bool state);

protected:

	uint32_t emulatorOutputs;
};
