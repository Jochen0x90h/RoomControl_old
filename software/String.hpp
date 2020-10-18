#pragma once

#include "defines.hpp"
#include <iostream>


// helper classes for distinguishing between fixed size array and c-string
struct False {};
struct True {};

template <typename T>
struct IsArray : False {};
  
template <typename T, int N>
struct IsArray<T[N]> : True {};

template<int N>
int getLength(char const (&str)[N], True) {
	int length = 0;
	while (length < N && str[length] != 0)
		++length;
	return length;
}

inline int getLength(char const *str, False) {
	int length = 0;
	while (str[length] != 0)
		++length;
	return length;
}


template<typename T>
int length(T &str) {
	return getLength(str, IsArray<T>());
}


/**
 * String, only references the data
 */
struct String {
	char const *data;
	int const length;

	String()
		: data(), length(0)
	{}

	String(String &str)
		: data(str.data), length(str.length)
	{}

	String(String const &str)
		: data(str.data), length(str.length)
	{}

	template<typename T>
	String(T &str)
		: data(str), length(getLength(str, IsArray<T>()))
	{}

	String(char const *data, int length)
		: data(data), length(length)
	{}

	String(uint8_t const *data, int length)
		: data(reinterpret_cast<char const*>(data)), length(length)
	{}

	bool empty() {return this->length <= 0;}
	
	String substring(int beginIndex, int endIndex) {
		return String(data + beginIndex, endIndex - beginIndex);
	}

	constexpr char const operator [](int index) const {return this->data[index];}

	char const *begin() const {return this->data;}
	char const *end() const {return this->data + this->length;}
};

/**
 * Assign a string to a fixed size c-string
 */
template <int N>
inline void assign(char (&str)[N], String s) {
	int count = N < s.length ? N : s.length;
	for (int i = 0; i < count; ++i) {
		str[i] = s[i];
	}
	for (int i = count; i < N; ++i) {
		str[i] = 0;
	}
}

inline std::ostream &operator <<(std::ostream &s, String str) {
	s.write(str.data, str.length);
	return s;
}
