#pragma once

#include <stdint.h>


class Serial {
public:
	using Device = uint32_t;
	
	Serial(Device device);

	void send(const uint8_t *data, int length);
	
	// return number of bytes sent, -1 on error
	int getSentCount() {
		return 0;
	}
	
	void receive(uint8_t *data, int length);
	
	// return number of bytes received, -1 on error
	int getReceivedCount() {
		return 0;
	}
	
	void update() {
	}

protected:

};
