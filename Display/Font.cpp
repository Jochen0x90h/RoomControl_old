#include "Font.hpp"


int Font::calcWidth(const char *text, int space) {
	if (text == nullptr)
		return 0;
	int length = 0;
	while (text[length] != 0)
		++length;
	return calcWidth(text, length, space);
}

int Font::calcWidth(const char *text, int length, int space) {
	int width = 0;
	while (length > 0) {
		unsigned char ch = *text;
		if (ch == '\t') {
			width += TAB_WIDTH;
		} else {
			if (ch < this->first || ch > this->last)
				ch = '?';

			// width of character
			width += this->characters[ch - this->first].width;
			
			// width of space
			width += space;
		}
	
		// next character
		++text;
		--length;
	}
	return width;
}
