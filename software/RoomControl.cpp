//#include <iostream>
#include "RoomControl.hpp"

// font
#include "tahoma_8pt.hpp"

constexpr String weekdays[7] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
constexpr String weekdaysShort[7] = {"M", "T", "W", "T", "F", "S", "S"};
constexpr String buttonStatesLong[] = {"release", "press"};
constexpr String switchStatesLong[] = {"off", "on"};
constexpr String rockerStatesLong[] = {"release", "up", "down", String()};

constexpr String buttonStates[] = {"#", "!", String()};
constexpr String switchStates[] = {"0", "1"};
constexpr String rockerStates[] = {"#", "+", "-", String()};

constexpr int8_t QOS = 1;


struct EndpointInfo {
	// type of endpoint
	EndpointType type;
	
	// name of endpoint (used in gui)
	String name;
	
	// number of array elements
	uint8_t elementCount;
};

constexpr EndpointInfo endpointInfos[] = {
	{EndpointType::BINARY_SENSOR, "Binary Sensor", 0},
	{EndpointType::BUTTON, "Button", 0},
	{EndpointType::SWITCH, "Switch", 0},
//	{EndpointType::BINARY_SENSOR_2, "2 x Binary Sensor", 2},
//	{EndpointType::BUTTON_2, "2 x Button", 2},
//	{EndpointType::SWITCH_2, "2 x Switch", 2},
	{EndpointType::ROCKER, "Rocker", 2},
//	{EndpointType::BINARY_SENSOR_4, "4 x Binary Sensor", 4},
//	{EndpointType::BUTTON_4, "4 x Button", 4},
//	{EndpointType::SWITCH_4, "4 x Switch", 4},
//	{EndpointType::ROCKER_2, "2 x Rocker", 4},

	{EndpointType::RELAY, "Relay", 0},
	{EndpointType::LIGHT, "Light", 0},
//	{EndpointType::RELAY_2, "2 x Relay", 2},
//	{EndpointType::LIGHT_2, "2 x Light", 2},
	{EndpointType::BLIND, "Blind", 2},
//	{EndpointType::RELAY_4, "4 x Relay", 4},
	
	{EndpointType::TEMPERATURE_SENSOR, "Temperature Sensor", 0},
};

EndpointInfo const *findEndpointInfo(EndpointType type) {
	int l = 0;
	int h = array::size(endpointInfos) - 1;
	while (l < h) {
		int mid = l + (h - l) / 2;
		if (endpointInfos[mid].type < type) {
			l = mid + 1;
		} else {
			h = mid;
		}
	}
	EndpointInfo const *endpointInfo = &endpointInfos[l];
	return endpointInfo->type == type ? endpointInfo : nullptr;
}


struct ClassInfo {
	uint8_t size;
	uint8_t classFlags;
};

struct ComponentInfo {
	// component has an element index
	static constexpr uint8_t ELEMENT_INDEX_1 = 1;
	static constexpr uint8_t ELEMENT_INDEX_2 = 2;

	// component has a configurable duration (derives from DurationElement)
	static constexpr uint8_t DURATION = 4;

	// component can receive commands (state derives from CommandElement)
	static constexpr uint8_t COMMAND = 8;


	RoomControl::Device::Component::Type type;
	String name;
	uint8_t shortNameIndex;//String shortName;
	ClassInfo flash;
	ClassInfo ram;
	uint8_t elementIndexStep;
};

template <typename T>
constexpr ClassInfo makeClassInfo() {
	return {uint8_t((sizeof(T) + 3) / 4), uint8_t(T::CLASS_FLAGS)};
}

constexpr String componentShortNames[] = {
	"bt",
	"sw",
	"rk",
	"rl",
	"bl",
	"tc",
	"tf"
};

constexpr int BT = 0;
constexpr int SW = 1;
constexpr int RK = 2;
constexpr int RL = 3;
constexpr int BL = 4;
constexpr int TC = 5;
constexpr int TF = 6;

constexpr ComponentInfo componentInfos[] = {
	{RoomControl::Device::Component::BUTTON, "Button", BT,
		makeClassInfo<RoomControl::Device::Button>(),
		makeClassInfo<RoomControl::DeviceState::Button>(),
		1},
	{RoomControl::Device::Component::HOLD_BUTTON, "Hold Button", BT,
		makeClassInfo<RoomControl::Device::HoldButton>(),
		makeClassInfo<RoomControl::DeviceState::HoldButton>(),
		1},
	{RoomControl::Device::Component::DELAY_BUTTON, "Delay Button", BT,
		makeClassInfo<RoomControl::Device::DelayButton>(),
		makeClassInfo<RoomControl::DeviceState::DelayButton>(),
		1},
	{RoomControl::Device::Component::SWITCH, "Switch", SW,
		makeClassInfo<RoomControl::Device::Switch>(),
		makeClassInfo<RoomControl::DeviceState::Switch>(),
		1},
	{RoomControl::Device::Component::ROCKER, "Rocker", RK,
		makeClassInfo<RoomControl::Device::Rocker>(),
		makeClassInfo<RoomControl::DeviceState::Rocker>(),
		2},
	{RoomControl::Device::Component::HOLD_ROCKER, "Hold Rocker", RK,
		makeClassInfo<RoomControl::Device::HoldRocker>(),
		makeClassInfo<RoomControl::DeviceState::HoldRocker>(),
		2},
	{RoomControl::Device::Component::RELAY, "Relay", RL,
		makeClassInfo<RoomControl::Device::Relay>(),
		makeClassInfo<RoomControl::DeviceState::Relay>(),
		0},
	{RoomControl::Device::Component::TIME_RELAY, "Time Relay", RL,
		makeClassInfo<RoomControl::Device::TimeRelay>(),
		makeClassInfo<RoomControl::DeviceState::TimeRelay>(),
		0},
	{RoomControl::Device::Component::BLIND, "Blind", BL,
		makeClassInfo<RoomControl::Device::Blind>(),
		makeClassInfo<RoomControl::DeviceState::Blind>(),
		0},
	{RoomControl::Device::Component::CELSIUS, "Celsius", TC,
		makeClassInfo<RoomControl::Device::TemperatureSensor>(),
		makeClassInfo<RoomControl::DeviceState::TemperatureSensor>(),
		0},
	{RoomControl::Device::Component::FAHRENHEIT, "Fahrenheit", TF,
		makeClassInfo<RoomControl::Device::TemperatureSensor>(),
		makeClassInfo<RoomControl::DeviceState::TemperatureSensor>(),
		0}
};


RoomControl::RoomControl()
	: storage(0, FLASH_PAGE_COUNT, room, devices, routes, timers)
{
	if (this->room.size() == 0) {
		assign(this->temp.room.name, "room");
		this->room.write(0, &this->temp.room);
	}
	
	// subscribe to mqtt broker (has to be repeated when connection to gateway is established)
	subscribeAll();
	
	// start device update
	SystemTime time = getSystemTime();
	this->lastUpdateTime = time;
	this->nextReportTime = time;
	onSystemTimeout3(time);
}

RoomControl::~RoomControl() {
}

// UpLink
// ------

void RoomControl::onUpConnected() {
	connect("MyClient");
}


// MqttSnClient
// ------------

void RoomControl::onConnected() {
	std::cout << "onConnected" << std::endl;

	// subscribe devices at gateway
	subscribeAll();
}

void RoomControl::onDisconnected() {

}

void RoomControl::onSleep() {

}

void RoomControl::onWakeup() {

}

void RoomControl::onError(int error, mqttsn::MessageType messageType) {

}


// MqttSnBroker
// ------------

