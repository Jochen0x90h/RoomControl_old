#pragma once

#include <stdint.h>


template <typename T>
struct Array {
	const T *data;
	uint8_t length;
	
	Array() : data(nullptr), length(0) {}
	
	template <int N>
	Array(const T (&array)[N]) : data(array), length(N) {}

	Array(const T *data, int length) : data(data), length(length) {}

	const T &operator [](int index) const {return this->data[index];}
};
