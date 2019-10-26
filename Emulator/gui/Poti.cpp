#include "Poti.hpp"
#include <iostream>
#include <cmath>


Poti::Poti()
	: Widget(
		"#version 330\n"
		"uniform float value;\n"
		"uniform float state;\n"
		"in vec2 xy;\n"
		"out vec4 pixel;\n"
		"void main() {\n"
			"vec2 a = xy - vec2(0.5, 0.5f);\n"
			"float angle = atan(a.y, a.x) - value;\n"
			"float length = sqrt(a.x * a.x + a.y * a.y);\n"
			"float limit = cos(angle * 24) * 0.02 + 0.48;\n"
			"pixel = length < limit ? (length < 0.1 ? vec4(state, state, state, 1) : vec4(0.7, 0.7, 0.7, 1)) : vec4(0, 0, 0, 1);\n"
		"}\n")
{
	
	// get uniform locations
	this->valueLocation = getUniformLocation("value");
	this->stateLocation = getUniformLocation("state");
}

Poti::~Poti()
{
}

void Poti::touch(bool first, float x, float y)
{
	float ax = x - 0.5f;
	float ay = y - 0.5f;
	bool inner = std::sqrt(ax * ax + ay * ay) < 0.1;
	if (first) {
		// check if button (inner circle) was hit
		this->state = inner;
	} else if (!inner) {
		float bx = this->x - 0.5f;
		float by = this->y - 0.5f;

		float d = (ax * by - ay * bx) / (std::sqrt(ax * ax + ay * ay) * std::sqrt(bx * bx + by * by));
		this->value = this->value - int(d * 10000 * 24);
	}
	this->x = x;
	this->y = y;
}

void Poti::release()
{
	this->state = false;
}

void Poti::setState()
{
	// set uniforms
	glUniform1f(this->valueLocation, float(this->value & 0xffff) / (65536.0f * 24.0f) * 2.0f * M_PI);
	glUniform1f(this->stateLocation, this->state ? 1.0f : 0.0f);
}
