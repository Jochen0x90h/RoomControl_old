#include "Font.hpp"


int Font::calcWidth(char const * text) {
	if (text == nullptr || *text == 0)
		return 0;
	int width = -1;
	for (; *text != 0; ++text) {
		unsigned char ch = *text;
		if (ch <= this->first || ch >= this->last)
			ch = ' ';
		width += this->characters[ch - this->first].width + 1;
	}
	return width;
}
