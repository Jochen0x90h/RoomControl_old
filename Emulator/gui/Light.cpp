#include "Light.hpp"
#include <iostream>
#include <cmath>


Light::Light(LayoutManager &layoutManager)
	: DeviceWidget(
		"#version 330\n"
		"uniform float value;\n"
		"in vec2 xy;\n"
		"out vec4 pixel;\n"
		"void main() {\n"
			"vec2 a = xy - vec2(0.5, 0.5f);\n"
			"float angle = atan(a.y, a.x) + value;\n"
			"float length = sqrt(a.x * a.x + a.y * a.y);\n"
			"float limit = cos(angle * 50) * 0.1 + 0.4;\n"
			"pixel = length < limit ? (vec4(value, value, 0, 1)) : vec4(0, 0, 0, 1);\n"
		"}\n", layoutManager)
{
	
	// get uniform locations
	this->valueLocation = getUniformLocation("value");	
}

Light::~Light() {
}

void Light::setState(const DeviceState &deviceState) {
	this->value = deviceState.interpolator >> 16;
}

void Light::setState() {
	// set uniforms
	glUniform1f(this->valueLocation, float(this->value) / 100.0f);
}
