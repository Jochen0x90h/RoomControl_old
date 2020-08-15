#pragma once


/**
 * Array, only references the data
 * @tparam T array element
 */
template <typename T>
struct Array {
	const T *data;
	int length;
		
	constexpr Array() : data(nullptr), length(0) {}

	template <int N>
	constexpr Array(const T (&array)[N]) : data(array), length(N) {}

	constexpr Array(const T *data, int length) : data(data), length(length) {}

	bool empty() {return this->length <= 0;}

	constexpr const T &operator [](int index) const {return this->data[index];}

	const T *begin() const {return this->data;}
	const T *end() const {return this->data + this->length;}
};
