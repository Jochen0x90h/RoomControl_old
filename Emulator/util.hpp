#pragma once

inline int min(int x, int y) {return x < y ? x : y;}
inline int max(int x, int y) {return x > y ? x : y;}
#ifndef _STDLIB_H_
inline int abs(int x) {return x > 0 ? x : -x;}
#endif
inline int clamp(int x, int minval, int maxval) {return x < minval ? minval : (x > maxval ? maxval : x);}
inline int limit(int x, int size) {return x < 0 ? 0 : (x >= size ? size - 1 : x);}
