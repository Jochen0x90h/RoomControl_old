#pragma once

#include "Widget.hpp"
#include "Bitmap.hpp"


class Display : public Widget {
public:
	
	Display();
	
	~Display() override;
	
	void update();

	Bitmap<128, 64> bitmap;

protected:

	void setState() override;
	void resetState() override;

	// texture buffer for copying bitmap contents into the texture
	uint8_t buffer[128 * 64];
	GLuint texture;
};
