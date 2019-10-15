#pragma once

#include "Widget.hpp"


class Blind : public Widget {
public:
	
	Blind();

	~Blind() override;
	
	void setValue(uint8_t value) {this->value = value;}//this->targetValue = value << 16;}
	
protected:
	
	void setState() override;
	
	int speed;
	
	GLuint valueLocation;
	uint8_t value = 0;
};
