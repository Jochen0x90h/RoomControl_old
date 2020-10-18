#pragma once

#include "defines.hpp"


/**
 * System duration, 1/1024 seconds
 */
struct SystemDuration {
	int32_t value;

	/**
	 * Convert duration to seconds
	 * @return duration in seconds
	 */
	int toSeconds() const {
		return value >> 10;
	}

	constexpr SystemDuration &operator +=(SystemDuration const &b) {
		this->value += b.value;
		return *this;
	}
	
	constexpr SystemDuration &operator -=(SystemDuration const &b) {
		this->value -= b.value;
		return *this;
	}
	
	constexpr SystemDuration &operator *=(int b) {
		this->value *= b;
		return *this;
	}
	
};

constexpr SystemDuration operator +(SystemDuration const &a, SystemDuration const &b) {
	return {a.value + b.value};
}

constexpr SystemDuration operator -(SystemDuration const &a, SystemDuration const &b) {
	return {a.value - b.value};
}

constexpr SystemDuration operator *(SystemDuration const &a, int b) {
	return {a.value * b};
}

constexpr SystemDuration operator *(SystemDuration const &a, float b) {
	return {int(float(a.value) * b + (a.value < 0 ? -0.5f : 0.5f))};
}

constexpr SystemDuration operator /(SystemDuration const &a, int b) {
	return {a.value / b};
}

/**
 * Divide to durations
 * @return quotient as float
 */
constexpr float operator /(SystemDuration const &a, SystemDuration const &b) {
	return float(a.value) / float(b.value);
}

constexpr bool operator <(SystemDuration const &a, SystemDuration const &b) {
	return a.value < b.value;
}

constexpr bool operator <=(SystemDuration const &a, SystemDuration const &b) {
	return a.value <= b.value;
}

constexpr bool operator >(SystemDuration const &a, SystemDuration const &b) {
	return a.value > b.value;
}

constexpr bool operator >=(SystemDuration const &a, SystemDuration const &b) {
	return a.value >= b.value;
}



/**
 * System time, 1/1024 seconds
 */
struct SystemTime {
	uint32_t value;
		
	constexpr SystemTime &operator +=(SystemDuration const &b) {
		this->value += b.value;
		return *this;
	}
	
	constexpr SystemTime &operator -=(SystemDuration const &b) {
		this->value -= b.value;
		return *this;
	}
};

constexpr SystemTime operator +(SystemTime const &a, SystemDuration const &b) {
	return {a.value + b.value};
}

constexpr SystemTime operator -(SystemTime const &a, SystemDuration const &b) {
	return {a.value - b.value};
}

constexpr SystemDuration operator -(SystemTime const &a, SystemTime const &b) {
	return {int32_t(a.value - b.value)};
}

constexpr bool operator <(SystemTime const &a, SystemTime const &b) {
	return int32_t(a.value - b.value) < 0;
}

constexpr bool operator <=(SystemTime const &a, SystemTime const &b) {
	return int32_t(a.value - b.value) <= 0;
}

constexpr bool operator >(SystemTime const &a, SystemTime const &b) {
	return int32_t(a.value - b.value) > 0;
}

constexpr bool operator >=(SystemTime const &a, SystemTime const &b) {
	return int32_t(a.value - b.value) >= 0;
}


constexpr SystemTime min(SystemTime x, SystemTime y) {return {x.value < y.value ? x.value : y.value};}

constexpr SystemDuration operator "" ms(unsigned long long ms) {
	return {int32_t((ms * 128 + 62) / 125)};
}

constexpr SystemDuration operator "" s(unsigned long long s) {
	return {int32_t(s * 1024)};
}