void RoomControl::onPublished(uint16_t topicId, uint8_t const *data, int length, int8_t qos, bool retain) {
	String message(data, length);

	std::cout << "onPublished " << topicId << " message " << message << " qos " << int(qos);
	if (retain)
		std::cout << " retain";
	std::cout << std::endl;


	// topic list of house, room or device (depending on topicDepth)
	if (topicId == this->selectedTopicId && message != "?") {
		int space = message.indexOf(' ', 0, message.length);
		if (this->topicDepth < 2 || !this->onlyCommands || message.substring(space + 1) == "c")
			this->topicSet.add(message.substring(0, space));
		return;
	}

	// global commands
	RoomState const &roomState = *this->room[0].ram;

	// check for request to list rooms in the house
	if (topicId == roomState.houseTopicId && message == "?") {
		// publish room name
		publish(roomState.houseTopicId, getRoomName(), QOS);
		return;
	}

	// check for request to list devices in this room
	if (topicId == roomState.roomTopicId && message == "?") {
		// publish device names
		// todo: do async
		for (int i = 0; i < this->devices.size(); ++i) {
			auto e = this->devices[i];
			publish(roomState.roomTopicId, e.flash->getName(), QOS);
		}
		return;
	}

	// check for routing
	for (auto e : this->routes) {
		RouteState const &routeState = *e.ram;
		if (topicId == routeState.srcTopicId) {
			std::cout << "route " << topicId << " -> " << routeState.dstTopicId << std::endl;
			publish(routeState.dstTopicId, data, length, qos);
		}
	}

	SystemTime time = getSystemTime();
	//std::cout << "time1: " << time.value << std::endl;
	
	// update time dependent state of devices (e.g. blind position)
	SystemTime nextTimeout = updateDevices(time);

	// iterate over bus devices
	for (DeviceElement e : this->devices) {
		Device const &device = *e.flash;
		DeviceState &deviceState = *e.ram;

		// check for request to list attributes in this device
		if (topicId == deviceState.deviceTopicId && message == "?") {
			ComponentIterator it(device, deviceState);
			while (!it.atEnd()) {
				StringBuffer<8> b = it.getComponent().getName();
				publish(deviceState.deviceTopicId, b, QOS);
				it.next();
			}
		} else {
			// group event to transfer up/down states to virtual button/rocker
			uint8_t outEndpointId = 0;
			uint8_t outData = 0;

			// iterate over device components
			ComponentIterator it(device, deviceState);
			while (!it.atEnd()) {
				auto &component = it.getComponent();
				auto &state = it.getState();
				switch (component.type) {
				case Device::Component::BUTTON:
					if (outEndpointId != 0) {
						// get button state
						uint8_t value = (outData >> component.elementIndex) & 1;
						if (value != state.flags) {
							state.flags = value;
							publish(state.statusTopicId, buttonStates[value], QOS);
						}
					}
					break;
				case Device::Component::HOLD_BUTTON:
				case Device::Component::DELAY_BUTTON:
					break;
				case Device::Component::SWITCH:
					if (outEndpointId != 0) {
						// get switch state
						uint8_t value = (outData >> component.elementIndex) & 1;
						if (value != state.flags) {
							state.flags = value;
							publish(state.statusTopicId, switchStates[value], QOS);
						}
					}
					break;
				case Device::Component::ROCKER:
					if (outEndpointId != 0) {
						// get rocker state
						uint8_t value = (outData >> component.elementIndex) & 3;
						if (value != state.flags) {
							state.flags = value;
							publish(state.statusTopicId, rockerStates[value], QOS);
						}
					}
					break;
				case Device::Component::HOLD_ROCKER:
					break;
				case Device::Component::RELAY:
				case Device::Component::TIME_RELAY:
					{
						auto &relayState = state.cast<DeviceState::Relay>();
						uint8_t relay = relayState.flags & DeviceState::Relay::RELAY;
						if (topicId == relayState.commandTopicId) {
							//bool lastOn = (relayState.flags & DeviceState::Relay::RELAY) != 0;
							bool on = relay != 0;

							// try to parse float value
							optional<float> value = parseFloat(message);
							if (value != null) {
								on = *value != 0.0f;
								relayState.flags &= ~DeviceState::Relay::PRESSED;
							} else {
								char command = length == 1 ? data[0] : 0;
								bool toggle = command == '!';
								bool up = command == '+';
								bool down = command == '-';
								bool release = command == '#';
								bool pressed = (relayState.flags & DeviceState::Blind::PRESSED) != 0;

								if (release) {
									// release
									relayState.flags &= ~DeviceState::Blind::PRESSED;
								} else if (!pressed || !(toggle || up || down)) {
									// stop when not pressed or unrecognized command (e.g. empty message)
									on = up || (toggle && !on);
									relayState.flags &= ~DeviceState::Blind::PRESSED;
								}
							}
							relay = uint8_t(on);

							// publish relay state if it has changed
							if (relay != (relayState.flags & DeviceState::Relay::RELAY)) {
								state.flags ^= DeviceState::Relay::RELAY;
								//uint8_t relay = uint8_t(on);
								//busSend(relayState.endpointId, &value, 1);
								outEndpointId = state.endpointId;

								publish(relayState.statusTopicId, switchStates[relay], QOS);
							
								// set timeout
								if (on && component.type == Device::Component::TIME_RELAY) {
									auto &timeRelay = component.cast<Device::TimeRelay>();
									auto &timeRelayState = state.cast<DeviceState::TimeRelay>();
								
									// set on timeout
									timeRelayState.timeout = time + timeRelay.duration;
								}
							}
							
							// set next timeout if currently on
							if (on && component.type == Device::Component::TIME_RELAY)
								nextTimeout = min(nextTimeout, state.cast<DeviceState::TimeRelay>().timeout);
						}
						outData |= relay << component.elementIndex;
					}
					break;
				case Device::Component::BLIND:
					{
						auto &blind = component.cast<Device::Blind>();
						auto &blindState = state.cast<DeviceState::Blind>();
						uint8_t relays = state.flags & DeviceState::Blind::RELAYS;
						if (topicId == blindState.commandTopicId) {
							// try to parse float value
							optional<float> value = parseFloat(message);
							if (value != null) {
								// move to target position in the range [0, 1]
								// todo: maybe prevent instant direction change
								if (*value >= 1.0f) {
									// move up with extra time
									blindState.timeout = time + blind.duration - blindState.duration + 500ms;
									relays = DeviceState::Blind::RELAY_UP;
								} else if (*value <= 0.0f) {
									// move down with extra time
									blindState.timeout = time + blind.duration + 500ms;
									relays = DeviceState::Blind::RELAY_DOWN;
								} else {
									// move to target position
									SystemDuration targetDuration = blind.duration * *value;
									if (targetDuration > blindState.duration) {
										// move up
										blindState.timeout = time + targetDuration - blindState.duration;
										relays = DeviceState::Blind::RELAY_UP;
										blindState.flags |= DeviceState::Blind::DIRECTIION_UP;
									} else {
										// move down
										blindState.timeout = time + blindState.duration - targetDuration;
										relays = DeviceState::Blind::RELAY_DOWN;
										blindState.flags &= ~DeviceState::Blind::DIRECTIION_UP;
									}
								}
								blindState.flags &= ~DeviceState::Blind::PRESSED;
							} else {
								char command = length == 1 ? data[0] : 0;
								bool toggle = command == '!';
								bool up = command == '+';
								bool down = command == '-';
								bool release = command == '#';
								bool pressed = (blindState.flags & DeviceState::Blind::PRESSED) != 0;
								
								if (relays != 0) {
									// currently moving
									if (release) {
										// release
										blindState.flags &= ~DeviceState::Blind::PRESSED;
									} else if (!pressed || !(toggle || up || down)) {
										// stop when not pressed or unrecognized command (e.g. empty message)
										relays = 0;
										blindState.flags &= ~DeviceState::Blind::PRESSED;
									}
								} else {
									// currently stopped: start on toggle, up and down
									bool directionUp = (blindState.flags & DeviceState::Blind::DIRECTIION_UP) != 0;
									if (up || (toggle && !directionUp)) {
										// up with extra time
										blindState.timeout = time + blind.duration - blindState.duration + 500ms;
										relays = DeviceState::Blind::RELAY_UP;
										blindState.flags |= DeviceState::Blind::DIRECTIION_UP | DeviceState::Blind::PRESSED;
									} else if (down || (toggle && directionUp)) {
										// down with extra time
										blindState.timeout = time + blindState.duration + 500ms;
										relays = DeviceState::Blind::RELAY_DOWN;
										blindState.flags &= ~DeviceState::Blind::DIRECTIION_UP;
										blindState.flags |= DeviceState::Blind::PRESSED;
									}
								}
							}
							
							// send relay state to bus device if it has changed
							if (relays != (state.flags & DeviceState::Blind::RELAYS)) {
								state.flags = (state.flags & ~DeviceState::Blind::RELAYS) | relays;
								//busSend(blindState.endpointId, &relays, 1);
								outEndpointId = state.endpointId;
								
								if (relays == 0) {
									// stopped: publish current blind position
									float pos = blindState.duration / blind.duration;
									//std::cout << "pos " << pos << std::endl;
									StringBuffer<8> b = flt(pos, 0, 3);
									publish(blindState.statusTopicId, b, QOS, true);
								}
							}
							
							// set next timeout if currently moving
							if (relays != 0)
								nextTimeout = min(nextTimeout, blindState.timeout);
						}
						outData |= relays << component.elementIndex;
					}
					break;
				case Device::Component::CELSIUS:
				case Device::Component::FAHRENHEIT:
					break;
				}
				if (it.next()) {
					if (outEndpointId != 0) {
						busSend(outEndpointId, &outData, 1);
						outEndpointId = 0;
					}
					outData = 0;
				}
			}
		}
	}
	//std::cout << "next1: " << nextTimeout.value << std::endl;
	setSystemTimeout3(nextTimeout);
}


// Bus
// ---

void RoomControl::onBusReady() {
	for (uint32_t deviceId : getBusDevices()) {
		// check if the device is registered
		for (auto e : this->devices) {
			Device const &device = *e.flash;
			DeviceState &deviceState = *e.ram;
		
			if (device.deviceId == deviceId) {
				// found device, now subscribe to endpoints
				Array<EndpointType> endpoints = getBusDeviceEndpoints(deviceId);
				
				ComponentIterator it(device, deviceState);
				int componentIndex = 0;
				while (!it.atEnd()) {
					auto &component = it.getComponent();
					auto &state = it.getState();

					int endpointIndex = component.endpointIndex;
					if (endpointIndex < endpoints.length) {
						// check endpoint type compatibility
						if (isCompatible(endpoints[endpointIndex], /*componentIndex,*/ component.type)) {
							// subscribe
							subscribeBus(state.endpointId, deviceId, endpointIndex);
						}
					}
					
					// increment component index
					++componentIndex;
					
					// next element
					if (it.next())
						componentIndex = 0;
				}
				
				break;
			}
		}
	}
}

