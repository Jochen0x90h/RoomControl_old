#pragma once

#include "DeviceWidget.hpp"


class Blind : public DeviceWidget {
public:
	
	Blind(LayoutManager &layoutManager);

	~Blind() override;

	static bool isCompatible(Device::Type type) {return type == Device::Type::BLIND;}

	void setState(const DeviceState &deviceState) override;

	void setValue(uint8_t value) {this->value = value;}//this->targetValue = value << 16;}
	
protected:
	
	void setState() override;
	
	int speed;
	
	GLuint valueLocation;
	uint8_t value = 0;
};
