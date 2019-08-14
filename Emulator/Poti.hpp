#pragma once

#include "Widget.hpp"


class Poti : public Widget {
public:
	
	Poti();

	~Poti() override;
	
	// quadrature encoder
	uint16_t getValue() {return this->value >> 16;}

	// switch
	bool getState();

	void touch(bool first, float x, float y) override;
	
	void release() override;
		
protected:
	
	void setState() override;
	
	GLuint valueLocation;
	GLuint stateLocation;
	
	// current rotation and press state of the poti
	uint32_t value = 0;
	bool state = false;
	
	// current mouse location
	float x;
	float y;
};
