#include "Temperature.hpp"
#include <cmath>

inline int clamp(int x, int minValue, int maxValue) {
	return x < minValue ? minValue : (x > maxValue ? maxValue : x);
}


Temperature::Temperature()
	: Widget(
		"#version 330\n"
		"uniform float targetValue;\n"
		"uniform float currentValue;\n"
		"uniform vec3 state;\n"
		"in vec2 xy;\n"
		"out vec4 pixel;\n"
		"void main() {\n"
		"	float x = (xy.x - 0.5) * 0.1;"
		"	float yc = (xy.y - currentValue);"
		"	float yt = (xy.y - targetValue);"
		"	pixel = (yt + x < 0 && yt - x > 0) || (yc + x > 0 && yc - x < 0) ? vec4(1.0, 1.0, 1.0, 1) : vec4(state, 1);\n"
		"}\n")
	, currentValue((maxValue + minValue) >> 1)
	, targetValue((maxValue + minValue) >> 1)
{
	// get uniform locations
	this->targetValueLocation = glGetUniformLocation(this->program, "targetValue");
	this->currentValueLocation = glGetUniformLocation(this->program, "currentValue");
	this->stateLocation = glGetUniformLocation(this->program, "state");
}

Temperature::~Temperature() {
}

void Temperature::touch(bool first, float x, float y) {
	this->currentValue = clamp(int(std::round(y * (this->maxValue - this->minValue))) + this->minValue,
		this->minValue, this->maxValue);
}

void Temperature::update() {
	int hysteresis = 256;
	this->state = this->currentValue < this->targetValue + (this->state ? hysteresis : -hysteresis);
}

void Temperature::setState() {
	float range = float(this->maxValue - this->minValue);
	
	// set uniforms
	glUniform1f(this->targetValueLocation, float(this->targetValue - this->minValue) / range);
	glUniform1f(this->currentValueLocation, float(this->currentValue - this->minValue) / range);
	const float off[] = {0.2f, 0.2f, 1.0f};
	const float on[] = {1.0f, 0.1f, 0.0f};
	glUniform3fv(this->stateLocation, 1, this->state ? on : off);
}
