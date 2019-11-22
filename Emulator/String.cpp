#include "String.hpp"


String string(const char *str) {
	int length = 0;
	while (str[length] != 0)
		++length;
	return {str, length};
}
