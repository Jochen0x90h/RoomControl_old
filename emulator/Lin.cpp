#include "Lin.hpp"
#include "util.hpp"
#include <iostream>

static Lin::Device devices[] = {
	{0xFEDCBA98, Lin::Device::SWITCH2, 0x33},
	{0x12345678, Lin::Device::SWITCH2, 0x77},
};


Lin::Lin(Parameters parameters) : gui(parameters.gui) {
	// post onLinReady() into event loop
	boost::asio::post(global::context, [this] {
		onLinReady();
	});
	
	std::fill(std::begin(this->states), std::end(this->states), 0);
}

Lin::~Lin() {
}

Array<Lin::Device> Lin::getLinDevices() {
	return {devices, size(devices)};
}

void Lin::linSend(uint32_t deviceId, const uint8_t *data, int length) {
	assert(length <= sizeof(8));
	for (int i = 0; i < size(devices); ++i) {
		Device const &device = devices[i];
		
		if (device.id == deviceId && length >= 1)
			this->states[i] = data[0];
	}
	this->sendBusy = true;
}

void Lin::doGui(int &id) {
	this->sendBusy = false;
	for (int i = 0; i < size(devices); ++i) {
		Device const &device = devices[i];
		uint8_t state = this->states[i];
		
		if (device.type == Device::SWITCH2) {
			uint8_t flags = device.flags;
			
			if ((flags & 0x11) != 0) {
				int rocker = this->gui.doubleRocker(id++);
				if (rocker != -1) {
					std::cout << rocker << std::endl;
					this->receiveData[0] = uint8_t(rocker);
					onLinReceived(device.id, this->receiveData, 1);
				}
			}
			
			int a = (flags >> 1) & 0x03;
			if (a == 3) {
				this->gui.blind(50);
			} else if (a >= 1) {
				this->gui.light(state & 1, 100);
				if (a == 2)
					this->gui.light(state & 2, 100);
			}
			
			int b = (flags >> 5) & 0x03;
			if (b == 3) {
				this->gui.blind(50);
			} else if (b >= 1) {
				this->gui.light(state & 4, 100);
				if (b == 2)
					this->gui.light(state & 8, 100);
			}
		}
		
		this->gui.newLine();
	}
}