void RoomControl::onBusReceived(uint8_t endpointId, uint8_t const *data, int length) {
	SystemTime time = getSystemTime();
	//std::cout << "time1: " << time.value << std::endl;
	
	// update time dependent state of devices (e.g. blind position)
	SystemTime nextTimeout = updateDevices(time);
	
	// iterate over bus devices
	for (auto e : this->devices) {
		Device const &device = *e.flash;
		DeviceState &deviceState = *e.ram;

		ComponentIterator it(device, deviceState);
		while (!it.atEnd()) {
			auto &component = it.getComponent();
			auto &state = it.getState();
			
			if (endpointId == state.endpointId) {
				auto type = it.getComponent().type;
				switch (type) {
				case Device::Component::BUTTON:
					{
						// get button state
						uint8_t value = (data[0] >> component.elementIndex) & 1;
						if (value != state.flags) {
							state.flags = value;
							publish(state.statusTopicId, buttonStates[value], QOS);
						}
					}
					break;
				case Device::Component::HOLD_BUTTON:
					{
						// get button state
						uint8_t value = (data[0] >> component.elementIndex) & 1;
						if (value != state.flags) {
							state.flags = value;

							auto &holdButton = component.cast<Device::HoldButton>();
							auto &holdButtonState = state.cast<DeviceState::HoldButton>();
							if (value != 0) {
								// pressed: set timeout
								holdButtonState.timeout = time + holdButton.duration;
							} else {
								// released: publish "stop" instead of "release" when timeout has elapsed
								if (time >= holdButtonState.timeout)
									value = 2;
							}
							publish(state.statusTopicId, buttonStates[value], QOS);
						}
					}
					break;
				case Device::Component::DELAY_BUTTON:
					{
						// get button state
						uint8_t value = (data[0] >> component.elementIndex) & 1;
						if (value != state.flags) {
							uint8_t pressed = state.flags & 2;
							state.flags = value;

							auto &delayButton = component.cast<Device::DelayButton>();
							auto &delayButtonState = state.cast<DeviceState::HoldButton>();
							if (value != 0) {
								// pressed: set timeout, but not change state yet
								delayButtonState.timeout = time + delayButton.duration;
								nextTimeout = min(nextTimeout, delayButtonState.timeout);
							} else {
								// released: publish "release" if "press" was sent
								if (pressed != 0) {
									publish(state.statusTopicId, buttonStates[0], QOS);
								}
							}
						}
					}
					break;
				case Device::Component::SWITCH:
					{
						// get switch state
						uint8_t value = (data[0] >> component.elementIndex) & 1;
						if (value != state.flags) {
							state.flags = value;
							publish(state.statusTopicId, switchStates[value], QOS);
						}
					}
					break;
				case Device::Component::ROCKER:
					{
						// get rocker state
						uint8_t value = (data[0] >> component.elementIndex) & 3;
						if (value != state.flags) {
							state.flags = value;
							publish(state.statusTopicId, rockerStates[value], QOS);
						}
					}
					break;
				case Device::Component::HOLD_ROCKER:
					{
						// get rocker state
						uint8_t value = (data[0] >> component.elementIndex) & 3;
						if (value != state.flags) {
							state.flags = value;

							auto &holdRocker = component.cast<Device::HoldRocker>();
							auto &holdRockerState = state.cast<DeviceState::HoldRocker>();
							if (value != 0) {
								// up or down pressed: set timeout
								holdRockerState.timeout = time + holdRocker.duration;
							} else {
								// released: publish "stop" instead of "release" when timeout has elapsed
								if (time >= holdRockerState.timeout)
									value = 3;
							}
							publish(state.statusTopicId, rockerStates[value], QOS);
						}
					}
					break;
				case Device::Component::RELAY:
					break;
				case Device::Component::TIME_RELAY:
					break;
				case Device::Component::BLIND:
					break;
				case Device::Component::CELSIUS:
					{
						// convert temperature from 1/20 K to 1/10 °C
						int temperature = ((data[0] | data[1] << 8) - 5463) / 2;
						StringBuffer<6> b = dec(temperature / 10) + '.' + dec(temperature % 10);
						publish(state.statusTopicId, b, QOS);
					}
					break;
				case Device::Component::FAHRENHEIT:
					{
						// convert temperature from 1/20 K to 1/10 °F
						int temperature = ((data[0] | data[1] << 8) - 5463) * 9 / 10 + 320;
						StringBuffer<6> b = dec(temperature / 10) + '.' + dec(temperature % 10);
						publish(state.statusTopicId, b, QOS);
					}
					break;
				}
			}
			
			it.next();
		}
	}
	
	setSystemTimeout3(nextTimeout);
}

void RoomControl::onBusSent() {

}

	
// SystemTimer
// -----------
		
void RoomControl::onSystemTimeout3(SystemTime time) {
	//std::cout << "time: " << time.value << std::endl;
	SystemTime nextTimeout = updateDevices(time);
	//std::cout << "next: " << nextTimeout.value << std::endl;
	setSystemTimeout3(nextTimeout);
}


// Display
// -------

void RoomControl::onDisplayReady() {

}


// Poti
// ----

void RoomControl::onPotiChanged(int delta, bool activated) {
	updateMenu(delta, activated);
	setDisplay(this->bitmap);
}


// Menu
// ----

