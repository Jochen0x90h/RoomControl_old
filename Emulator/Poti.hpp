#pragma once

#include "Widget.hpp"


class Poti : public Widget {
public:
	
	Poti();

	~Poti() override;
	
	uint16_t getValue() {return this->value >> 8;}

	void touch(bool first, float x, float y) override;
	
	void draw() override;
	
protected:
	
	void setState() override;
	
	GLuint valueLocation;
	uint32_t value;
	
	float x;
	float y;
};
