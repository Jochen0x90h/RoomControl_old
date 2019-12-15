#include "Light.hpp"
#include <iostream>
#include <cmath>


Light::Light(LayoutManager &layoutManager)
	: DeviceWidget(
		"#version 330\n"
		"uniform float value;\n"
		"uniform float innerValue;\n"
		"in vec2 xy;\n"
		"out vec4 pixel;\n"
		"void main() {\n"
			"vec2 a = xy - vec2(0.5, 0.5f);\n"
			"float angle = atan(a.y, a.x) + value;\n"
			"float length = sqrt(a.x * a.x + a.y * a.y);\n"
			"float limit = cos(angle * 50) * 0.1 + 0.4;\n"
			"float innerLimit = 0.1;\n"
			"vec4 background = vec4(0, 0, 0, 1);\n"
			"vec4 color = vec4(value, value, 0, 1);\n"
			"vec4 innerColor = vec4(innerValue, innerValue, 0, 1);\n"
			"pixel = length < limit ? (length < innerLimit ? innerColor : color) : background;\n"
		"}\n", layoutManager)
{
	
	// get uniform locations
	this->valueLocation = getUniformLocation("value");	
	this->innerValueLocation = getUniformLocation("innerValue");
}

Light::~Light() {
}

void Light::setState(const DeviceState &deviceState) {
	this->value = float(deviceState.mode == DeviceState::DELAY ? deviceState.lastState : deviceState.state) / 100.0f;
	this->innerValue = float(deviceState.state) / 100.0f;
}

void Light::setState() {
	// set uniforms
	glUniform1f(this->valueLocation, this->value);
	glUniform1f(this->innerValueLocation, this->innerValue);
}
