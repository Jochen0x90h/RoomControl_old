#include "Button.hpp"
#include <iostream>
#include <cmath>


Button::Button()
	: Widget(
		"#version 330\n"
		"uniform float state;\n"
		"in vec2 xy;\n"
		"out vec4 pixel;\n"
		"void main() {\n"
			"vec2 a = xy - vec2(0.5, 0.5f);\n"
			"float radius = sqrt(a.x * a.x + a.y * a.y);\n"
			"float limit = 0.5;\n"
			"pixel = radius < limit ? vec4(state, state, state, 1) : vec4(0, 0, 0, 1);\n"
		"}\n")
{
	
	// get uniform locations
	this->stateLocation = getUniformLocation("state");
}

Button::~Button()
{
}

void Button::touch(bool first, float x, float y)
{
	float ax = x - 0.5f;
	float ay = y - 0.5f;
	bool hit = std::sqrt(ax * ax + ay * ay) < 0.5;
	if (first) {
		// check if button was hit
		this->state = hit;
	}
	this->x = x;
	this->y = y;
}

void Button::release()
{
	this->state = false;
}

void Button::setState()
{
	// set uniforms
	glUniform1f(this->stateLocation, this->state ? 1.0f : 0.6f);
}
