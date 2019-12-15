#pragma once

constexpr int min(int x, int y) {return x < y ? x : y;}
constexpr int max(int x, int y) {return x > y ? x : y;}
#ifndef _STDLIB_H_
constexpr int abs(int x) {return x > 0 ? x : -x;}
#endif
constexpr int clamp(int x, int minval, int maxval) {return x < minval ? minval : (x > maxval ? maxval : x);}
constexpr int limit(int x, int size) {return x < 0 ? 0 : (x >= size ? size - 1 : x);}

template <typename T, int N>
constexpr int size(const T (&array)[N]) {return N;}

template <int N, int M>
constexpr void copy(const char (&src)[N], char (&dst)[M]) {
	for (int i = 0; i < min(N, M); ++i)
		dst[i] = src[i];
}
