#include "Font.hpp"


int Font::calcWidth(const char *text, int space) {
	if (text == nullptr || *text == 0)
		return 0;
	int width = 0;
	while (true) {
		// width of character
		unsigned char ch = *text;
		if (ch <= this->first || ch >= this->last)
			ch = ' ';
		width += this->characters[ch - this->first].width;
		
		// increment and check for end
		++text;
		if (*text == 0)
			break;
		
		// width of space
		width += space;
	}
	return width;
}

int Font::calcWidth(const char *text, int length, int space) {
	if (length == 0)
		return 0;
	int width = 0;
	int i = 0;
	while (true) {
		// width of character
		unsigned char ch = text[i];
		if (ch <= this->first || ch >= this->last)
			ch = ' ';
		width += this->characters[ch - this->first].width;
	
		// increment and check for end
		++i;
		if (i == length)
			break;

		// width of space
		width += space;
	}
	return width;
}
