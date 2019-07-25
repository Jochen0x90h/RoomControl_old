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
void copyBitmapH(int w, int h, uint8_t *data, int x, int y, int width, int height, const uint8_t *bitmap, Mode mode);

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
	
	void drawRectangle(int x, int y, int width, int height, Mode mode) {
		fillBitmap(W, H, this->data, x, y, width, 1, mode);
		fillBitmap(W, H, this->data, x, y + height - 1, width, 1, mode);
		fillBitmap(W, H, this->data, x, y + 1, 1, height - 2, mode);
		fillBitmap(W, H, this->data, x + width - 1, y + 1, 1, height - 2, mode);
	}

	void fillRectangle(int x, int y, int width, int height, Mode mode) {
		fillBitmap(W, H, this->data, x, y, width, height, mode);
	}
	
	void drawGlyph(int x, int y, int width, int height, const uint8_t *data, Mode mode) {
		copyBitmapH(W, H, this->data, x, y, width, height, data, mode);
	}

	///
	/// draw text
	/// @return width of text
	int drawText(int x, int y, Font const & font, char const * text, Mode mode) {
		if (text == nullptr || *text == 0)
			return 0;
		for (; *text != 0; ++text) {
			unsigned char ch = *text;
			if (ch <= font.first || ch >= font.last)
				ch = ' ';
			const Character &character = font.characters[ch - font.first];
			
			copyBitmapH(W, H, this->data, x, y, character.width, font.height, font.bitmap + character.offset, mode);
			
			x += character.width + 1;
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
