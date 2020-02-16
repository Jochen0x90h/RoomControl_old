#include "String.hpp"


String string(char const *str) {
	int length = 0;
	while (str[length] != 0)
		++length;
	return {str, length};
}