void RoomControl::updateMenu(int delta, bool activated) {
	// if menu entry was activated, read menu state from stack
	if (this->stackHasChanged) {
		this->stackHasChanged = false;
		this->state = this->stack[this->stackIndex].state;
		this->selected = this->stack[this->stackIndex].selected;
		this->selectedY = this->stack[this->stackIndex].selectedY;
		this->yOffset = this->stack[this->stackIndex].yOffset;

		// set entryIndex to a large value so that the value of this->selected "survives" first call to menu()
		this->entryIndex = 0xffff;
	}

	// clear bitmap
	this->bitmap.clear();

	// toast
	if (!this->buffer.empty() && getSystemTime() - this->toastTime < 3s) {
		String text = this->buffer;
		int y = 10;
		int len = tahoma_8pt.calcWidth(text, 1);
		this->bitmap.drawText((bitmap.WIDTH - len) >> 1, y, tahoma_8pt, text, 1);
		return;
	}

	// draw menu
	switch (this->state) {
	case IDLE:
		{
			// get current clock time
			ClockTime time = getClockTime();
						
			// display weekday and clock time
			StringBuffer<16> b = weekdays[time.getWeekday()] + "  "
				+ dec(time.getHours()) + ':'
				+ dec(time.getMinutes(), 2) + ':'
				+ dec(time.getSeconds(), 2);
			bitmap.drawText(20, 10, tahoma_8pt, b, 1);
/*
			// update target temperature
			int targetTemperature = this->targetTemperature = clamp(this->targetTemperature + delta, 10 << 1, 30 << 1);
			this->temperature.setTargetValue(targetTemperature);

			// get current temperature
			int currentTemperature = this->temperature.getCurrentValue();

			this->buffer = decimal(currentTemperature >> 1), (currentTemperature & 1) ? ".5" : ".0" , " oC";
			bitmap.drawText(20, 30, tahoma_8pt, this->buffer, 1);
			this->buffer = decimal(targetTemperature >> 1), (targetTemperature & 1) ? ".5" : ".0" , " oC";
			bitmap.drawText(70, 30, tahoma_8pt, this->buffer, 1);
*/
			// enter menu if poti-switch was pressed
			if (activated) {
				this->stack[0] = {MAIN, 0, 0, 0};
				this->stackHasChanged = true;
			}
		}
		break;
	case MAIN:
		menu(delta, activated);

		if (entry("Bus Devices"))
			push(BUS_DEVICES);
		if (entry("Routes"))
			push(ROUTES);
		if (entry("Timers"))
			push(TIMERS);
		if (entry("Exit")) {
			this->state = IDLE;
		}
		break;
	case BUS_DEVICES:
		menu(delta, activated);
		
		// list devices
		for (auto e : this->devices) {
			auto &device = *e.flash;
			
			StringBuffer<24> b = device.getName();
			if (entry(b)) {
				// edit device
				clone(this->temp.device, this->tempState.device, device, *e.ram);
				push(EDIT_BUS_DEVICE);
			}
		}

		// add device

		if (entry("Exit"))
			pop();
		break;
	case EDIT_BUS_DEVICE:
	case ADD_BUS_DEVICE:
		menu(delta, activated);
		{
			uint8_t editBegin[4];
			uint8_t editEnd[4];
			Device &device = this->temp.device;
			DeviceState &deviceState = this->tempState.device;

			// device name
			//todo: edit name
			label(device.getName());

			// device endpoints
			Array<EndpointType> endpoints = getBusDeviceEndpoints(device.deviceId);
			ComponentIterator it(device, deviceState);
			for (int endpointIndex = 0; endpointIndex < endpoints.length; ++endpointIndex) {
				line();
				
				// endpoint type
				auto endpointType = endpoints[endpointIndex];
				StringBuffer<24> b;
				auto endpointInfo = findEndpointInfo(endpointType);
				if (endpointInfo != nullptr)
					b += endpointInfo->name;
				else
					b += '?';
				label(b);
				
				// device components that are associated with the current device endpoint
				int componentIndex = 0;
				while (!it.atEnd() && it.getComponent().endpointIndex == endpointIndex) {
					auto &component = it.getComponent();
					ComponentInfo const &componentInfo = componentInfos[component.type];

					// menu entry for component
					b = component.getName() + ": " + componentInfo.name;
					if (entry(b)) {
						// set index
						this->tempIndex = it.componentIndex;

						// copy component
						auto dst = reinterpret_cast<uint32_t *>(&this->temp2.component);
						auto src = reinterpret_cast<uint32_t const *>(&component);
						array::copy(dst, dst + componentInfo.flash.size, src);

						// set component index just to be sure
						//this->temp2.component.componentIndex = componentIndex;
						
						push(EDIT_COMPONENT);
					}
					
					it.next();
					++componentIndex;
				}
				
				// add device component
				if (device.componentCount < Device::MAX_COMPONENT_COUNT) {
					if (entry("Add Component")) {
						// set index
						this->tempIndex = it.componentIndex;

						// select first valid type
						auto &component = this->temp2.component;
						component.init(endpointType/*, endpointIndex, componentIndex*/);
						component.endpointIndex = endpointIndex;
						component.nameIndex = device.getNameIndex(component.type, -1);

						// set default duration
						if (component.is<Device::TimeComponent>())
							this->temp2.timeComponent.duration = 1s;

						push(ADD_COMPONENT);
					}
				}
			}
			line();

			// save, delete, cancel
			if (entry("Save Device")) {
				int index = getThisIndex();
				
				// unsubscribe old command topics
				if (index < this->devices.size()) {
					auto e = this->devices[index];
					destroy(*e.flash, *e.ram);
				}
				this->devices.write(index, &device, &deviceState);
				pop();
			}
			if (this->state == EDIT_BUS_DEVICE) {
				if (entry("Delete Device")) {
					int index = getThisIndex();

					// unsubscribe new command topics
					destroy(device, deviceState);

					// unsubscribe old command topics
					auto e = this->devices[index];
					destroy(*e.flash, *e.ram);
					
					this->routes.erase(getThisIndex());
					pop();
				}
			}
			if (entry("Cancel")) {
				// unsubscribe new command topics
				destroy(device, deviceState);
				pop();
			}
		}
		break;
		
	case EDIT_COMPONENT:
	case ADD_COMPONENT:
		menu(delta, activated);
		{
			uint8_t editBegin[5];
			uint8_t editEnd[5];
			Device const &device = this->temp.device;
			Device::Component &component = this->temp2.component;
			
			// get endpoint type
			auto endpointType = getBusDeviceEndpoints(device.deviceId)[component.endpointIndex];
			auto endpointInfo = findEndpointInfo(endpointType);
			
			// edit component type
			int edit = getEdit(1);
			if (edit > 0 && delta != 0) {
				component.rotateType(endpointType, delta);
				component.nameIndex = device.getNameIndex(component.type, this->tempIndex);

				// set default values
				if (component.is<Device::TimeComponent>())
					this->temp2.timeComponent.duration = 1s;
			}
			ComponentInfo const &componentInfo = componentInfos[component.type];
			
			// menu entry for component type
			editBegin[1] = 0;
			StringBuffer<24> b = componentInfo.name;
			editEnd[1] = b.length();
			entry(b, edit > 0, editBegin[1], editEnd[1]);

			// name
			b = "Name: " + component.getName();
			label(b);
		
			// element index
			int step = componentInfo.elementIndexStep;
			if (step != 0 && endpointInfo != nullptr && endpointInfo->elementCount >= 2) {
				// edit element index
				int edit = getEdit(1);
				if (edit > 0) {
					int elementCount = endpointInfo->elementCount;
					component.elementIndex = (component.elementIndex + delta * step + elementCount) % elementCount;
				}

				// menu entry for element index
				b = "Index: ";
				editBegin[1] = b.length();
				b += dec(component.elementIndex);
				editEnd[1] = b.length();
				entry(b, edit > 0, editBegin[1], editEnd[1]);
			}
		
			// duration
			if (component.is<Device::TimeComponent>()) {
				SystemDuration duration = this->temp2.timeComponent.duration;
								
				// decompose duration
				int tenths = duration % 1s / 100ms;
				int seconds = int(duration / 1s) % 60;
				int minutes = int(duration / 1min) % 60;
				int hours = duration / 1h;
				
				// edit duration
				int edit = getEdit(hours == 0 ? 4 : 3);
				if (edit > 0) {
					if (delta != 0) {
						if (edit == 1) {
							hours = (hours + delta + 512) & 511;
							tenths = 0;
						}
						if (edit == 2)
							minutes = (minutes + delta + 60) % 60;
						if (edit == 3)
							seconds = (seconds + delta + 60) % 60;
						if (edit == 4) {
							tenths = (tenths + delta + 10) % 10;
						}
						this->temp2.timeComponent.duration = ((hours * 60 + minutes) * 60 + seconds) * 1s + tenths * 100ms;
					}
				}/* else if (duration < 100ms) {
					// enforce minimum duration
					this->temp2.timeComponent.duration = 100ms;
				}*/

				// menu entry for duration
				b = "Duration: ";
				editBegin[1] = b.length();
				b += dec(hours);
				editEnd[1] = b.length();
				b += ':';
				editBegin[2] = b.length();
				b += dec(minutes, 2);
				editEnd[2] = b.length();
				b += ':';
				editBegin[3] = b.length();
				b += dec(seconds, 2);
				editEnd[3] = b.length();
				if (hours == 0) {
					b += '.';
					editBegin[4] = b.length();
					b += dec(tenths);
					editEnd[4] = b.length();
				}
				entry(b, edit > 0, editBegin[edit], editEnd[edit]);
			}
		
			// save, delete, cancel
			if (entry("Save Component")) {
				// seek to component
				ComponentEditor editor(this->temp.device, this->tempState.device);
				while (editor.componentIndex < this->tempIndex)
					editor.next();

				auto &state = editor.getState();
				TopicBuffer topic = getRoomName() / device.getName();

				// set type or insert component
				if (this->state == EDIT_COMPONENT) {
					auto &oldComponent = editor.getComponent();

					// unregister old status topic
					unregisterTopic(state.statusTopicId);
					
					// unsubscribe from old command topic
					if (state.is<DeviceState::CommandComponent>(oldComponent)) {
						auto &oldCommandState = state.cast<DeviceState::CommandComponent>();
						StringBuffer<8> b = oldComponent.getName();
						topic /= b;
						unsubscribeTopic(oldCommandState.commandTopicId, topic.command());
						topic.removeLast();
					}
										
					// change component
					editor.changeType(component.type);
				} else {
					// add component
					editor.insert(component.type);
				
					// subscribe to endpoint
					subscribeBus(state.endpointId, device.deviceId, component.endpointIndex);
				}

				// copy component
				auto &newComponent = editor.getComponent();
				auto dst = reinterpret_cast<uint32_t *>(&newComponent);
				auto src = reinterpret_cast<uint32_t const *>(&component);
				array::copy(dst, dst + componentInfo.flash.size, src);

				// register new state topic
				StringBuffer<8> b = component.getName();
				topic /= b;
				registerTopic(state.statusTopicId, topic.status());

				// subscribe new command topic
				if (state.is<DeviceState::CommandComponent>(component)) {
					auto &commandState = state.cast<DeviceState::CommandComponent>();
					subscribeTopic(commandState.commandTopicId, topic.command(), QOS);
				}

				pop();
			}
			if (this->state == EDIT_COMPONENT) {
				if (entry("Delete Component")) {
					ComponentEditor editor(this->temp.device, this->tempState.device);
					while (editor.componentIndex < this->tempIndex)
						editor.next();
					
					
					//editor.erase();


					// unsubscribe old command topics

					pop();
				}
			}
			if (entry("Cancel")) {
				pop();
			}
		}
		break;

	case ROUTES:
		menu(delta, activated);
		{
			// list routes
			for (auto e : this->routes) {
				Route const &route = *e.flash;
				String srcTopic = route.getSrcTopic();
				String dstTopic = route.getDstTopic();
				if (entry(srcTopic)) {
					// edit route
					clone(this->temp.route, this->tempState.route, route, *e.ram);
					push(EDIT_ROUTE);
				}
				//label(dstTopic);
				//line();
			}
			
			// add route
			if (entry("Add Route")) {
				// check if list of routes is full
				if (this->routes.size() < MAX_ROUTE_COUNT) {
					// check if memory for a new route is available
					Route &route = this->temp.route;
					route.srcTopicLength = TopicBuffer::MAX_TOPIC_LENGTH * 3;
					route.dstTopicLength = 0;
					if (this->routes.hasSpace(&route)) {
						// clea
						route.srcTopicLength = 0;
						this->tempState.route.srcTopicId = 0;
						this->tempState.route.dstTopicId = 0;
						push(ADD_ROUTE);
					} else {
						// error: out of memory
						// todo
					}
				} else {
					// error: maximum number of routes reached
					// todo: inform user
				}
			}
			
			// exit
			if (entry("Exit"))
				pop();
		}
		break;
	case EDIT_ROUTE:
	case ADD_ROUTE:
		menu(delta, activated);
		{
			// get route
			Route &route = this->temp.route;
			RouteState &routeState = this->tempState.route;

			String srcTopic = route.getSrcTopic();
			String dstTopic = route.getDstTopic();
			if (entry(this->tempState.route.srcTopicId != 0 ? srcTopic : "Select Source Topic")) {
				enterTopicSelector(srcTopic, false, 0);
			}
			if (entry(this->tempState.route.dstTopicId != 0 ? dstTopic : "Select Destination Topic")) {
				enterTopicSelector(dstTopic, true, 1);
			}
			
			// save, delete, cancel
			if (routeState.srcTopicId != 0 && routeState.dstTopicId != 0) {
				if (entry("Save Route")) {
					int index = getThisIndex();
					
					// unsubscribe from old source topic
					if (index < this->routes.size()) {
						auto e = this->routes[index];
						TopicBuffer topic = e.flash->getSrcTopic();
						unsubscribeTopic(e.ram->srcTopicId, topic.status());
					}
					this->routes.write(index, &route, &routeState);
					pop();
				}
			}
			if (this->state == EDIT_ROUTE) {
				if (entry("Delete Route")) {
					int index = getThisIndex();

					// unsubscribe from new source topic
					TopicBuffer topic = route.getSrcTopic();
					unsubscribeTopic(routeState.srcTopicId, topic.status());

					// unsubscribe from old source topic
					auto e = this->routes[index];
					topic = e.flash->getSrcTopic();
					unsubscribeTopic(e.ram->srcTopicId, topic.status());
					
					this->routes.erase(index);
					pop();
				}
			}
			if (entry("Cancel")) {
				// unsubscribe from new source topic
				TopicBuffer topic = route.getSrcTopic();
				unsubscribeTopic(routeState.srcTopicId, topic.status());
				pop();
			}
		}
		break;
	case TIMERS:
		menu(delta, activated);
		{
			// list timers
			for (auto e : this->timers) {
				Timer const &timer = *e.flash;
				
				int minutes = timer.time.getMinutes();
				int hours = timer.time.getHours();
				StringBuffer<16> b = dec(hours) + ':' + dec(minutes, 2) + "    ";
				if (entryWeekdays(b, timer.time.getWeekdays())) {
					// edit timer
					clone(this->temp.timer, this->tempState.timer, timer, *e.ram);
					push(EDIT_TIMER);
				}
			}
			
			// add timer
			if (entry("Add Timer")) {
				// check if list of timers is full
				if (this->timers.size() < MAX_TIMER_COUNT) {
					// check if memory for a new timer is available
					Timer &timer = this->temp.timer;
					timer.time = {};
					timer.commandCount = 1;
					timer.u.commands[0].topicLength = TopicBuffer::MAX_TOPIC_LENGTH * 3;
					if (this->timers.hasSpace(&timer)) {
						// clear
						timer.commandCount = 0;
						push(ADD_TIMER);
					} else {
						// error: out of memory
						// todo
					}
				} else {
					// error: maximum number of timers reached
					// todo: inform user
				}
			}

			// exit
			if (entry("Exit"))
				pop();
		}
		break;
	case EDIT_TIMER:
	case ADD_TIMER:
		menu(delta, activated);
		{
			int edit;
			uint8_t editBegin[6];
			uint8_t editEnd[6];
			
			// get timer
			Timer &timer = this->temp.timer;
			TimerState &timerState = this->tempState.timer;
			
			// get time
			int hours = timer.time.getHours();
			int minutes = timer.time.getMinutes();
			int weekdays = timer.time.getWeekdays();

			// edit time
			edit = getEdit(2);
			if (edit > 0) {
				if (edit == 1)
					hours = (hours + delta + 24) % 24;
				else
					minutes = (minutes + delta + 60) % 60;
			}

			// time menu entry
			StringBuffer<16> b = "Time: ";
			editBegin[1] = this->buffer.length();
			b += dec(hours);
			editEnd[1] = this->buffer.length();
			b += ':';
			editBegin[2] = this->buffer.length();
			b += dec(minutes, 2);
			editEnd[2] = this->buffer.length();
			entry(b, edit > 0, editBegin[edit], editEnd[edit]);

			// edit weekdays
			edit = getEdit(7);
			if (edit > 0 && delta != 0)
				weekdays ^= 1 << (edit - 1);

			// weekdays menu entry
			b = "Days: ";
			entryWeekdays(b, weekdays, edit > 0, edit - 1);

			// write back
			timer.time = makeClockTime(weekdays, hours, minutes);

			// commands
			line();
			{
				//uint8_t *buffer = timer.u.buffer;
				uint8_t *data = timer.begin();
				uint8_t *end = timer.end();
				int commandIndex = 0;
				while (commandIndex < timer.commandCount) {
					// get command
					Command &command = timer.u.commands[commandIndex];
															
					// edit value type
					uint8_t *value = data;
					edit = getEdit(1 + command.getValueCount());
					if (edit == 1 && delta != 0) {
						// edit value type
						command.changeType(delta, value, end);
					}
					int valueSize = command.getValueSize();

					// get topic
					String topic(String(data + valueSize, command.topicLength));
					
					// menu entry for type
					editBegin[1] = 0;
					switch (command.type) {
					case Command::BUTTON:
						// button state is release or press, one data byte
						b = "Button: ";
						editEnd[1] = b.length() - 1;

						if (edit == 2 && delta != 0)
							value[1] ^= 1;
						
						editBegin[2] = b.length();
						b += buttonStatesLong[data[0] & 1];
						editEnd[2] = b.length();
						break;
					case Command::SWITCH:
						// binary state is off or on, one data byte
						b = "Switch: ";
						editEnd[1] = b.length() - 1;

						if (edit == 2 && delta != 0)
							value[0] ^= 1;

						editBegin[2] = b.length();
						b += switchStatesLong[data[0] & 1];
						editEnd[2] = b.length();
						break;
					case Command::ROCKER:
						// rocker state is release, up or down, one data byte
						b = "Rocker: ";
						editEnd[1] = b.length() - 1;

						if (edit == 2)
							value[0] = (value[0] + delta + 30) % 3;
						
						editBegin[2] = b.length();
						b += rockerStatesLong[data[0] & 3];
						editEnd[2] = b.length();
						break;
					case Command::VALUE8:
						// value 0-255, one data byte
						b = "Value 0-255: ";
						editEnd[1] = b.length() - 1;

						if (edit == 2)
							value[0] += delta;
						
						editBegin[2] = b.length();
						b += dec(data[0]);
						editEnd[2] = b.length();
						break;
					case Command::PERCENTAGE:
						// percentage 0-100, one data byte
						b = "Percentage: ";
						editEnd[1] = b.length() - 1;

						if (edit == 2)
							value[0] = (value[0] + delta + 100) % 100;
						
						editBegin[2] = b.length();
						b += dec(data[0]) + '%';
						editEnd[2] = b.length();
						break;
					case Command::TEMPERATURE:
						// room temperature 8° - 33,5°
						b = "Temperature: ";
						editEnd[1] = b.length() - 1;

						if (edit == 2)
							value[0] += delta;
						
						editBegin[2] = b.length();
						b += dec(8 + data[0] / 10) + '.' + dec(data[0] % 10);
						editEnd[2] = b.length();
						b += "oC";
						break;
					case Command::COLOR_RGB:
						b = "RGB Color: ";
						editEnd[1] = b.length() - 1;

						if (edit == 2)
							value[0] += delta;
						if (edit == 3)
							value[1] += delta;
						if (edit == 4)
							value[2] += delta;

						editBegin[2] = b.length();
						b += dec(data[0]);
						editEnd[2] = b.length();
						b += ' ';
						
						editBegin[3] = b.length();
						b += dec(data[1]);
						editEnd[3] = b.length();
						b += ' ';

						editBegin[4] = b.length();
						b += dec(data[2]);
						editEnd[4] = b.length();
						
						break;
					case Command::TYPE_COUNT:
						;
					}
					entry(b, edit > 0, editBegin[edit], editEnd[edit]);
					
					// menu entry for topic
					if (entry(!topic.empty() ? topic : "Select Topic")) {
						enterTopicSelector(topic, true, commandIndex);
					}
					
					// menu entry for testing the command
					if (entry("Test")) {
						publishValue(timerState.topicIds[commandIndex], command.type, value);
					}
					
					// menu entry for deleting the command
					int size = command.topicLength + valueSize;
					if (entry("Delete Command")) {
						// erase command
						array::erase(reinterpret_cast<uint8_t *>(&command), data, sizeof(Command));
						--timer.commandCount;
						data -= sizeof(Command);

						// erase data
						array::erase(data, end, sizeof(Command) + size);
					
						// erase topic id
						array::erase(timerState.topicIds + commandIndex, array::end(timerState.topicIds), 1);
					
						// select next menu entry (-4 + 1)
						this->selected -= 3;
					} else {
						++commandIndex;
						data += size;
					}
					
					line();
				}
				
				// add command
				if (timer.commandCount < Timer::MAX_COMMAND_COUNT) {
					if (entry("Add Command")) {
						// insert data
						array::insert(data, end, sizeof(Command));
						data += sizeof(Command);

						// initialize new command
						Command &command = timer.u.commands[commandIndex];
						command.type = Command::SWITCH;
						command.topicLength = 0;
						
						// initialize binary value
						*data = 0;
						
						++timer.commandCount;
					}
					line();
				}
			}
			
			// save, delete, cancel
			if (entry("Save Timer")) {
				int index = getThisIndex();
				this->timers.write(index, &this->temp.timer);
				pop();
			}
			if (this->state == EDIT_TIMER) {
				if (entry("Delete Timer")) {
					int index = getThisIndex();
					this->timers.erase(index);
					pop();
				}
			}
			if (entry("Cancel")) {
				pop();
			}
		}
		break;
	case SELECT_TOPIC:
		menu(delta, activated);
		
		// menu entry for "parent directory" (select device or room)
		if (this->topicDepth > 0) {
			constexpr String components[] = {"Room", "Device"};
			StringBuffer<16> b = "Select " + components[this->topicDepth - 1];
			if (entry(b)) {
				// unsubscribe from current topic
				unsubscribeTopic(this->selectedTopicId, this->selectedTopic.enumeration());

				// go to "parent directory"
				this->selectedTopic.removeLast();
				--this->topicDepth;
				
				// subscribe to new topic
				subscribeTopic(this->selectedTopicId, this->selectedTopic.enumeration(), QOS);

				// request the new topic list, gets filled in onPublished
				this->topicSet.clear();
				publish(this->selectedTopicId, "?", 1);
			}
		}

		// menu entry for each room/device/attribute
		String selected;
		for (String topic : this->topicSet) {
			if (entry(topic))
				selected = topic;
		}
		
		// cancel menu entry
		if (entry("Cancel")) {
			// unsubscribe from selected topic if it is not one of the own global topics
			RoomState *roomState = this->room[0].ram;
			unsubscribeTopic(this->selectedTopicId, this->selectedTopic.enumeration());
			
			// clear
			this->selectedTopic.clear();
			this->selectedTopicId = 0;
			this->topicSet.clear();
			
			// exit menu
			pop();
		}
		
		if (!selected.empty()) {
			// unsubscribe from selected topic if it is not one of the own global topics
			RoomState *roomState = this->room[0].ram;
			unsubscribeTopic(this->selectedTopicId, this->selectedTopic.enumeration());
			
			// append new element (room, device or attribute) to selected topic
			this->selectedTopic /= selected;// << '/' << selected;
			++this->topicDepth;

			// clear list of elements
			this->topicSet.clear();
			
			if (this->topicDepth < 3) {
				// enter next level
				subscribeTopic(this->selectedTopicId, this->selectedTopic.enumeration(), QOS);
				publish(this->selectedTopicId, "?", QOS);
				
				// select first entry in menu
				this->selected = 1;
			} else {
				// exit menu and write topic back to command
				pop();
				
				String topic = this->selectedTopic.string();
				switch (this->stack[this->stackIndex].state) {
				case EDIT_ROUTE:
				case ADD_ROUTE:
					{
						Route &route = this->temp.route;
						RouteState &state = this->tempState.route;
						
						if (this->tempIndex == 0) {
							// unsubscribe from old source topic
							TopicBuffer oldTopic = route.getSrcTopic();
							unsubscribeTopic(state.srcTopicId, oldTopic.status());

							// set new source topic
							route.setSrcTopic(topic);
							
							// subscribe to new source topic
							subscribeTopic(state.srcTopicId, this->selectedTopic.status(), QOS);
						} else {
							// unregsiter old destination topic
							unregisterTopic(state.dstTopicId);
							
							// set new destination topic
							route.setDstTopic(topic);
							
							// register new destination topic
							registerTopic(state.dstTopicId, this->selectedTopic.command());
						}
					}
					break;
				case EDIT_TIMER:
				case ADD_TIMER:
					{
						Timer &timer = this->temp.timer;
						TimerState &timerState = this->tempState.timer;
						
						// set topic to command
						int commandIndex = this->tempIndex;
						Command &command = timer.u.commands[commandIndex];
						uint8_t *data = timer.begin();
						uint8_t *end = timer.end();
						for (int i = 0; i < commandIndex; ++i)
							data += timer.u.commands[i].getFlashSize();
						data += command.getValueSize();
						command.setTopic(topic, data, end);
						
						// unregister topic for command
						unregisterTopic(timerState.topicIds[commandIndex]);
						
						// register topic for command
						registerTopic(timerState.topicIds[commandIndex], this->selectedTopic.command());
					}
					break;
				default:
					break;
				}

				// clear
				this->selectedTopic.clear();
				this->selectedTopicId = 0;
			}
		}
		break;
	}
	this->buffer.clear();
}



