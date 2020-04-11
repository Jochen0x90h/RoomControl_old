#pragma once

#include "global.hpp"


/**
 * Binary inputs such as buttons
 */
class Inputs {
public:

	Inputs() {}

	virtual ~Inputs();

	/**
	 * Gets called when an input event occurs
	 */
	virtual void onInput(int index, bool state) = 0;

protected:
	
};
