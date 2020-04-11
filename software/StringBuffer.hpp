#pragma once

#include "String.hpp"


// decimal numbers
template <typename T>
struct Decimal {
	T value;
	int digitCount;
};
template <typename T>
Decimal<T> decimal(T value, int digitCount = 1) {
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
};
template <typename T>
Hex<T> hex(T value) {
	return {value};
}

static const char *hexTable = "0123456789ABCDEF";

/**
 * String buffer with fixed maximum length
 */
template <int L>
class StringBuffer {
public:
	StringBuffer() {}
	
	void clear() {this->index = 0;}
	/*
	template <typename T>
	StringBuffer &operator = (const T &value) {
		this->index = 0;
		return operator , (value);
	}

	template <typename T>
	StringBuffer &operator += (const T &value) {
		return operator , (value);
	}
*/
	StringBuffer &operator << (char ch) {
		if (this->index < L)
			this->buffer[this->index++] = ch;
		this->buffer[this->index] = 0;
		return *this;
	}

/*
	StringBuffer &operator , (const char* s) {
		while (*s != 0 && this->index < L) {
			this->buffer[this->index++] = *(s++);
		}
		this->buffer[this->index] = 0;
		return *this;
	}


	template <int N>
	StringBuffer &operator , (const char (&string)[N]) {
		for (int i = 0; i < N && this->index < L; ++i) {
			char ch = string[i];
			if (ch == 0)
				break;
			this->buffer[this->index++] = ch;
		}
		this->buffer[this->index] = 0;
		return *this;
	}
*/
	StringBuffer &operator << (const String &str) {
		for (int i = 0; i < str.length && this->index < L; ++i) {
			char ch = str.data[i];
			if (ch == 0)
				break;
			this->buffer[this->index++] = ch;
		}
		this->buffer[this->index] = 0;
		return *this;
	}

	template <typename T>
	StringBuffer &operator << (Decimal<T> dec) {
		char buffer[12];

		unsigned int value = dec.value < 0 ? -dec.value : dec.value;

		char * b = buffer + 11;
		*b = 0;
		int digitCount = dec.digitCount;
		while (value > 0 || digitCount > 0) {
			--b;
			*b = '0' + value % 10;
			value /= 10;
			--digitCount;
		};

		if (dec.value < 0) {
			--b;
			*b = '-';
		}

		// append the number
		while (*b != 0 && this->index < L) {
			this->buffer[this->index++] = *(b++);
		}
		this->buffer[this->index] = 0;
		return *this;
	}

	StringBuffer &operator << (int dec) {
		return operator << (Decimal<int>{dec, 1});
	}

	template <typename T>
	StringBuffer &operator << (Bcd<T> bcd) {
		int end = bcd.digitCount * 4;
		while (end < 32 && bcd.value >> end != 0)
			end += 4;
		for (int i = end - 4; i >= 0 && this->index < L; i -= 4) {
			this->buffer[this->index++] = '0' + ((bcd.value >> i) & 0xf);
		}
		this->buffer[this->index] = 0;
		return *this;
	}

	template <typename T>
	StringBuffer &operator << (Hex<T> hex) {
		for (int i = sizeof(T) * 8 - 4; i >= 0 && this->index < L; i -= 4) {
			this->buffer[this->index++] = hexTable[(hex.value >> i) & 0xf];
		}
		this->buffer[this->index] = 0;
		return *this;
	}

	operator String () {
		return {this->buffer, this->index};
	}
	
	bool empty() {return this->index == 0;}
	int length() {return this->index;}

protected:
	char buffer[L + 1];
	int index = 0;
};