// Menu System
// -----------

void RoomControl::menu(int delta, bool activated) {
	// update selected according to delta motion of poti when not in edit mode
	if (this->edit == 0) {
		this->selected += delta;
		if (this->selected < 0) {
			this->selected = 0;

			// also clear yOffset in case the menu has a non-selectable header
			this->yOffset = 0;
		} else if (this->selected >= this->entryIndex) {
			this->selected = this->entryIndex - 1;
		}
	}
	
	// set activated state for selecting the current menu entry
	this->activated = activated;


	const int lineHeight = tahoma_8pt.height + 4;

	// adjust yOffset so that selected entry is visible
	int upper = this->selectedY;
	int lower = upper + lineHeight;
	if (upper < this->yOffset)
		this->yOffset = upper;
	if (lower > this->yOffset + bitmap.HEIGHT)
		this->yOffset = lower - bitmap.HEIGHT;

	this->entryIndex = 0;
	this->entryY = 0;
}
	
void RoomControl::label(String s) {
	int x = 10;
	int y = this->entryY + 2 - this->yOffset;
	this->bitmap.drawText(x, y, tahoma_8pt, s, 1);
	this->entryY += tahoma_8pt.height + 4;
}

void RoomControl::line() {
	int x = 10;
	int y = this->entryY + 2 - this->yOffset;
	this->bitmap.fillRectangle(x, y, 108, 1);
	this->entryY += 1 + 4;
}

