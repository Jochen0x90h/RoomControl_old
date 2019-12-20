#pragma once

#include "DeviceWidget.hpp"


class Light : public DeviceWidget {
public:
	
	Light(LayoutManager &layoutManager);

	~Light() override;
	
	static bool isCompatible(Device::Type type) {return type == Device::Type::SWITCH;}

	void setState(const DeviceState &deviceState) override;
	
	void setValue(uint8_t value) {this->value = value;}
	
protected:
	
	void setState() override;
	
	GLuint valueLocation;
	GLuint innerValueLocation;
	float value = 0;
	float innerValue = 0;
};
