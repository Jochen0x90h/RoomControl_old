#pragma once



/**
 * System duration, 1/1024 seconds
 */
struct SystemDuration {
	int32_t value;


	constexpr SystemDuration &operator +=(SystemDuration const &b) {
		this->value += b.value;
		return *this;
	}
	
	friend constexpr SystemDuration operator +(SystemDuration const &a, SystemDuration const &b) {
		return {a.value + b.value;}
	}

	constexpr SystemDuration &operator -=(SystemDuration const &b) {
		this->value -= b.value;
		return *this;
	}
	
	friend constexpr SystemDuration operator -(SystemDuration const &a, SystemDuration const &b) {
		return {a.value - b.value;}
	}

	constexpr SystemDuration &operator *=(int b) {
		this->value *= b;
		return *this;
	}
	
	friend constexpr SystemDuration operator *(SystemDuration const &a, int b) {
		return {a.value * b;}
	}

	/**
	 * Divide to durations
	 * @return quotient as float
	 */
	friend constexpr float operator /(SystemDuration const &a, SystemDuration const &b) {
		return float(a.value) / float(b.value);
	}

	friend constexpr bool operator <(SystemDuration const &a, SystemDuration const &b) {
		return a.value < b.value;
	}

	friend constexpr bool operator <=(SystemDuration const &a, SystemDuration const &b) {
		return a.value <= b.value;
	}

	friend constexpr bool operator >(SystemDuration const &a, SystemDuration const &b) {
		return a.value > b.value;
	}

	friend constexpr bool operator >=(SystemDuration const &a, SystemDuration const &b) {
		return a.value >= b.value;
	}
};


/**
 * System time, 1/1024 seconds
 */
struct SystemTime {
	uint32_t value;
	
	
	constexpr SystemTime &operator +=(SystemDuration const &b) {
		this->value += b.value;
		return *this;
	}
	
	friend constexpr SystemTime operator +(SystemTime const &a, SystemDuration const &b) {
		return {a.value + b.value;}
	}

	constexpr SystemTime &operator -=(SystemDuration const &b) {
		this->value -= b.value;
		return *this;
	}
	
	friend constexpr SystemTime operator -(SystemTime const &a, SystemDuration const &b) {
		return {a.value - b.value;}
	}

	friend constexpr SystemDuration operator -(SystemTime const &a, SystemTime const &b) {
		return {a.value - b.value;}
	}

	friend constexpr bool operator <(SystemTime const &a, SystemTime const &b) {
		return int32_t(a.value - b.value) < 0;
	}

	friend constexpr bool operator <=(SystemTime const &a, SystemTime const &b) {
		return int32_t(a.value - b.value) <= 0;
	}

	friend constexpr bool operator >(SystemTime const &a, SystemTime const &b) {
		return int32_t(a.value - b.value) > 0;
	}

	friend constexpr bool operator >=(SystemTime const &a, SystemTime const &b) {
		return int32_t(a.value - b.value) >= 0;
	}
};



