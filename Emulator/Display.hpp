#pragma once

#include "Widget.hpp"
#include "Bitmap.hpp"


class Display : public Widget {
public:
	static const int WIDTH = 128;
	static const int HEIGHT = 64;

	Display();
	
	~Display() override;
	
	void update(Bitmap<WIDTH, HEIGHT> &bitmap);

protected:

	void setState() override;
	void resetState() override;

	// texture buffer for copying bitmap contents into the texture
	uint8_t buffer[WIDTH * HEIGHT];
	GLuint texture;
};