bool RoomControl::entry(String s, bool underline, int begin, int end) {
	int x = 10;
	int y = this->entryY + 2 - this->yOffset;//this->entryIndex * lineHeight + 2 - this->yOffset;
	this->bitmap.drawText(x, y, tahoma_8pt, s);
	
	bool selected = this->entryIndex == this->selected;
	if (selected) {
		this->bitmap.drawText(0, y, tahoma_8pt, ">", 0);
		this->selectedY = this->entryY;
	}
	
	if (underline) {
		int start = tahoma_8pt.calcWidth(s.substring(0, begin));
		int width = tahoma_8pt.calcWidth(s.substring(begin, end)) - 1;
		this->bitmap.hLine(x + start, y + tahoma_8pt.height, width);
	}

	++this->entryIndex;
	this->entryY += tahoma_8pt.height + 4;

	return selected && this->activated;
}

bool RoomControl::entryWeekdays(String s, int weekdays, bool underline, int index) {
	int x = 10;
	int y = this->entryY + 2 - this->yOffset;

	// text (e.g. time)
	int x2 = this->bitmap.drawText(x, y, tahoma_8pt, s, 1) + 1;
	int start = x;
	int width = x2 - x;

	// week days
	for (int i = 0; i < 7; ++i) {
		int x3 = this->bitmap.drawText(x2 + 1, y, tahoma_8pt, weekdaysShort[i], 1);
		if (weekdays & 1)
			this->bitmap.fillRectangle(x2, y, x3 - x2, tahoma_8pt.height - 1, Mode::FLIP);
		if (i == index) {
			start = x2;
			width = x3 - x2;
		}
		x2 = x3 + 4;
		weekdays >>= 1;
	}

	bool selected = this->entryIndex == this->selected;
	if (selected) {
		this->bitmap.drawText(0, y, tahoma_8pt, ">", 1);
		this->selectedY = this->entryY;
	}
	
	if (underline) {
		this->bitmap.hLine(start, y + tahoma_8pt.height, width);
	}

	++this->entryIndex;
	this->entryY += tahoma_8pt.height + 4;

	return selected && this->activated;
}

int RoomControl::getEdit(int editCount) {
	if (this->selected == this->entryIndex) {
		// cycle edit mode if activated
		if (this->activated) {
			if (this->edit < editCount) {
				++this->edit;
			} else {
				this->edit = 0;
				//this->editFinished = true;
			}
		}
		return this->edit;
	}
	return 0;
}

void RoomControl::push(State state) {
	this->stack[this->stackIndex] = {this->state, this->selected, this->selectedY, this->yOffset};
	++this->stackIndex;
	assert(this->stackIndex < array::size(this->stack));
	this->stack[this->stackIndex] = {state, 0, 0, 0};
	this->stackHasChanged = true;
}

void RoomControl::pop() {
	--this->stackIndex;
	this->stackHasChanged = true;
}


// Room
// ----

int RoomControl::Room::getFlashSize() const {
	int length = array::size(this->name);
	for (int i = 0; i < array::size(this->name); ++i) {
		if (this->name[i] == 0) {
			length = i + 1;
			break;
		}
	}
	return offsetof(Room, name[length]);
}

int RoomControl::Room::getRamSize() const {
	return sizeof(RoomState);
}

// todo don't register all topics in one go, message buffer may overflow
void RoomControl::subscribeAll() {
	
	// register/subscribe room topics
	RoomElement e = this->room[0];
	Room const &room = *e.flash;
	RoomState &roomState = *e.ram;
	
	// house topic (room name is published when "?" is received on this topic)
	TopicBuffer topic;
	subscribeTopic(roomState.houseTopicId, topic.enumeration(), QOS);

	// toom topic (device names are published when "?" is received on this topic)
	topic /= room.name;
	subscribeTopic(roomState.roomTopicId, topic.enumeration(), QOS);


	// register/subscribe device topics
	for (int i = 0; i < this->devices.size(); ++i) {
		subscribeDevice(i);
	}

	// register routes
	for (int i = 0; i < this->routes.size(); ++i) {
		subscribeRoute(i);
	}

	// register timers
	for (int i = 0; i < this->timers.size(); ++i) {
		subscribeTimer(i);
	}
}


// Devices
// -------

Plus<String, Dec<uint8_t>> RoomControl::Device::Component::getName() const {
	//return componentInfos[this->type].shortName + dec(this->endpointIndex) + ('a' + this->componentIndex);
	uint8_t shortNameIndex = componentInfos[this->type].shortNameIndex;
	return componentShortNames[shortNameIndex] + dec(this->nameIndex);
}

void RoomControl::Device::Component::init(EndpointType endpointType/*, uint8_t endpointIndex, uint8_t componentIndex*/) {
	auto type = Device::Component::BUTTON;
	
	int i = array::size(componentInfos);
	while (i > 0 && !isCompatible(endpointType/*, componentIndex*/, type)) {
		--i;
		int t = type + 1;
		type = Device::Component::Type(t % array::size(componentInfos));
	}
	
	int newSize = componentInfos[type].flash.size;
	auto dst = reinterpret_cast<uint32_t *>(this);
	array::fill(dst, dst + newSize, 0);

	this->type = type;
	//this->endpointIndex = endpointIndex;
	//this->componentIndex = componentIndex;

}

void RoomControl::Device::Component::rotateType(EndpointType endpointType, int delta) {
	auto type = this->type;
	int oldSize = componentInfos[type].flash.size;
	
	int dir = delta >= 0 ? 1 : -1;
	int i = array::size(componentInfos);
	while (delta != 0) {
		delta -= dir;
		do {
			--i;
			int t = type + dir;
			type = Device::Component::Type((t + array::size(componentInfos)) % array::size(componentInfos));
		} while (i > 0 && !isCompatible(endpointType, /*this->componentIndex,*/ type));
	}
	this->type = type;
	int newSize = componentInfos[type].flash.size;
	
	if (newSize > oldSize) {
		auto dst = reinterpret_cast<uint32_t *>(this) + oldSize;
		array::fill(dst, dst + (newSize - oldSize), 0);
	}
}

bool RoomControl::Device::Component::checkClass(uint8_t classFlags) const {
	return (componentInfos[this->type].flash.classFlags & classFlags) == classFlags;
}

int RoomControl::Device::getFlashSize() const {
	// flash size of name and elements
	uint32_t const *flash = this->buffer;
	int size = 0;
	for (int i = 0; i < this->componentCount; ++i) {
		auto type = reinterpret_cast<Device::Component const *>(flash + size)->type;
		size += componentInfos[type].flash.size;
	}
	return getOffset(Device, buffer[size]);
}

int RoomControl::Device::getRamSize() const {
	// ram size of elements
	uint32_t const *flash = this->buffer;
	int size = 0;
	for (int i = 0; i < this->componentCount; ++i) {
		auto type = reinterpret_cast<Device::Component const *>(flash)->type;
		flash += componentInfos[type].flash.size;
		size += componentInfos[type].ram.size;
	}
	return getOffset(DeviceState, buffer[size]);
}

void RoomControl::Device::setName(String name) {
	assign(this->name, name);
/*
	uint8_t *dst = this->buffer;
	uint8_t *end = array::end(this->buffer);
	
	// reallocate name
	int oldSize = align4(this->nameLength);
	int newSize = align4(name.length);
	if (newSize > oldSize)
		array::insert(dst + oldSize, end, newSize - oldSize);
	else if (newSize < oldSize)
		array::erase(dst + newSize, end, oldSize - newSize);
	this->nameLength = name.length;
	
	// copy name
	array::copy(dst, dst + name.length, name.data);*/
}

uint8_t RoomControl::Device::getNameIndex(Component::Type type, int skipComponentIndex) const {
	// set of already used name indices
	uint32_t usedSet = 0;
	
	uint8_t shortNameIndex = componentInfos[type].shortNameIndex;
	
	uint32_t const *flash = this->buffer;
	for (int i = 0; i < this->componentCount; ++i) {
		auto &component = *reinterpret_cast<Component const *>(flash);
		auto &componentInfo = componentInfos[component.type];
		if (i != skipComponentIndex) {
			if (componentInfo.shortNameIndex == shortNameIndex)
				usedSet |= 1 << component.nameIndex;
		}
		flash += componentInfo.flash.size;
	}
	
	// find first unused name index
	for (int i = 0; i < 32; ++i) {
		if (((usedSet >> i) & 1) == 0)
			return i;
	}
	return 0;
}


bool RoomControl::DeviceState::Component::checkClass(Device::Component::Type type, uint8_t classFlags) const {
	return (componentInfos[type].ram.classFlags & classFlags) == classFlags;
}


bool RoomControl::ComponentIterator::next() {
	auto type = getComponent().type;
	uint8_t endpointIndex = getComponent().endpointIndex;
	++this->componentIndex;
	this->flash += componentInfos[type].flash.size;
	this->ram += componentInfos[type].ram.size;

	// return true if a group of components that are associated to the same endpoint ends
	return atEnd() || getComponent().endpointIndex != endpointIndex;
}

RoomControl::Device::Component &RoomControl::ComponentEditor::insert(Device::Component::Type type) {
	auto &componentInfo = componentInfos[type];
	
	// insert element
	array::insert(this->flash, array::end(this->device.buffer), componentInfo.flash.size);
	array::insert(this->ram, array::end(this->deviceState.buffer), componentInfo.ram.size);
	++this->device.componentCount;

	// clear ram
	array::fill(this->ram, this->ram + componentInfo.ram.size, 0);

	// set type
	auto &component = *reinterpret_cast<Device::Component *>(this->flash);
	component.type = type;
	return component;
}

