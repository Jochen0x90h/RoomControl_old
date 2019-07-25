#include "Poti.hpp"
#include <iostream>
#include <cmath>


Poti::Poti() : Widget(
		"#version 330\n"
		"uniform float value;\n"
		"in vec2 uv;\n"
		"out vec4 pixel;\n"
		"void main() {\n"
		"    vec2 d = uv - vec2(0.5, 0.5f);\n"
		"    float angle = atan(d.y, d.x) + value;\n"
		"    float length = sqrt(d.x * d.x + d.y * d.y);\n"
		"    float limit = cos(angle * 24) * 0.02 + 0.48;\n"
		"    pixel = length < limit ? vec4(0.7, 0.7, 0.7, 1) : vec4(0, 0, 0, 1);\n"
		"}\n"), value(0) {
	
	// get uniform locations
	this->valueLocation = getUniformLocation("value");	
}

Poti::~Poti() {
}

void Poti::touch(bool first, float x, float y) {
	if (!first) {
		float ax = x - 0.5f;
		float ay = y - 0.5f;

		float bx = this->x - 0.5f;
		float by = this->y - 0.5f;

		float d = (ax * by - ay * bx) / (std::sqrt(ax * ax + ay * ay) * std::sqrt(bx * bx + by * by));
		this->value = this->value + int(d * 10000);
	}
	this->x = x;
	this->y = y;
}

void Poti::draw() {
	Widget::draw();
}

void Poti::setState() {
	// set uniforms
	glUniform1f(this->valueLocation, float(this->value & 0xffff) / 65536.0f * 2.0f * M_PI);
}
