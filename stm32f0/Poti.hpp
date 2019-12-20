#pragma once


class Poti {
public:
	
	Poti() {}
	
	// quadrature encoder
	uint16_t getValue() {return 0;}

	int getDelta()
	{
		int16_t value = getValue();
		int delta = value - this->lastValue;
		this->lastValue = value;
		return delta;
	}

	// switch
	bool getState() {return false;}

	bool isActivated()
	{
		bool state = getState();
		bool press = !state && this->lastState;
		this->lastState = state;
		return press;
	}
		
protected:
		
	// current rotation and press state of the poti
	int16_t lastValue = 0;
	bool lastState = false;
};
