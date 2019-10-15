#pragma once


// decimal numbers
template <typename T>
struct Decimal {
	Decimal(T value, int digitCount) : value(value), digitCount(digitCount) {}
	T value;
	int digitCount;
};
template <typename T>
Decimal<T> decimal(T value, int digitCount = 1) {
	return Decimal<T>(value, digitCount);
}

// binary coded decimal numbers
template <typename T>
struct Bcd {
	Bcd(T value, int digitCount) : value(value), digitCount(digitCount) {}
	T value;
	int digitCount;
};
template <typename T>
Bcd<T> bcd(T value, int digitCount = 1) {
	return Bcd<T>(value, digitCount);
}

// hexadecimal numbers
template <typename T>
struct Hex {
	Hex(T value) : value(value) {}
	T value;
};
template <typename T>
Hex<T> hex(T value) {
	return Hex<T>(value);
}

static const char *hexTable = "0123456789ABCDEF";

/**
 * String buffer with fixed maximum length
 */
template <int L>
class String {
public:
	String() {}
	
	template <typename T>
	String &operator = (const T &value) {
		this->index = 0;
		return operator , (value);
	}

	template <typename T>
	String &operator += (const T &value) {
		return operator , (value);
	}

	String &operator , (char ch) {
		if (this->index < L)
			this->buffer[this->index++] = ch;
		this->buffer[this->index] = 0;
		return *this;
	}

/*
	String &operator , (const char* s) {
		while (*s != 0 && this->index < L) {
			this->buffer[this->index++] = *(s++);
		}
		this->buffer[this->index] = 0;
		return *this;
	}
*/

	template <int N>
	String &operator , (const char (&array)[N]) {
		for (int i = 0; i < N && this->index < L; ++i) {
			char ch = array[i];
			if (ch == 0)
				break;
			this->buffer[this->index++] = ch;
		}
		this->buffer[this->index] = 0;
		return *this;
	}

	template <typename T>
	String &operator , (Decimal<T> decimal) {
		char buffer[12];

		unsigned int value = decimal.value < 0 ? -decimal.value : decimal.value;

		char * b = buffer + 11;
		*b = 0;
		int digitCount = decimal.digitCount;
		while (value > 0 || digitCount > 0) {
			--b;
			*b = '0' + value % 10;
			value /= 10;
			--digitCount;
		};

		if (decimal.value < 0) {
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

	String &operator , (int dec) {
		return operator , (Decimal<int>(dec, 1));
	}

	template <typename T>
	String &operator , (Bcd<T> bcd) {
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
	String &operator , (Hex<T> hex) {
		for (int i = sizeof(T) * 8 - 4; i >= 0 && this->index < L; i -= 4) {
			this->buffer[this->index++] = hexTable[(hex.value >> i) & 0xf];
		}
		this->buffer[this->index] = 0;
		return *this;
	}

	operator const char * () {
		return this->buffer;
	}
	
	int length() {return this->index;}

protected:
	char buffer[L + 1];
	int index = 0;
};