void RoomControl::ComponentEditor::changeType(Device::Component::Type type) {
	auto oldType = getComponent().type;
	int oldFlashSize = componentInfos[oldType].flash.size;
	int oldRamSize = componentInfos[oldType].ram.size;

	int newFlashSize = componentInfos[type].flash.size;
	int newRamSize = componentInfos[type].ram.size;

	// reallocate flash
	if (newFlashSize > oldFlashSize)
		array::insert(this->flash + oldFlashSize, array::end(this->device.buffer), newFlashSize - oldFlashSize);
	else if (newFlashSize < oldFlashSize)
		array::erase(this->flash + newFlashSize, array::end(this->device.buffer), oldFlashSize - newFlashSize);
	
	// reallocate ram
	if (newRamSize > oldRamSize)
		array::insert(this->ram + oldRamSize, array::end(this->deviceState.buffer), newRamSize - oldRamSize);
	else if (newRamSize < oldRamSize)
		array::erase(this->ram + newRamSize, array::end(this->device.buffer), oldRamSize - newRamSize);

	// clear ram behind base class
	array::fill(this->ram + (sizeof(DeviceState::Component) + 3 / 4), this->ram + newRamSize, 0);

	getComponent().type = type;
}

void RoomControl::ComponentEditor::next() {
	auto type = getComponent().type;
	++this->componentIndex;
	this->flash += componentInfos[type].flash.size;
	this->ram += componentInfos[type].ram.size;
}


void RoomControl::subscribeDevice(int index) {
	auto e = this->devices[index];
	Device const &device = *e.flash;
	DeviceState &deviceState = *e.ram;

	// subscribe to device topic (enum/<room>/<device>)
	TopicBuffer topic = getRoomName() / device.getName();
	subscribeTopic(e.ram->deviceTopicId, topic.enumeration(), QOS);

	// iterate over device components
	ComponentIterator it(device, deviceState);
	while (!it.atEnd()) {
		auto &component = it.getComponent();
		auto &state = it.getState();
		
		// append component topic
		StringBuffer<8> b = component.getName();
		topic /= b;
		
		// register status topic (stat/<room>/<device>/<component>)
		registerTopic(state.statusTopicId, topic.status());

		// check if component has a command topic
		if (state.is<DeviceState::CommandComponent>(component)) {
			// subscribe to command topic (cmnd/<room>/<device>/<component>)
			auto &commandComponentState = state.cast<DeviceState::CommandComponent>();
			subscribeTopic(commandComponentState.commandTopicId, topic.command(), QOS);
		}
		
		topic.removeLast();
		it.next();
	}
}

SystemTime RoomControl::updateDevices(SystemTime time) {
	bool report = time >= this->nextReportTime;
	if (report) {
		// publish changing values in a regular interval
		this->nextReportTime = time + 1s;
		//std::cout << "report" << std::endl;
	}

	SystemDuration duration = time - this->lastUpdateTime;
	this->lastUpdateTime = time;

	SystemTime nextTimeout = this->nextReportTime;

	// iterate over bus devices
	for (auto e : this->devices) {
		Device const &device = *e.flash;
		DeviceState &deviceState = *e.ram;

		// group event to transfer up/down states to virtual button/rocker
		//bool groupEvent = false;
		//uint8_t groupState;
		uint8_t outEndpointId = 0;
		uint8_t outData = 0;

		// iterate over device elements
		ComponentIterator it(device, deviceState);
		while (!it.atEnd()) {
			auto &component = it.getComponent();
			auto &state = it.getState();
			
			switch (component.type) {
			case Device::Component::BUTTON:
				if (outEndpointId != 0) {
					// get button state
					uint8_t value = (outData >> component.elementIndex) & 1;
					if (value != state.flags) {
						state.flags = value;
						publish(state.statusTopicId, buttonStates[value], QOS);
					}
				}
				break;
				/*
			case Device::Component::BUTTON:
				if (groupEvent) {
					// get button state
					uint8_t value = (groupState >> component.elementIndex) & 1;
					if (value != state.flags) {
						state.flags = value;
						publish(state.statusTopicId, buttonStates[value], QOS);
					}
				}
				break;*/
			case Device::Component::HOLD_BUTTON:
				break;
			case Device::Component::DELAY_BUTTON:
				{
					auto &delayButtonState = state.cast<DeviceState::DelayButton>();

					if (state.flags == 1) {
						// currently pressed, but timeout has not elapsed yet
						if (time < delayButtonState.timeout) {
							// calc next timeout
							nextTimeout = min(nextTimeout, delayButtonState.timeout);
						} else {
							// timeout elapsed: publish press
							state.flags = 3;
							publish(delayButtonState.statusTopicId, switchStates[1], QOS);
						}
					}
				}
				break;
			case Device::Component::SWITCH:
				if (outEndpointId != 0) {
					// get switch state
					uint8_t value = (outData >> component.elementIndex) & 1;
					if (value != state.flags) {
						state.flags = value;
						publish(state.statusTopicId, switchStates[value], QOS);
					}
				}
				break;
			case Device::Component::ROCKER:
				if (outEndpointId != 0) {
					// get rocker state
					uint8_t value = (outData >> component.elementIndex) & 3;
					if (value != state.flags) {
						state.flags = value;
						publish(state.statusTopicId, rockerStates[value], QOS);
					}
				}
				break;
/*
			case Device::Component::SWITCH:
				if (groupEvent) {
					// get switch state
					uint8_t value = (groupState >> component.elementIndex) & 1;
					if (value != state.flags) {
						state.flags = value;
						publish(state.statusTopicId, switchStates[value], QOS);
					}
				}
				break;
			case Device::Component::ROCKER:
				if (groupEvent) {
					// get rocker state
					uint8_t value = (groupState >> component.elementIndex) & 3;
					if (value != state.flags) {
						state.flags = value;
						publish(state.statusTopicId, rockerStates[value], QOS);
					}
				}
				break;*/
			case Device::Component::HOLD_ROCKER:
			case Device::Component::RELAY:
				break;
			case Device::Component::TIME_RELAY:
				{
					uint8_t relay = state.flags & DeviceState::Blind::RELAY;
					if (relay != 0 && component.type == Device::Component::TIME_RELAY) {
						// currently on
						auto &timeRelayState = state.cast<DeviceState::TimeRelay>();
						if (time < timeRelayState.timeout) {
							// calc next timeout
							nextTimeout = min(nextTimeout, timeRelayState.timeout);
						} else {
							// switch off
							timeRelayState.flags &= ~DeviceState::Blind::RELAY;
							relay = 0;
							outEndpointId = state.endpointId;
							//busSend(timeRelayState.endpointId, &relay, 1);

							// report state change
							publish(state.statusTopicId, switchStates[0], QOS);
						}
					}
					outData |= relay << component.elementIndex;
				}
				break;
			case Device::Component::BLIND:
				{
					uint8_t relays = state.flags & DeviceState::Blind::RELAYS;
					if (relays != 0) {
						// currently moving
						auto &blind = component.cast<Device::Blind>();
						auto &blindState = state.cast<DeviceState::Blind>();
						if (relays & DeviceState::Blind::RELAY_UP) {
							// currently moving up
							blindState.duration += duration;
							if (blindState.duration > blind.duration)
								blindState.duration = blind.duration;
						} else {
							// currently moving down
							blindState.duration -= duration;
							if (blindState.duration < 0s)
								blindState.duration = 0s;
						}
					
						// check if run time has elapsed
						int decimalCount;
						if (time < blindState.timeout) {
							// no: calc next timeout
							nextTimeout = min(nextTimeout, blindState.timeout);
							decimalCount = 2;
						} else {
							// yes: stop
							blindState.flags &= ~DeviceState::Blind::RELAYS;
							relays = 0;
							outEndpointId = state.endpointId;
							//busSend(blindState.endpointId, &relays, 1);
							decimalCount = 3;
						
							//groupEvent = true;
							//groupState = 0;
						}
						
						if (report || relays == 0) {
							// publish current position after interval or when stopped
							float pos = blindState.duration / blind.duration;
							StringBuffer<8> b = flt(pos, 0, decimalCount);
							publish(blindState.statusTopicId, b, QOS);
						}
					}
					outData |= relays << component.elementIndex;
				}
				break;
			case Device::Component::CELSIUS:
			case Device::Component::FAHRENHEIT:
				break;
			}
			if (it.next()) {
				if (outEndpointId != 0) {
					busSend(outEndpointId, &outData, 1);
					outEndpointId = 0;
				}
			}
		}
	}
	
	return nextTimeout;
}

bool RoomControl::isCompatible(EndpointType endpointType/*, int componentIndex*/, Device::Component::Type type) {
	bool binary = type == Device::Component::BUTTON || type == Device::Component::SWITCH;
	bool timeBinary = type == Device::Component::HOLD_BUTTON || Device::Component::DELAY_BUTTON;
	bool ternary = type == Device::Component::ROCKER;
	bool timeTernary = type == Device::Component::HOLD_ROCKER;
	bool relay = type == Device::Component::RELAY || type == Device::Component::TIME_RELAY;
	
	switch (endpointType) {
	case EndpointType::BINARY_SENSOR:
	case EndpointType::BUTTON:
	case EndpointType::SWITCH:
		// buttons and switchs
		return binary || timeBinary;
//	case EndpointType::BINARY_SENSOR_2:
//	case EndpointType::BUTTON_2:
//	case EndpointType::SWITCH_2:
	case EndpointType::ROCKER:
//	case EndpointType::BINARY_SENSOR_4:
//	case EndpointType::BUTTON_4:
//	case EndpointType::SWITCH_4:
//	case EndpointType::ROCKER_2:
		// buttons, switches and rockers
		return binary || timeBinary || ternary || timeTernary;
	case EndpointType::RELAY:
	case EndpointType::LIGHT:
		// one relay with binary sensors for the relay state
		return relay || binary;
//	case EndpointType::RELAY_2:
//	case EndpointType::LIGHT_2:
	case EndpointType::BLIND:
//	case EndpointType::RELAY_4:
//	case EndpointType::LIGHT_4:
//	case EndpointType::BLIND_2:
		// two relays with binary and ternaty sensors for the relay states
		return type == relay || Device::Component::BLIND || binary || ternary;
	case EndpointType::TEMPERATURE_SENSOR:
		// temperature sensor
		return type == Device::Component::CELSIUS || type == Device::Component::FAHRENHEIT;
	}
	return false;
}

