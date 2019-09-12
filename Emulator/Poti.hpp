#pragma once

#include "Widget.hpp"


class Poti : public Widget {
public:
	
	Poti();

	~Poti() override;
	
	// quadrature encoder
	uint16_t getValue() {return this->value >> 16;}

	int getDelta()
	{
		int16_t value = getValue();
		int delta = value - this->lastValue;
		this->lastValue = value;
		return delta;
	}

	// switch
	bool getState() {return this->state;}

	bool getPress()
	{
		bool state = getState();
		bool press = !state && this->lastState;
		this->lastState = state;
		return press;
	}

	void touch(bool first, float x, float y) override;
	
	void release() override;
		
protected:
	
	void setState() override;
	
	GLuint valueLocation;
	GLuint stateLocation;
	
	// current rotation and press state of the poti
	uint32_t value = 0;
	int16_t lastValue = 0;
	bool state = false;
	bool lastState = false;
	
	// current mouse location
	float x;
	float y;
};
