#pragma once

#include "Widget.hpp"


class Light : public Widget {
public:
	
	Light();

	~Light() override;
	
	void setValue(uint8_t value) {this->value = value;}
	
protected:
	
	void setState() override;
	
	GLuint valueLocation;
	uint8_t value = 0;
};