void RoomControl::clone(Device &dstDevice, DeviceState &dstDeviceState,
	Device const &srcDevice, DeviceState const &srcDeviceState)
{
	// copy device
	{
		auto begin = reinterpret_cast<uint32_t *>(&dstDevice);
		auto end = begin + (srcDevice.getFlashSize() >> 2);
		array::copy(begin, end, reinterpret_cast<uint32_t const *>(&srcDevice));
	}
	
	// copy state
	{
		auto begin = reinterpret_cast<uint32_t *>(&dstDeviceState);
		auto end = begin + (srcDevice.getRamSize() >> 2);
		array::copy(begin, end, reinterpret_cast<uint32_t const *>(&srcDeviceState));
	}
	
	// dublicate subscriptions of command topics
	for (ComponentIterator it(dstDevice, dstDeviceState); !it.atEnd(); it.next()) {
		auto &component = it.getComponent();
		auto &state = it.getState();

		if (state.is<DeviceState::CommandComponent>(component)) {
			addSubscriptionReference(state.cast<DeviceState::CommandComponent>().commandTopicId);
		}
	}
}

void RoomControl::destroy(Device const &device, DeviceState &deviceState) {
	TopicBuffer topic = getRoomName() / device.getName();

	// destroy components
	for (ComponentIterator it(device, deviceState); !it.atEnd(); it.next()) {
		auto &component = it.getComponent();
		auto &state = it.getState();
		
		if (state.is<DeviceState::CommandComponent>(component)) {
			StringBuffer<8> b = component.getName();
			topic /= b;
			unsubscribeTopic(state.cast<DeviceState::CommandComponent>().commandTopicId, topic.command());
			topic.removeLast();
		}
	}
}


// Routes
// ------

int RoomControl::Route::getFlashSize() const {
	return getOffset(Route, buffer[this->srcTopicLength + this->dstTopicLength]);
}

int RoomControl::Route::getRamSize() const {
	return sizeof(RouteState);
}

void RoomControl::Route::setSrcTopic(String topic) {
	// move data of destination topic
	int oldSize = this->srcTopicLength;
	int newSize = topic.length;
	if (newSize > oldSize)
		array::insert(this->buffer, this->buffer + newSize + this->dstTopicLength, newSize - oldSize);
	else if (newSize < oldSize)
		array::erase(this->buffer, this->buffer + oldSize + this->dstTopicLength, oldSize - newSize);

	uint8_t *dst = this->buffer;
	array::copy(dst, dst + topic.length, topic.begin());
	this->srcTopicLength = newSize;
}

void RoomControl::Route::setDstTopic(String topic) {
	uint8_t *dst = this->buffer + this->srcTopicLength;
	array::copy(dst, dst + topic.length, topic.begin());
	this->dstTopicLength = topic.length;
}

void RoomControl::subscribeRoute(int index) {
	auto e = this->routes[index];
	Route const &route = *e.flash;
	RouteState &routeState = *e.ram;
	
	TopicBuffer topic = route.getSrcTopic();
	subscribeTopic(routeState.srcTopicId, topic.status(), QOS);
	
	topic = route.getDstTopic();
	registerTopic(routeState.dstTopicId, topic.command());
}

void RoomControl::clone(Route &dstRoute, RouteState &dstRouteState,
	Route const &srcRoute, RouteState const &srcRouteState)
{
	// copy route
	uint8_t *dst = reinterpret_cast<uint8_t *>(&dstRoute);
	uint8_t const *src = reinterpret_cast<uint8_t const *>(&srcRoute);
	array::copy(dst, dst + srcRoute.getFlashSize(), src);

	// clone state
	dstRouteState.srcTopicId = srcRouteState.srcTopicId;
	addSubscriptionReference(dstRouteState.dstTopicId = srcRouteState.dstTopicId);
}


// Command
// -------

int RoomControl::Command::getValueSize() const {
	switch (this->type) {
	case COLOR_RGB:
		return 3;
	default:
		return 1;
	}
}

int RoomControl::Command::getValueCount() const {
	switch (this->type) {
	case COLOR_RGB:
		return 3;
	default:
		return 1;
	}
}

void RoomControl::Command::changeType(int delta, uint8_t *data, uint8_t *end) {
	// change value type
	int oldSize = getValueSize();
	this->type = Type((uint32_t(this->type) + delta + TYPE_COUNT) % uint32_t(TYPE_COUNT));
	int newSize = getValueSize();
	
	// move data of topic and following commands
	if (newSize > oldSize)
		array::insert(data, end, newSize - oldSize);
	else if (newSize < oldSize)
		array::erase(data, end, oldSize - newSize);
	
	// clear value
	array::fill(data, data + newSize, 0);
}

void RoomControl::Command::setTopic(String topic, uint8_t *data, uint8_t *end) {
	int oldSize = this->topicLength;
	int newSize = topic.length;

	// move data of following commands
	if (newSize > oldSize)
		array::insert(data, end, newSize - oldSize);
	else if (newSize < oldSize)
		array::erase(data, end, oldSize - newSize);

	array::copy(data, data + topic.length, topic.begin());
	this->topicLength = newSize;
}

void RoomControl::publishValue(uint16_t topicId, Command::Type type, uint8_t const *value) {
	StringBuffer<16> b;
	switch (type) {
	case Command::BUTTON:
		b += buttonStates[*value & 1];
		break;
	case Command::SWITCH:
		b += switchStates[*value & 1];
		break;
	case Command::ROCKER:
		b += rockerStates[*value & 3];
		break;
	case Command::VALUE8:
		b += dec(*value);
		break;
	case Command::PERCENTAGE:
		b += flt(*value * 0.01f, 0, 2);
		break;
	case Command::TEMPERATURE:
		b += flt(8.0f + *value * 0.1f, 0, 1);
		break;
	case Command::COLOR_RGB:
		b += "rgb(" + dec(value[0]) + ',' + dec(value[1]) + ',' + dec(value[2]) + ')';
		break;
	case Command::TYPE_COUNT:
		;
	}
	
	publish(topicId, b, 1);
}


// Timers
// ------

int RoomControl::Timer::getFlashSize() const {
	int size = sizeof(Command) * this->commandCount;
	for (int i = 0; i < this->commandCount; ++i) {
		Command const &command = this->u.commands[i];
		size += command.getFlashSize();
	}
	return getOffset(Timer, u.buffer[size]);
}

int RoomControl::Timer::getRamSize() const {
	return getOffset(TimerState, topicIds[this->commandCount]);
}

void RoomControl::subscribeTimer(int index) {
	auto e = this->timers[index];
	
	Timer const *timer = e.flash;
	TimerState *timerState = e.ram;
	
	uint8_t const *data = timer->u.buffer + sizeof(Command) * timer->commandCount;
	for (int commandIndex = 0; commandIndex < timer->commandCount; ++commandIndex) {
		Command const &command = timer->u.commands[commandIndex];
		
		// get value
		uint8_t const *value = data;
		int valueSize = command.getValueSize();

		// get topic
		TopicBuffer topic = String(data + valueSize, command.topicLength);

		// register topic (so that we can publish on timer event)
		registerTopic(timerState->topicIds[commandIndex], topic.command());
		
		data += valueSize + command.topicLength;
	}
}

void RoomControl::clone(Timer &dstTimer, TimerState &dstTimerState,
	Timer const &srcTimer, TimerState const &srcTimerState)
{
	// copy timer
	uint8_t *dst = reinterpret_cast<uint8_t *>(&dstTimer);
	uint8_t const *src = reinterpret_cast<uint8_t const *>(&srcTimer);
	array::copy(dst, dst + srcTimer.getFlashSize(), src);

	// clone state
	for (int i = 0; i < srcTimer.commandCount; ++i) {
		addSubscriptionReference(dstTimerState.topicIds[i] = srcTimerState.topicIds[i]);
	}
}


// Clock
// -----

void RoomControl::onSecondElapsed() {
	updateMenu(0, false);
	setDisplay(this->bitmap);

	// check for timer event
	ClockTime now = getClockTime();
	if (now.getSeconds() == 0) {
		int minutes = now.getMinutes();
		int hours = now.getHours();
		int weekday = now.getWeekday();
		for (auto e : this->timers) {
			const Timer *timer = e.flash;
			const TimerState *timerState = e.ram;
			
			ClockTime t = timer->time;
			if (t.getMinutes() == minutes && t.getHours() == hours && (t.getWeekdays() & (1 << weekday)) != 0) {
				// timer event
				uint8_t const *data = timer->u.buffer + sizeof(Command) * timer->commandCount;
				for (int commandIndex = 0; commandIndex < timer->commandCount; ++commandIndex) {
					Command const &command = timer->u.commands[commandIndex];
					
					// get value
					uint8_t const *value = data;
					int valueSize = command.getValueSize();

					// publish value
					publishValue(timerState->topicIds[commandIndex], command.type, value);

					data += command.topicLength + valueSize;
				}
			}
		}
	}
}


// Topic Selector
// --------------

void RoomControl::enterTopicSelector(String topic, bool onlyCommands, int index) {
	this->selectedTopic = topic;

	// check if current topic has the format <room>/<device>/<attribute>
	if (this->selectedTopic.getElementCount() == 3) {
		// remove attibute from topic
		this->selectedTopic.removeLast();
		this->topicDepth = 2;
	} else {
		// start at this room
		this->selectedTopic = getRoomName();
		this->topicDepth = 1;
	}

	// only list attributes that have a command topic
	this->onlyCommands = onlyCommands;

	// subscribe to selected topic
	subscribeTopic(this->selectedTopicId, this->selectedTopic.enumeration(), QOS);

	// request enumeration of rooms, devices or attributes
	publish(this->selectedTopicId, "?", QOS);

	// enter topic selector menu
	push(SELECT_TOPIC);
	this->tempIndex = index;
}
