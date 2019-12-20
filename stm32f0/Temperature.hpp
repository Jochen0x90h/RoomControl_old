#pragma once


class Temperature {
public:
	
	/**
	 * Constructor: Min and max value in °C in 8:8 fixed point notation
	 */
	Temperature() {}
	
	void setTargetValue(uint8_t targetValue) {this->targetValue = uint16_t(targetValue) << 7;}
	uint8_t getCurrentValue() {return this->currentValue >> 7;}
	bool getState(bool state) {return this->state;}

	void update() {}
		
protected:
	
	uint16_t minValue = 10 << 8;
	uint16_t maxValue = 30 << 8;

	uint16_t targetValue = 0;
	uint16_t currentValue = 20 << 8;
	bool state = false;

};
