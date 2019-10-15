#pragma once

#include "Font.hpp"


// draw modes
enum class Mode {
	CLEAR = 0,
	KEEP = 1,
	FLIP = 2,
	SET = 3
};

// fill a bitmap
void fillBitmap(int w, int h, uint8_t *data, int x, int y, int width, int height, Mode mode);

// copy a bitmap that is organized horizontally (font)
void copyBitmapH(int w, int h, uint8_t *data, int x, int y, int width, int height, const uint8_t *bitmap,
	Mode frontMode, Mode backMode);

template <int W, int H>
class Bitmap {
public:
	static const int WIDTH = W;
	static const int HEIGHT = H;
	
	void clear() {
		int size = W * H >> 3;
		for (int i = 0; i < size; ++i)
			this->data[i] = 0;
	}
	
	void drawRectangle(int x, int y, int width, int height, Mode mode = Mode::SET) {
		fillBitmap(W, H, this->data, x, y, width, 1, mode);
		fillBitmap(W, H, this->data, x, y + height - 1, width, 1, mode);
		fillBitmap(W, H, this->data, x, y + 1, 1, height - 2, mode);
		fillBitmap(W, H, this->data, x + width - 1, y + 1, 1, height - 2, mode);
	}

	void fillRectangle(int x, int y, int width, int height, Mode mode = Mode::SET) {
		fillBitmap(W, H, this->data, x, y, width, height, mode);
	}
	
	//void drawGlyph(int x, int y, int width, int height, const uint8_t *data, Mode mode = Mode::SET) {
	//	copyBitmapH(W, H, this->data, x, y, width, height, data, mode);
	//}

	///
	/// draw text
	/// @return x coordinate of end of text
	int drawText(int x, int y, Font const & font, char const * text, int space, Mode frontMode = Mode::SET,
		Mode backMode = Mode::KEEP)
	{
		if (text == nullptr || *text == 0)
			return x;
		while (true) {
			// draw character
			unsigned char ch = *text;
			if (ch <= font.first || ch >= font.last)
				ch = ' ';
			const Character &character = font.characters[ch - font.first];
			copyBitmapH(W, H, this->data, x, y, character.width, font.height, font.bitmap + character.offset, frontMode,
				backMode);
			x += character.width;
			
			// increment and check for end
			++text;
			if (*text == 0)
				break;
			
			// draw space
			fillBitmap(W, H, this->data, x, y, space, font.height, backMode);
			x += space;
		}
		return x;
	}

	///
	/// bitmap data, data layout: rows of 8 pixels where each byte describes a column in each row
	/// this would be the layout of a 16x16 display where each '|' is one byte
	/// ||||||||||||||||
	/// ||||||||||||||||
	uint8_t data[W * (H >> 3)];
};
