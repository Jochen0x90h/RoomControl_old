#pragma once

#define getOffset(Type, member) intptr_t(&((Type*)nullptr)->member)

constexpr int min(int x, int y) {return x < y ? x : y;}
constexpr int max(int x, int y) {return x > y ? x : y;}

/*
#ifndef _STDLIB_H_
constexpr int abs(int x) {return x > 0 ? x : -x;}
#endif
constexpr int clamp(int x, int minval, int maxval) {return x < minval ? minval : (x > maxval ? maxval : x);}
constexpr int limit(int x, int size) {return x < 0 ? 0 : (x >= size ? size - 1 : x);}
*/
namespace array {

template <typename T, int N>
constexpr int size(const T (&array)[N]) {return N;}

template <typename T, int N>
constexpr T *end(T (&array)[N]) {return array + N;}

template <typename T, int N>
constexpr T const *end(const T (&array)[N]) {return array + N;}

template <typename T>
/*void insert(T *array, int size, int index, int count = 1) {
	int i = size;
	while (i > index + count) {
		--i;
		array[i] = array[i - count];
	}
}*/
void insert(T *array, T *end, int count = 1) {
	T *it = end - count;
	while (it > array) {
		--it;
		it[count] = it[0];
	}
}

template <typename T>
/*void erase(T *array, int size, int index, int count = 1) {
	for (int i = index + count; i < size; ++i) {
		array[i - count] = array[i];
	}
}
*/
void erase(T *array, T *end, int count = 1) {
	for (T *it = array + count; it < end; ++it) {
		it[-count] = it[0];
	}
}

template <typename ForwardIt, typename T>
void fill(ForwardIt first, ForwardIt last, const T &value) {
	for (ForwardIt it = first; it < last; ++it) {
		*it = value;
	}
}

template <typename InputIt, typename OutputIt>
void copy(InputIt first, InputIt last, OutputIt dst) {
	for (InputIt src = first; src != last; ++src, ++dst) {
		*dst = *src;
	}
}

}

/*
template <int N, int M>
constexpr void copy(const char (&src)[N], char (&dst)[M]) {
	for (int i = 0; i < min(N, M); ++i)
		dst[i] = src[i];
}
*/
