#pragma once

#include "String.hpp"
#include "convert.hpp"
#include "util.hpp"


// decimal numbers
template <typename T>
struct Dec {
	T value;
	int digitCount;
};
template <typename T>
Dec<T> dec(T value, int digitCount = 1) {
	return {value, digitCount};
}

// binary coded decimal numbers
template <typename T>
struct Bcd {
	T value;
	int digitCount;
};
template <typename T>
Bcd<T> bcd(T value, int digitCount = 1) {
	return {value, digitCount};
}

// hexadecimal numbers
template <typename T>
struct Hex {
	T value;
	int digitCount;
};
template <typename T>
Hex<T> hex(T value, int digitCount = sizeof(T) * 2) {
	return {value, digitCount};
}

// floating point numbers
struct Flt {
	float value;
	int digitCount;
	int decimalCount;
};
constexpr Flt flt(float value, int decimalCount = 3) {
	return {value, 1, decimalCount};
}
constexpr Flt flt(float value, int digitCount, int decimalCount) {
	return {value, digitCount, decimalCount};
}

/**
 * String buffer with fixed maximum length
 */
template <int L>
class StringBuffer {
public:
	StringBuffer() : index(0) {}
	
	void clear() {this->index = 0;}

	StringBuffer &operator <<(char ch) {
		if (this->index < L)
			this->data[this->index++] = ch;
		this->data[this->index] = 0;
		return *this;
	}

	StringBuffer &operator <<(const String &str) {
		int l = min(str.length, L - this->index);
		char const *src = str.begin();
		char *dst = this->data + this->index;
		for (int i = 0; i < l; ++i) {
			dst[i] = src[i];
		}
		this->index += l;
		this->data[this->index] = 0;
		return *this;
	}

	template <typename T>
	StringBuffer &operator <<(Dec<T> dec) {
		uint32_t value = dec.value;
		if (dec.value < 0) {
			if (this->index < L)
				this->data[this->index++] = '-';
			value = -dec.value;
		}
		this->index += toString(value, this->data + this->index, L - this->index, dec.digitCount);
		this->data[this->index] = 0;
		return *this;
	}

	StringBuffer &operator <<(int dec) {
		return operator <<(Dec<int>{dec, 1});
	}

	template <typename T>
	StringBuffer &operator <<(Bcd<T> bcd) {
		int end = bcd.digitCount * 4;
		while (end < 32 && bcd.value >> end != 0)
			end += 4;
		for (int i = end - 4; i >= 0 && this->index < L; i -= 4) {
			this->data[this->index++] = '0' + ((bcd.value >> i) & 0xf);
		}
		this->data[this->index] = 0;
		return *this;
	}

	template <typename T>
	StringBuffer &operator <<(Hex<T> hex) {
		this->index += hexToString(hex.value, this->data + this->index, L - this->index, hex.digitCount);
		this->data[this->index] = 0;
		return *this;
	}
	
	StringBuffer &operator <<(Flt flt) {
		this->index += toString(flt.value, this->data + this->index, L - this->index, flt.digitCount,
			flt.decimalCount);
		this->data[this->index] = 0;
		return *this;
	}

	char operator [](int index) const {return this->data[index];}
	char &operator [](int index) {return this->data[index];}

	operator String() {
		return {this->data, this->index};
	}
	String string() const {
		return {this->data, this->index};
	}

	bool empty() {return this->index == 0;}
	int length() {return this->index;}
	void setLength(int length) {this->index = length;}

	int indexOf(char ch, int startIndex = 0) {
		for (int i = startIndex; i < this->index; ++i) {
			if (this->data[i] == ch)
				return i;
		}
		return -1;
	}

	int lastIndexOf(char ch) {
		int i = this->index;
		while (i > 0) {
			--i;
			if (this->data[i] == ch)
				return i;
		}
		return -1;
	}

protected:

	char data[L + 1];
	int index = 0;
};
