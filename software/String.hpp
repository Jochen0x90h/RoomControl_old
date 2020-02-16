#pragma once

#include <stdint.h>


struct String {
	char const *data;
	int length;

	String(char const *data, int length)
		: data(data), length(length)
	{}
	
	template <int N>
	String(char const (&str)[N])
		: data(str)
	{
		int length = 0;
		while (length < N && str[length] != 0)
			++length;
		this->length = length;
	}
	
	bool empty() {return this->data == nullptr || this->length == 0;}
};
String string(char const *str);
