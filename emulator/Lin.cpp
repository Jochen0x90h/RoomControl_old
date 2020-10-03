#include "Lin.hpp"
#include "util.hpp"
#include <iostream>

static Lin::Device devices[] = {
	{0x00000001, Lin::Device::SWITCH2, 0x9f},
	{0x00000002, Lin::Device::SWITCH2, 0xff},
};


Lin::Lin(Parameters parameters) : gui(parameters.gui) {
	// post onLinReady() into event loop
	boost::asio::post(global::context, [this] {
		onLinReady();
	});
	
	std::fill(std::begin(this->states), std::end(this->states), State{0, 0, 0});
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
			this->states[i].relays = data[0];
	}
	this->sendBusy = true;
}

void Lin::doGui(int &id) {
	// time difference
	auto now = std::chrono::steady_clock::now();
	int ms = std::chrono::duration_cast<std::chrono::microseconds>(now - this->time).count();
	this->time = now;


	this->sendBusy = false;
	for (int i = 0; i < size(devices); ++i) {
		Device const &device = devices[i];
		State &state = this->states[i];

		// update blinds
		if (state.relays & 1)
			state.blindX = std::min(state.blindX + ms, 100*65536);
		else if (state.relays & 2)
			state.blindX = std::max(state.blindX - ms, 0);
		if (state.relays & 4)
			state.blindY = std::min(state.blindY + ms, 100*65536);
		else if (state.relays & 8)
			state.blindY = std::max(state.blindY - ms, 0);

		if (device.type == Device::SWITCH2) {
			uint8_t flags = device.flags;
			
			if ((flags & 0x0f) != 0) {
				int rocker = this->gui.doubleRocker(id++);
				if (rocker != -1) {
					std::cout << rocker << std::endl;
					this->receiveData[0] = uint8_t(rocker);
					onLinReceived(device.id, this->receiveData, 1);
				}
			}
			
			int x = (flags >> 4) & 0x03;
			if (x == 3) {
				this->gui.blind(state.blindX >> 16);
			} else if (x >= 1) {
				this->gui.light(state.relays & 1, 100);
				if (x == 2)
					this->gui.light(state.relays & 2, 100);
			}
			
			int y = (flags >> 6) & 0x03;
			if (y == 3) {
				this->gui.blind(state.blindY >> 16);
			} else if (y >= 1) {
				this->gui.light(state.relays & 4, 100);
				if (y == 2)
					this->gui.light(state.relays & 8, 100);
			}
		}
		
		this->gui.newLine();
	}
}
