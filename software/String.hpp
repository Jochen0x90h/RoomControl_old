#pragma once

#include <stdint.h>

struct False {};
struct True {};

template <typename T>
struct IsArray : False {};
  
template <typename T, int N>
struct IsArray<T[N]> : True {};

template<int N>
int lengthT(char const (&str)[N], True) {
	int length = 0;
	while (length < N && str[length] != 0)
		++length;
	return length;
}

inline int lengthT(char const *str, False) {
	int length = 0;
	while (str[length] != 0)
		++length;
	return length;
}

template<typename T>
int length(T &str) {
	return lengthT(str, IsArray<T>());
}


struct String {
	char const *data;
	int const length;

	String(String &str)
		: data(str.data), length(str.length)
	{}

	String(String const &str)
		: data(str.data), length(str.length)
	{}

	template<typename T>
	String(T &str)
		: data(str), length(::length(str))
	{}

	String(char const *data, int length)
		: data(data), length(length)
	{}

/*
	
	template <int N>
	String(char const (&str)[N])
		: data(str)
	{
		int length = 0;
		while (length < N && str[length] != 0)
			++length;
		this->length = length;
	}
*/
	bool empty() {return this->length == 0;}
	
	String substring(int beginIndex, int endIndex) {
		return String(data + beginIndex, endIndex - beginIndex);
	}
	
	char const *begin() const {return this->data;}
	char const *end() const {return this->data + this->length;}
};
String string(char const *str);
