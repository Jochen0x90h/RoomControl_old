#pragma once

#include "Widget.hpp"


class Button : public Widget {
public:

	Button();

	~Button() override;

	// switch
	bool getState() {return this->state;}

	bool isActivated()
	{
		bool state = getState();
		bool activated = !state && this->lastState;
		this->lastState = state;
		return activated;
	}

	void touch(bool first, float x, float y) override;
	
	void release() override;
		
protected:
	
	void setState() override;
	
	GLuint stateLocation;
	
	// current press state of the button
	bool state = false;
	bool lastState = false;
	
	// current mouse location
	float x;
	float y;
};
