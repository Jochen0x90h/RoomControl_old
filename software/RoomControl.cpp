//#include <iostream>
#include "RoomControl.hpp"

// font
#include "tahoma_8pt.hpp"

static char const *prefix = "rc";
static char const weekdays[7][4] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
static char const weekdaysShort[7][2] = {"M", "T", "W", "T", "F", "S", "S"};
static char const *buttonStates[] = {"release", "press"};
static char const *rockerStates[] = {"release", "up", "down", ""};
static char const *binaryStates[] = {"off", "on"};

static uint8_t const offOn[2] = {'0', '1'};


RoomControl::RoomControl()
	: storage(0, FLASH_PAGE_COUNT, room, localDevices, timers)
{
	if (this->room.size() == 0) {
		assign(this->temp.room.name, "room");
		this->room.write(0, &this->temp.room);
	}
	
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
	subscribeDevices();
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

	// check for house command
	if (topicId == roomState.houseTopicId && message == "?") {
		// publish room name
		publish(roomState.houseTopicId, this->room[0].flash->name, 1);
		return;
	}

	// check for room command
	if (topicId == roomState.roomTopicId && message == "?") {
		// publish device names
		for (int i = 0; i < this->localDevices.size(); ++i) {
			LocalDeviceElement e = this->localDevices[i];
			publish(roomState.roomTopicId, e.flash->getName(), 1);
		}
		return;
	}

	//if (retain) {
		for (Forward forward : forwards) {
			if (topicId == forward.srcId) {
				std::cout << "forward " << topicId << " -> " << forward.dstId << std::endl;
				publish(forward.dstId, data, length, qos);
			}
		}
	//}


	SystemTime time = getSystemTime();

	// update time dependent state of devices (e.g. blind position)
	SystemTime nextTimeout = updateDevices(time);

	// iterate over local devices
	for (LocalDeviceElement e : this->localDevices) {
		switch (e.flash->type) {
		case Lin::Device::SWITCH2:
			{
				Switch2Device const &device = *reinterpret_cast<Switch2Device const *>(e.flash);
				Switch2State &state = *reinterpret_cast<Switch2State *>(e.ram);

				// check if the attribute list of the device is requested
				if (topicId == state.deviceTopicId && message == "?") {
					uint8_t flags = device.flags;
					for (int i = 0; i < 2; ++i, flags >>= 2) {
						// switches
						Switch2State::Switches &ss = state.s[i];
						uint8_t s = flags & 3;
						if (s != 0) {
							this->buffer << "ab"[i];
							if (s == 2)
								this->buffer << '0';
							this->buffer << " s"; // only state
							publish(state.deviceTopicId, this->buffer, 1);
							if (s == 2) {
								this->buffer[1] = '1';
								publish(state.deviceTopicId, this->buffer, 1);
							}
							this->buffer.clear();
						}

						// relays
						Switch2State::Relays &sr = state.r[i];
						uint8_t r = (flags >> 4) & 3;
						if (r != 0) {
							this->buffer << "xy"[i];
							if (r == 2)
								this->buffer << '0';
							this->buffer << " c"; // command and state
							publish(state.deviceTopicId, this->buffer, 1);
							if (r == 2) {
								this->buffer[1] = '1';
								publish(state.deviceTopicId, this->buffer, 1);
							}
							this->buffer.clear();
						}
					}
					return;
				}

				// try to parese float value
				optional<float> value = parseFloat(message);

				uint8_t flags = device.flags >> 4;
				uint8_t const relays = state.relays;
				uint8_t bit1 = 0x01;
				for (int i = 0; i < 2; ++i, flags >>= 2, bit1 <<= 2) {
					Switch2Device::Relays const &dr = device.r[i];
					Switch2State::Relays &sr = state.r[i];

					if (topicId == sr.commandId1 || topicId == sr.commandId2) {
						uint8_t bit2 = bit1 << 1;
						uint8_t s = flags & 3;
						if (s == 3) {
							// blind (two interlocked relays)
							if (value != null) {
								// move to position in the range [0, 1]
								state.relays &= ~(bit1 | bit2);
								if (*value >= 1.0f) {
									// up with extra time
									sr.timeout1 = time + dr.duration1 - sr.duration + 500ms;
									state.relays |= bit1;
								} else if (*value <= 0.0f) {
									// down with extra time
									sr.timeout1 = time + sr.duration + 500ms;
									state.relays |= bit2;
								} else {
									// move to target value
									SystemDuration targetDuration = dr.duration1 * *value;
									if (targetDuration > sr.duration) {
										// up
										sr.timeout1 = time + targetDuration - sr.duration;
										state.relays |= bit1;
									} else {
										// down
										sr.timeout1 = time + sr.duration - targetDuration;
										state.relays |= bit2;
									}
								}
								nextTimeout = min(nextTimeout, sr.timeout1);
							
							} else if (length == 1) {
								bool up = data[0] == '+';
								bool toggle = data[0] == '!';
								bool press = up || toggle || data[0] == '-';
								bool release = data[0] == '#';
								bool p = (state.pressed & bit1) != 0;
								if ((!p && press) || (p && release)) {
									state.pressed ^= bit1;
								
									if (relays & (bit1 | bit2)) {
										// currently moving
										bool stop = press || (release && time >= sr.timeout2) ;
										if (stop) {
											// stop
											state.relays &= ~(bit1 | bit2);
										
											// publish current blind position
											int pos = (sr.duration * 100 + dr.duration1 / 2) / dr.duration1;
											//std::cout << "pos " << pos << std::endl;
											this->buffer << pos;
											publish(sr.stateId1, this->buffer, 1, true);
											this->buffer.clear();
										}
									} else {
										// currently stopped
										if (press) {
											// start
											
											// set rocker long press timeout
											sr.timeout2 = time + dr.duration2;
											
											// use bit2 in pressed to store last direction and decide up/down on toggle
											if (up || (toggle && (state.pressed & bit2) == 0)) {
												// up with extra time
												sr.timeout1 = time + dr.duration1 - sr.duration + 500ms;
												state.relays |= bit1;
												state.pressed |= bit2;
											} else {
												// down with extra time
												sr.timeout1 = time + sr.duration + 500ms;
												state.relays |= bit2;
												state.pressed &= ~bit2;
											}
											nextTimeout = min(nextTimeout, sr.timeout1);
										}
									}
								}
							}
						} else if (s != 0) {
							// one or two relays
							bool on = false;
							bool change = false;
							char command = 0;
							if (value != null) {
								on = *value != 0.0f;
								change = true;
							} else if (length == 1) {
								command = data[0];
							}
							if (topicId == sr.commandId1) {
								// relay 1
								if (command != 0) {
									if ((state.pressed & bit1) == 0) {
										// not pressed
										bool toggle = command == '!';
										on = command == '+' || (toggle && (relays & bit1) == 0);
										if (toggle || on || command == '-') {
											// press with command on, off or toggle
											state.pressed |= bit1;
											change = true;
										}
									} else {
										// pressed
										if (command == '#') {
											// release
											state.pressed &= ~bit1;
										}
									}
								}
								if (change) {
									if (on) {
										// set on
										state.relays |= bit1;

										// set on timeout
										sr.timeout1 = time + dr.duration1;
										if (dr.duration1 > 0s)
											nextTimeout = min(nextTimeout, sr.timeout1);
									} else {
										// set off
										state.relays &= ~bit1;
									}

									// publish current switch state if it has changed
									if ((state.relays ^ relays) & bit1)
										publish(sr.stateId1, &offOn[int(on)], 1, 1, true);
								}
							} else {
								// relay 2
								if (command != 0) {
									if ((state.pressed & bit2) == 0) {
										// not pressed
										bool toggle = command == '!';
										on = command == '+' || (toggle && (relays & bit2) == 0);
										if (toggle || on || command == '-') {
											// press with command on, off or toggle
											state.pressed |= bit2;
											change = true;
										}
									} else {
										// pressed
										if (command == '#') {
											// release
											state.pressed &= ~bit2;
										}
									}
								}
								if (change) {
									if (on) {
										// set on
										state.relays |= bit2;
										
										// set on timeout
										sr.timeout2 = time + dr.duration2;
										if (dr.duration2 > 0s)
											nextTimeout = min(nextTimeout, sr.timeout2);
									} else {
										// set off
										state.relays &= ~bit2;
									}

									// publish current switch state if it has changed
									if ((state.relays ^ relays) & bit2)
										publish(sr.stateId2, &offOn[int(on)], 1, 1, true);
								}
							}
						}
					}
				}
				
				// send relay state to lin device if it has changed
				if (state.relays != relays) {
					linSend(device.id, &state.relays, 1);
				}
			}
			break;
		}
	}
	setSystemTimeout3(nextTimeout);
}


// Lin
// ---

void RoomControl::onLinReady() {
	// iterate over lin devices and check if they are in the list of local devices
	Array<Lin::Device> linDevices = getLinDevices();
	for (Lin::Device const &linDevice : linDevices) {
		// check if device exists
		bool found = false;
		for (LocalDeviceElement e : this->localDevices) {
			if (e.flash->id == linDevice.id) {
				found = true;
				break;
			}
		}
		
		// add new device if not found
		if (!found) {
			// clear device
			memset(&this->temp, 0, sizeof(this->temp));
		
			// set device id and type
			this->temp.linDevice = linDevice;

			// initialize name with hex id
			this->buffer << hex(linDevice.id);
			this->temp.localDevice.setName(this->buffer);
			this->buffer.clear();

this->temp.switch2Device.r[0].duration1 = 6500ms;
this->temp.switch2Device.r[0].duration2 = 3s;
this->temp.switch2Device.r[1].duration1 = 6500ms;
this->temp.switch2Device.r[1].duration2 = 3s;

			// add new device
			int index = this->localDevices.size();
			this->localDevices.write(index, &this->temp.localDevice);
		}
	}
	
	// todo: decide when to subscribe
	subscribeDevices();
}

void RoomControl::onLinReceived(uint32_t deviceId, uint8_t const *data, int length) {
	for (LocalDeviceElement e : this->localDevices) {
		if (e.flash->id == deviceId) {
			switch (e.flash->type) {
			case Lin::Device::SWITCH2:
				{
					//uint8_t const commands[] = "#+-";
					Switch2Device const &device = *reinterpret_cast<Switch2Device const *>(e.flash);
					Switch2State &state = *reinterpret_cast<Switch2State *>(e.ram);

					uint8_t switches = data[0];
					uint8_t flags = device.flags;
					uint8_t change = switches ^ state.switches;
					state.switches = switches;
					for (int i = 0; i < 2; ++i, switches >>= 2, flags >>= 2, change >>= 2) {
						Switch2State::Switches &ss = state.s[i];
						
						uint8_t s = flags & 3;
						if (s == 3) {
							// rocker switch
							uint8_t const commands[] = "#+-!";

							// check if state has changed
							if (change & 3) {
								publish(ss.stateId1, &commands[switches & 3], 1, 1, true);
							}
						} else if (s != 0) {
							// individual switches
							uint8_t const commands[] = "#!";

							// check if state has changed
							if (change & 1) {
								publish(ss.stateId1, &commands[switches & 1], 1, 1, true);
							}
							if (change & 2) {
								publish(ss.stateId2, &commands[(switches >> 1) & 1], 1, 1, true);
							}
						}
					}
				}
				break;
			}
		}
	}
}

void RoomControl::onLinSent() {

}

	
// SystemTimer
// -----------
		
void RoomControl::onSystemTimeout3(SystemTime time) {
	SystemTime nextTimeout = updateDevices(time);
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
			this->buffer << weekdays[time.getWeekday()] << "  "
				<< dec(time.getHours()) << ':'
				<< dec(time.getMinutes(), 2) << ':'
				<< dec(time.getSeconds(), 2);
			bitmap.drawText(20, 10, tahoma_8pt, this->buffer, 1);
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

		if (entry("Local Devices"))
			push(LOCAL_DEVICES);
		if (entry("Timers"))
			push(TIMERS);
		if (entry("Connections"))
			push(CONNECTIONS);
		if (entry("Exit")) {
			this->state = IDLE;
		}
		break;
	case LOCAL_DEVICES:
		menu(delta, activated);
		{
			// list devices
			int deviceCount = this->localDevices.size();
			for (int i = 0; i < deviceCount; ++i) {
				LocalDeviceElement e = this->localDevices[i];
				LocalDevice const &device = *e.flash;
				this->buffer << device.getName() << ": ";
				switch (device.type) {
				case Lin::Device::SWITCH2:
					this->buffer << "Switch";
					break;
				}
				if (entry(this->buffer)) {
					// edit device
					this->temp.localDevice = device;
					this->tempState = e.ram;
					push(EDIT_LOCAL_DEVICE);
				}
				/*DeviceState &deviceState = this->deviceStates[i];
				this->buffer = device.name, ": ";
				addDeviceStateToString(device, deviceState);
				if (entry(this->buffer)) {
					// edit device
					this->u.device = devices[this->selected];
					this->actions = &this->u.device.actions;
					push(EDIT_DEVICE);
				}*/
				this->buffer.clear();
			}
			/*if (deviceCount < DEVICE_COUNT && this->storage.hasSpace(1, offsetof(Device, actions.actions[8]))) {
				if (entry("Add Device")) {
					this->u.device.id = newDeviceId();
					this->u.device.type = Device::Type::SWITCH;
					copy("New Device", this->u.device.name);
					this->u.device.value1 = 0;
					this->u.device.value2 = 0;
					this->u.device.output = 0;
					this->u.device.actions.clear();
					this->actions = &this->u.device.actions;
					push(ADD_DEVICE);
				}
			}*/
			if (entry("Exit"))
				pop();
		}
		break;
	case EDIT_LOCAL_DEVICE:
		menu(delta, activated);

		// device name
		entry(this->temp.localDevice.getName());
		
		// device specific properties
		switch (this->temp.localDevice.type) {
		case Lin::Device::SWITCH2:
			{
				Switch2State *state = reinterpret_cast<Switch2State *>(this->tempState);
				line();
				entry("Switch A");
				entry("Type: Two Buttons");
				line();
				entry("Switch B");
				entry("Type: Rocker");
				line();
				entry("Relays A");
				entry("Type: Two Relays");
				line();
				entry("Relays B");
				entry("Type: Blind");
				line();
			}
			break;
		}
	
		// save changes / cancel
		if (entry("Save Device")) {
			this->localDevices.write(getThisIndex(), &this->temp.localDevice);
			pop();
		}
		if (entry("Cancel"))
			pop();

		break;
	case TIMERS:
		menu(delta, activated);
		{
			// list timers
			int timerCount = this->timers.size();
			for (int i = 0; i < timerCount; ++i) {
				TimerElement e = this->timers[i];
				Timer const &timer = *e.flash;
				
				int minutes = timer.time.getMinutes();
				int hours = timer.time.getHours();
				this->buffer << dec(hours) << ':' << dec(minutes, 2);
				this->buffer << "    ";
				if (entryWeekdays(this->buffer, timer.time.getWeekdays())) {
					// edit timer
					this->temp.timer = timer;
					this->tempState = e.ram;
					push(EDIT_TIMER);
				}

				this->buffer.clear();
			}
			
			// add timer
			if (entry("Add Timer")) {
				// check if list of timers is full
				if (this->timers.size() < MAX_TIMER_COUNT) {
					this->temp.timer.time = {};
					
					// check for space for a timer with one command and topic length of 64
					this->temp.timer.commandCount = 1;
					this->temp.timer.u.commands[0].topicLength = 64;
					if (timers.hasSpace(&this->temp.timer)) {
						// clear commands again
						this->temp.timer.commandCount = 0;
						push(ADD_TIMER);
					} else {
						// error: out of memory
						// todo
					}
				} else {
					// error: maximum number of timers reached
					// todo
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
			TimerState &timerState = *(TimerState*)this->tempState;
			
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
			this->buffer << "Time: ";
			editBegin[1] = this->buffer.length();
			this->buffer << dec(hours);
			editEnd[1] = this->buffer.length();
			this->buffer << ':';
			editBegin[2] = this->buffer.length();
			this->buffer << dec(minutes, 2);
			editEnd[2] = this->buffer.length();
			entry(this->buffer, edit > 0, editBegin[edit], editEnd[edit]);
			this->buffer.clear();

			// edit weekdays
			edit = getEdit(7);
			if (edit > 0 && delta != 0)
				weekdays ^= 1 << (edit - 1);

			// weekdays menu entry
			this->buffer << "Days: ";
			entryWeekdays(this->buffer, weekdays, edit > 0, this->edit - 1);
			this->buffer.clear();

			// write back
			timer.time = makeClockTime(weekdays, hours, minutes);

			// commands
			line();
			{
				uint8_t *buffer = timer.u.buffer;
				uint8_t *end = array::end(timer.u.buffer);
				uint8_t *data = buffer + sizeof(Command) * timer.commandCount;
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
					editBegin[1] = this->buffer.length();
					switch (command.type) {
					case Command::BUTTON:
						// button state is release or press, one data byte
						this->buffer << "Button: ";
						editEnd[1] = this->buffer.length() - 1;

						if (edit == 2 && delta != 0)
							value[1] ^= 1;
						
						editBegin[2] = this->buffer.length();
						this->buffer << buttonStates[data[0] & 1];
						editEnd[2] = this->buffer.length();
						break;
					case Command::ROCKER:
						// rocker state is release, up or down, one data byte
						this->buffer << "Rocker: ";
						editEnd[1] = this->buffer.length() - 1;

						if (edit == 2)
							value[0] = (value[0] + delta + 30) % 3;
						
						editBegin[2] = this->buffer.length();
						this->buffer << rockerStates[data[0] & 3];
						editEnd[2] = this->buffer.length();
						break;
					case Command::BINARY:
						// binary state is off or on, one data byte
						this->buffer << "Binary: ";
						editEnd[1] = this->buffer.length() - 1;

						if (edit == 2 && delta != 0)
							value[0] ^= 1;

						editBegin[2] = this->buffer.length();
						this->buffer << binaryStates[data[0] & 1];
						editEnd[2] = this->buffer.length();
						break;
					case Command::VALUE8:
						// value 0-255, one data byte
						this->buffer << "Value 0-255: ";
						editEnd[1] = this->buffer.length() - 1;

						if (edit == 2)
							value[0] += delta;
						
						editBegin[2] = this->buffer.length();
						this->buffer << dec(data[0]);
						editEnd[2] = this->buffer.length();
						break;
					case Command::PERCENTAGE:
						// percentage 0-100, one data byte
						this->buffer << "Percentage: ";
						editEnd[1] = this->buffer.length() - 1;

						if (edit == 2)
							value[0] = (value[0] + delta + 100) % 100;
						
						editBegin[2] = this->buffer.length();
						this->buffer << dec(data[0]) << '%';
						editEnd[2] = this->buffer.length();
						break;
					case Command::TEMPERATURE:
						// room temperature 8° - 33,5°
						this->buffer << "Temperature: ";
						editEnd[1] = this->buffer.length() - 1;

						if (edit == 2)
							value[0] += delta;
						
						editBegin[2] = this->buffer.length();
						this->buffer << dec(8 + data[0] / 10) << '.' << dec(data[0] % 10);
						editEnd[2] = this->buffer.length();
						this->buffer << "oC";
						break;
					case Command::COLOR_RGB:
						this->buffer << "RGB Color: ";
						editEnd[1] = this->buffer.length() - 1;

						if (edit == 2)
							value[0] += delta;
						if (edit == 3)
							value[1] += delta;
						if (edit == 4)
							value[2] += delta;

						editBegin[2] = this->buffer.length();
						this->buffer << dec(data[0]);
						editEnd[2] = this->buffer.length();
						this->buffer << ' ';
						
						editBegin[3] = this->buffer.length();
						this->buffer << dec(data[1]);
						editEnd[3] = this->buffer.length();
						this->buffer << ' ';

						editBegin[4] = this->buffer.length();
						this->buffer << dec(data[2]);
						editEnd[4] = this->buffer.length();
						
						break;
					case Command::TYPE_COUNT:
						;
					}
					entry(this->buffer, edit > 0, editBegin[edit], editEnd[edit]);
					this->buffer.clear();
					
					// menu entry for topic
					if (entry(!topic.empty() ? topic : "Select Topic")) {
						// check if current topic has the format <room>/<device>/<attribute>
						int p1 = topic.indexOf('/');
						int p2 = topic.indexOf('/', p1 + 1);
						int p3 = topic.indexOf('/', p2 + 1);
						
						String rd;
						if (p1 > 0 && p2 > p1 + 1 && p3 == -1) {
							// get <room>/<device>
							rd = topic.substring(0, p2);
							this->topicDepth = 2;
						} else {
							// start at this room
							rd = this->room[0].flash->name;
							this->topicDepth = 1;
						}

						// selected topic is <prefix>/<room>/<device> or <prefix>/<room>
						this->selectedTopic << prefix << '/' << rd;
						
						// only list attributes that have a command topic (ending with /cmd)
						this->onlyCommands = true;

						// subscribe to selected topic
						this->selectedTopicId = subscribeTopic(this->selectedTopic).topicId;
						publish(this->selectedTopicId, "?", 1);
						push(SELECT_TOPIC);
						this->tempIndex = commandIndex;
					}
					
					// menu entry for testing the command
					if (entry("Test")) {
						publishValue(timerState.topicIds[commandIndex], command.type, value);
					}
					
					// menu entry for deleting the command
					int size = command.topicLength + valueSize;
					if (entry("Delete Command")) {
						// erase command
						//array::erase(buffer, data - buffer, sizeof(Command) * commandIndex, sizeof(Command));
						array::erase(buffer + sizeof(Command) * commandIndex, data, sizeof(Command));
						--timer.commandCount;
						data -= sizeof(Command);

						// erase data
						//array::erase(buffer, end - buffer, data - buffer, sizeof(Command) + size);
						array::erase(data, buffer, sizeof(Command) + size);
					
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
						//array::insert(buffer, end - buffer, sizeof(Command) * commandIndex, sizeof(Command));
						array::insert(buffer + sizeof(Command) * commandIndex, end, sizeof(Command));
						data += sizeof(Command);

						// initialize new command
						Command &command = timer.u.commands[commandIndex];
						command.type = Command::BINARY;
						command.topicLength = 0;
						*data = 0;
						
						++timer.commandCount;
					}
					line();
				}
			}
			
			// save, delete, cancel
			if (entry("Save Timer")) {
				this->timers.write(getThisIndex(), &this->temp.timer);
				pop();
			}
			if (this->state == EDIT_TIMER) {
				if (entry("Delete Timer")) {
					this->timers.erase(getThisIndex());
					pop();
				}
			}
			if (entry("Cancel"))
				pop();
		}
		break;
	case SELECT_TOPIC:
	//case SELECT_COMMAND_TOPIC:
		menu(delta, activated);
		
		// menu entry for "parent directory" (select device or room)
		if (this->topicDepth > 0) {
			static char const* components[] = {"Room", "Device"};
			this->buffer << "Select " << components[this->topicDepth - 1];
			if (entry(this->buffer)) {
				// unsubscribe from current topic
				unsubscribeTopic(this->selectedTopic);

				// go to "parent directory"
				--this->topicDepth;
				this->selectedTopic.setLength(this->selectedTopic.lastIndexOf('/'));
				
				// subscribe to new topic
				this->selectedTopicId = subscribeTopic(this->selectedTopic).topicId;

				// request the new topic list, gets filled in onPublished
				this->topicSet.clear();
				publish(this->selectedTopicId, "?", 1);
			}
			this->buffer.clear();
		}

		// menu entry for each room/device/attribute
		String selected;
		for (String topic : this->topicSet) {
			if (entry(topic))
				selected = topic;
		}
		if (!selected.empty()) {
			++this->topicDepth;
			
			// unsubscribe from selected topic if it is not one of the own global topics
			RoomState *roomState = this->room[0].ram;
			if (this->selectedTopicId != roomState->houseTopicId && this->selectedTopicId != roomState->roomTopicId)
				unsubscribeTopic(this->selectedTopic);
			
			// append new element (room, device or attribute) to selected topic
			this->selectedTopic << '/' << selected;
			
			// clear list of elements
			this->topicSet.clear();
			
			if (this->topicDepth < 3) {
				// enter next level
				this->selectedTopicId = subscribeTopic(this->selectedTopic).topicId;
				publish(this->selectedTopicId, "?", 1);
			} else {
				// exit menu and write topic back to command
				pop();
				State state = this->stack[this->stackIndex].state;
				
				switch (state) {
				case EDIT_TIMER:
				case ADD_TIMER:
					{
						Timer &timer = this->temp.timer;
						TimerState &timerState = *(TimerState*)this->tempState;
						
						// set topic to command
						int commandIndex = this->tempIndex;
						Command &command = timer.u.commands[commandIndex];
						uint8_t *buffer = timer.u.buffer;
						uint8_t *end = array::end(timer.u.buffer);
						uint8_t *data = buffer + sizeof(Command) * timer.commandCount;
						for (int i = 0; i < commandIndex; ++i)
							data += timer.u.commands[i].getFlashSize();
						data += command.getValueSize();
						String topic = this->selectedTopic.string().substring(String(prefix).length + 1);
						command.setTopic(topic, data, end);
						
						// register topic for command
						this->selectedTopic << "/cmd";
						timerState.topicIds[commandIndex] = registerTopic(this->selectedTopic).topicId;
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
		
		if (entry("Cancel")) {
			unsubscribeTopic(this->selectedTopic);
			this->selectedTopic.clear();
			this->selectedTopicId = 0;
			this->topicSet.clear();
			pop();
		}

		break;

/*
	case EVENTS:
		{
			menu(delta, activated);
			
			// list events
			for (const Event &event : this->events) {
				if (event.input < INPUT_COUNT) {
					this->buffer = "Motion Detector";
				} else if (event.input == INPUT_COUNT) {
					this->buffer = "(No Input)";
				} else {
					this->buffer = hex(event.input), ':', hex(event.value);
				}
				if (entry(this->buffer)) {
					// edit event
					this->u.event = this->events[this->selected];
					this->actions = &u.event.actions;
					push(EDIT_EVENT);
				}
			}
			if (this->events.size() < EVENT_COUNT && this->storage.hasSpace(1, offsetof(Event, actions.actions[8]))) {
				if (entry("Add Event")) {
					this->u.event.input = INPUT_COUNT;
					this->u.event.value = 0;
					this->u.event.actions.clear();
					this->actions = &u.event.actions;
					push(ADD_EVENT);
				}
			}
			if (entry("Exit"))
				pop();
		}
		break;
	case EDIT_EVENT:
	case ADD_EVENT:
		{
			menu(delta, activated);

			// edit input
			bool editEntry = isEdit();
			if (editEntry && delta != 0) {
				uint32_t i = this->u.event.input + delta;
				this->u.event.input = i > INPUT_COUNT ? (i < 0xfffffff0 ? INPUT_COUNT : 0) : i;
				this->u.event.value = 0;
			}

			// input menu entry
			this->buffer = "Input: ";
			int begin = this->buffer.length();
			if (this->u.event.input < INPUT_COUNT) {
				this->buffer += "Motion Detector";
			} else if (this->u.event.input == INPUT_COUNT) {
				if (editEntry)
					this->buffer += "Enocean...";
				else
					this->buffer += "(No Input)";
			} else {
				this->buffer += hex(this->u.event.input), ':', hex(this->u.event.value);
			}
			int end = this->buffer.length();
			entry(this->buffer, editEntry, begin, end);

			// list actions of event
			listActions();

			if (entry("Save Event")) {
				this->events.write(getThisIndex(), this->u.event);
				pop();
			}
			if (this->state == EDIT_EVENT) {
				if (entry("Delete Event")) {
					this->events.erase(getThisIndex());
					pop();
				}
			}
			if (entry("Cancel"))
				pop();
		}
		break;

	case TIMERS:
		{
			menu(delta, activated);
			
			// list timers
			for (const Timer &timer : this->timers) {
				int minutes = (timer.time & Clock::MINUTES_MASK) >> Clock::MINUTES_SHIFT;
				int hours = (timer.time & Clock::HOURS_MASK) >> Clock::HOURS_SHIFT;
				this->buffer = bcd(hours) , ':' , bcd(minutes, 2);
				this->buffer += "    ";
				if (entryWeekdays(this->buffer, timer.weekdays)) {
					// edit timer
					this->u.timer = this->timers[this->selected];
					this->actions = &this->u.timer.actions;
					push(EDIT_TIMER);
				}
			}
			if (this->timers.size() < TIMER_COUNT && this->storage.hasSpace(1, offsetof(Timer, actions.actions[8]))) {
				if (entry("Add Timer")) {
					this->u.timer.time = 0;
					this->u.timer.weekdays = 0;
					this->u.timer.actions.clear();
					this->actions = &this->u.timer.actions;
					push(ADD_TIMER);
				}
			}
			if (entry("Exit"))
				pop();
		}
		break;
	case EDIT_TIMER:
	case ADD_TIMER:
		{
			menu(delta, activated);

			// time
			{
				// get time
				int hours = (this->u.timer.time & Clock::HOURS_MASK) >> Clock::HOURS_SHIFT;
				int minutes = (this->u.timer.time & Clock::MINUTES_MASK) >> Clock::MINUTES_SHIFT;

				// edit time
				bool editEntry = isEdit(2);
				if (editEntry) {
					if (this->edit == 1)
						hours = Clock::addTime(hours, delta, 0x23);
					else
						minutes = Clock::addTime(minutes, delta, 0x59);

					// write back
					this->u.event.input = (hours << Clock::HOURS_SHIFT) | (minutes << Clock::MINUTES_SHIFT);
				}

				// time menu entry
				this->buffer = "Time: ";
				int begin1 = this->buffer.length();
				this->buffer += bcd(hours);
				int end1 = this->buffer.length();
				this->buffer += ':';
				int begin2 = this->buffer.length();
				this->buffer += bcd(minutes, 2);
				int end2 = this->buffer.length();
				entry(this->buffer, editEntry, this->edit == 1 ? begin1 : begin2, this->edit == 1 ? end1 : end2);
			}

			// weekdays
			{
				// edit weekdays
				bool editEntry = isEdit(7);
				if (editEntry) {
					// edit weekday
					if (delta != 0)
						this->u.timer.weekdays ^= 1 << (this->edit - 1);
				}

				// get weekdays
				int weekdays = this->u.timer.weekdays;

				// weekdays menu entry
				this->buffer = "Days: ";
				entryWeekdays(this->buffer, weekdays, editEntry, this->edit - 1);
			}

			// list actions of timer
			listActions();

			if (entry("Save Timer")) {
				this->timers.write(getThisIndex(), this->u.timer);
				pop();
			}
			if (this->state == EDIT_EVENT) {
				if (entry("Delete Timer")) {
					this->timers.erase(getThisIndex());
					pop();
				}
			}
			if (entry("Cancel"))
				pop();
		}
		break;

	case SCENARIOS:
		{
			int scenarioCount = this->scenarios.size();

			menu(delta, activated);
			for (int i = 0; i < scenarioCount; ++i) {
				const Scenario &scenario = scenarios[i];
				this->buffer = scenario.name;
				if (entry(this->buffer)) {
					// edit scenario
					this->u.scenario = scenarios[this->selected];
					this->actions = &this->u.scenario.actions;
					push(EDIT_SCENARIO);
				}
			}
			if (scenarioCount < SCENARIO_COUNT && this->storage.hasSpace(1, offsetof(Scenario, actions.actions[8]))) {
				if (entry("Add Scenario")) {
					this->u.device.id = newScenarioId();
					copy("New Scenario", this->u.device.name);
					this->u.scenario.actions.clear();
					this->actions = &this->u.scenario.actions;
					push(ADD_SCENARIO);
				}
			}
			if (entry("Exit"))
				pop();
		}
		break;
	case EDIT_SCENARIO:
	case ADD_SCENARIO:
		{
			menu(delta, activated);

			this->buffer = "Name: ", this->u.scenario.name;
			entry(this->buffer);

			// list actions of scenario
			listActions();

			if (entry("Save Scenario")) {
				// save
				this->scenarios.write(getThisIndex(), this->u.scenario);
				pop();
			}
			if (this->state == EDIT_SCENARIO)
				entry("Delete Scenario");
			if (entry("Cancel"))
				pop();
		}
		break;

	case SELECT_ACTION:
	case ADD_ACTION:
		{
			menu(delta, activated);

			// list all device states and state transitions
			for (const Device &device : this->devices) {
				Array<Device::State> states = device.getActionStates();
				for (int i = 0; i < states.length; ++i) {
					this->buffer = device.name, ' ', string(states[i].name);
					if (entry(this->buffer)) {
						pop();
						this->actions->set(this->actionIndex, {device.id, states[i].state});
					}
				}

				Array<Device::Transition> transitions = device.getTransitions();
				for (int i = 0; i < transitions.length; ++i) {
					Device::Transition transition = transitions[i];
					this->buffer = device.name, ' ', device.getStateName(transition.fromState), " -> ",
						device.getStateName(transition.toState);
					if (entry(this->buffer)) {
						pop();
						this->actions->set(this->actionIndex, {device.id, uint8_t(Action::TRANSITION_START + i)});
					}
				}
			}
			
			// list all scenarios
			for (const Scenario &scenario : this->scenarios) {
				this->buffer = scenario.name;
				if (entry(this->buffer)) {
					pop();
					this->actions->set(this->actionIndex, {scenario.id, Action::SCENARIO});
				}
			}
			line();
	
			if (this->state == SELECT_ACTION) {
				if (entry("Delete Action")) {
				   pop();
				   this->actions->erase(this->actionIndex);
			   }
			}
			if (entry("Cancel"))
				pop();
		}
		break;

	case DEVICES:
		{
			// list devices
			menu(delta, activated);
			int deviceCount = this->devices.size();
			for (int i = 0; i < deviceCount; ++i) {
				const Device &device = this->devices[i];
				DeviceState &deviceState = this->deviceStates[i];
				this->buffer = device.name, ": ";
				addDeviceStateToString(device, deviceState);
				if (entry(this->buffer)) {
					// edit device
					this->u.device = devices[this->selected];
					this->actions = &this->u.device.actions;
					push(EDIT_DEVICE);
				}
			}
			if (deviceCount < DEVICE_COUNT && this->storage.hasSpace(1, offsetof(Device, actions.actions[8]))) {
				if (entry("Add Device")) {
					this->u.device.id = newDeviceId();
					this->u.device.type = Device::Type::SWITCH;
					copy("New Device", this->u.device.name);
					this->u.device.value1 = 0;
					this->u.device.value2 = 0;
					this->u.device.output = 0;
					this->u.device.actions.clear();
					this->actions = &this->u.device.actions;
					push(ADD_DEVICE);
				}
			}
			if (entry("Exit"))
				pop();
		}
		break;
	case EDIT_DEVICE:
	case ADD_DEVICE:
		{
			menu(delta, activated);

			label("Device");
			line();
			
			// device type
			{
				bool editEntry = isEdit();
				if (editEntry) {
					int type = clamp(int(this->u.device.type) + delta, 0, 4);
					this->u.device.type = Device::Type(type);
				}
				this->buffer = "Type: ";
				int begin = this->buffer.length();
				switch (this->u.device.type) {
					case Device::Type::SWITCH:
						this->buffer += "Switch";
						break;
					case Device::Type::LIGHT:
						this->buffer += "Light";
						break;
					case Device::Type::DIMMER:
						this->buffer += "Dimmer";
						break;
					case Device::Type::BLIND:
						this->buffer += "Blind";
						break;
					case Device::Type::HANDLE:
						this->buffer += "Handle";
						break;
				}
				int end = this->buffer.length();
				entry(this->buffer, editEntry, begin, end);
			}

			// device name
			this->buffer = "Name: ", this->u.device.name;
			entry(this->buffer);

			// device state
			this->buffer = "State: ";
			addDeviceStateToString(this->u.device, this->deviceStates[getThisIndex()]);
			entry(this->buffer);

			// device output (to local relay or enocean device)
			{
				bool editEntry = isEdit();
				if (editEntry && delta != 0) {
					uint32_t i = this->u.device.output + delta;
					this->u.device.output = i > OUTPUT_COUNT ? (i < 0xfffffff0 ? OUTPUT_COUNT : 0) : i;
				}
				this->buffer = "Output: ";
				int begin = this->buffer.length();
				if (this->u.device.output < OUTPUT_COUNT) {
					this->buffer += "Output ", int(this->u.device.output);
				} else if (this->u.device.output == OUTPUT_COUNT) {
					if (editEntry)
						this->buffer += "Enocean...";
					else
						this->buffer += "(No Output)";
				} else {
					this->buffer += hex(this->u.device.output);
				}
				int end = this->buffer.length();
				entry(this->buffer, editEntry, begin, end);
			}

			line();
			
			// list/edit conditions
			int conditionCount = this->u.device.actions.size();
			uint8_t id = this->u.device.id;
			if (conditionCount > 0 && this->u.device.actions[0].id != id) {
				this->buffer = string(this->u.device.getStates()[0].name), " when";
				label(this->buffer);
			}
			bool noEffect = false;
			for (int i = 0; i < conditionCount; ++i) {
				Action condition = this->u.device.actions[i];

				if (condition.id == id) {
					// check if constrained state is at last position
					bool last = (i == conditionCount - 1) || this->u.device.actions[i + 1].id == id;

					this->buffer.clear();
					String stateName = this->u.device.getStateName(condition.state);
					if (last && i == 0) {
						this->buffer += "Initial ", stateName;
					} else {
						if (!noEffect) {
							if (last)
								this->buffer += "Else ";
							this->buffer += stateName;
							if (!last)
								this->buffer += " When";
						} else {
							this->buffer += '(', stateName, ')';
						}
					}
					noEffect |= last;
				} else {
					setConstraint(this->u.device.actions[i]);
				}
				if (entry(this->buffer)) {
					// edit condition
					push(SELECT_CONDITION);
					this->selected = 0;
					this->actionIndex = i;
				}
			}
			if (conditionCount < Actions::ACTION_COUNT && this->storage.hasSpace(0, sizeof(Action))) {
				if (entry("Add Condition")) {
					push(ADD_CONDITION);
					this->selected = 0;
					this->actionIndex = conditionCount;
				}
			}
			line();
			
			if (entry("Save Device")) {
				this->devices.write(getThisIndex(), this->u.device);
				pop();
			}
			if (this->state == EDIT_DEVICE) {
				if (entry("Delete Device")) {
					deleteDeviceId(this->u.device.id);
					this->devices.erase(getThisIndex());
					pop();
				}
			}
			if (entry("Cancel"))
				pop();
		}
		break;
	case SELECT_CONDITION:
	case ADD_CONDITION:
		{
			menu(delta, activated);

			// list "own" device states
			{
				Array<Device::State> states = this->u.device.getActionStates();
				for (int i = 0; i < states.length; ++i) {
					this->buffer = string(states[i].name);
					if (entry(this->buffer)) {
						// set "own" device state
						pop();
						this->u.device.actions.set(this->actionIndex, {this->u.device.id, states[i].state});
					}
				}
			}
			line();
			
			// list device states of other devices
			for (const Device &device : this->devices) {
				if (device.id != this->u.device.id) {
					Array<Device::State> states = device.getStates();
					for (int i = 0; i < states.length; ++i) {
						this->buffer = device.name, ' ', string(states[i].name);
						if (entry(this->buffer)) {
							// set condition
							pop();
							this->u.device.actions.set(this->actionIndex, {device.id, states[i].state});
						}
					}
				}
			}
			
			line();
			if (this->state == SELECT_CONDITION) {
				if (entry("Remove Condition")) {
					pop();
					this->u.device.actions.erase(this->actionIndex);
				}
			}
			if (entry("Cancel"))
				pop();
		}
		break;
*/
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
	
void RoomControl::label(const char* s) {
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
			this->edit = this->edit < editCount ? this->edit + 1 : 0;
			this->activated = false;
		}
		return this->edit;
	}
	return 0;
}

void RoomControl::push(State state) {
	this->stack[this->stackIndex] = {this->state, this->selected, this->selectedY, this->yOffset};
	++this->stackIndex;
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


// Devices
// -------

int RoomControl::LocalDevice::getFlashSize() const {
	switch (this->type) {
	case Lin::Device::SWITCH2:
		{
			Switch2Device const &device = *reinterpret_cast<Switch2Device const *>(this);
			
			// determine length of name including trailing zero if it fits into fixed size string
			int length = array::size(device.name);
			for (int i = 0; i < array::size(device.name); ++i) {
				if (device.name[i] == 0) {
					length = i + 1;
					break;
				}
			}

			// return actual byte size of the struct
			return getOffset(Switch2Device, name[length]);
		}
	}
}

int RoomControl::LocalDevice::getRamSize() const {
	switch (this->type) {
	case Lin::Device::SWITCH2:
		return sizeof(Switch2State);
	}
}

String RoomControl::LocalDevice::getName() const {
	switch (this->type) {
	case Lin::Device::SWITCH2:
		return String(reinterpret_cast<Switch2Device const *>(this)->name);
	}
}

void RoomControl::LocalDevice::setName(String name) {
	switch (this->type) {
	case Lin::Device::SWITCH2:
		assign(reinterpret_cast<Switch2Device *>(this)->name, name);
		break;
	}
}

void RoomControl::LocalDevice::operator =(LocalDevice const &device) {
	memcpy(this, &device, device.getFlashSize());
}

// todo don't register all topics in one go, message buffer may overflow
void RoomControl::subscribeDevices() {
	
	// register/subscribe room topics
	RoomElement e = this->room[0];
	Room const &room = *e.flash;
	RoomState &roomState = *e.ram;

	// house topic (room name is published when "?" is received on this topic)
	this->buffer << prefix;
	//int lenght = this->buffer.length();
	roomState.houseTopicId = registerTopic(this->buffer).topicId;

	// toom topic (device names are published when "?" is received on this topic)
	this->buffer << '/' << room.name;
	roomState.roomTopicId = registerTopic(this->buffer).topicId;
	this->buffer.clear();


	// register/subscribe device topics
	for (int i = 0; i < this->localDevices.size(); ++i) {
		subscribeDevice(i);
	}

	// register timers
	for (int i = 0; i < this->timers.size(); ++i) {
		registerTimer(i);
	}

this->forwards[0].srcId = subscribeTopic("rc/room/00000001/a").topicId;
this->forwards[0].dstId = registerTopic("rc/room/00000001/x/cmd").topicId;
this->forwards[1].srcId = subscribeTopic("rc/room/00000001/b0").topicId;
this->forwards[1].dstId = registerTopic("rc/room/00000001/y0/cmd").topicId;
this->forwards[2].srcId = subscribeTopic("rc/room/00000001/b1").topicId;
this->forwards[2].dstId = registerTopic("rc/room/00000001/y1/cmd").topicId;
this->forwards[3].srcId = subscribeTopic("rc/room/00000002/a").topicId;
this->forwards[3].dstId = registerTopic("rc/room/00000002/x/cmd").topicId;
this->forwards[4].srcId = subscribeTopic("rc/room/00000002/b0").topicId;
this->forwards[4].dstId = registerTopic("rc/room/00000002/y/cmd").topicId;

}

void RoomControl::subscribeDevice(int index) {
	LocalDeviceElement e = this->localDevices[index];
	
	// <prefix>/<room>
	this->buffer << prefix << '/' << this->room[0].flash->name;
	{
		LocalDevice const &device = *reinterpret_cast<LocalDevice const *>(e.flash);
		DeviceState &state = *reinterpret_cast<DeviceState *>(e.ram);
		
		// <prefix>/<room>/<device>
		this->buffer << '/' << device.getName();
		state.deviceTopicId = registerTopic(this->buffer).topicId;
		//int l = this->buffer.length();

		// <prefix>/<room>/<device>/cmd
		//this->buffer << "/cmd";
		//state.deviceCommand = subscribeTopic(this->buffer).topicId;
		//this->buffer.setLength(l);
	}
	
	this->buffer << '/';
	int deviceLength = this->buffer.length();

	switch (e.flash->type) {
	case Lin::Device::SWITCH2:
		{
			Switch2Device const &device = *reinterpret_cast<Switch2Device const *>(e.flash);
			Switch2State &state = *reinterpret_cast<Switch2State *>(e.ram);
			
			uint8_t flags = device.flags;
			for (int i = 0; i < 2; ++i, flags >>= 2) {
				// switches
				Switch2State::Switches &ss = state.s[i];
				uint8_t s = flags & 3;
				if (s != 0) {
					this->buffer.setLength(deviceLength);
					this->buffer << "ab"[i];
					if (s == 2)
						this->buffer << '0';
					ss.stateId1 = registerTopic(this->buffer).topicId;
					if (s == 2) {
						this->buffer.setLength(deviceLength + 1);
						this->buffer << '1';
						ss.stateId2 = registerTopic(this->buffer).topicId;
					}
				}

				// relays
				Switch2State::Relays &sr = state.r[i];
				uint8_t r = (flags >> 4) & 3;
				if (r != 0) {
					this->buffer.setLength(deviceLength);
					this->buffer << "xy"[i];
					if (r == 2)
						this->buffer << '0';
					sr.stateId1 = subscribeTopic(this->buffer).topicId;
					this->buffer << "/cmd";
					sr.commandId1 = subscribeTopic(this->buffer).topicId;
					if (r == 2) {
						this->buffer.setLength(deviceLength + 1);
						this->buffer << '1';
						sr.stateId2 = subscribeTopic(this->buffer).topicId;
						this->buffer << "/cmd";
						sr.commandId2 = subscribeTopic(this->buffer).topicId;
					}
				}
			}
		}
		break;
	}
	this->buffer.clear();
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

	// iterate over local devices
	for (LocalDeviceElement e : this->localDevices) {
		switch (e.flash->type) {
		case Lin::Device::SWITCH2:
			{
				Switch2Device const &device = *reinterpret_cast<Switch2Device const *>(e.flash);
				Switch2State &state = *reinterpret_cast<Switch2State *>(e.ram);

				uint8_t const relays = state.relays;
				uint8_t relayBit1 = 0x01;
				uint8_t f = device.flags >> 4;
				for (int i = 0; i < 2; ++i) {
					Switch2Device::Relays const &dr = device.r[i];
					Switch2State::Relays &sr = state.r[i];
					uint8_t relayBit2 = relayBit1 << 1;

					if ((f & 3) == 3) {
						// blind (two interlocked relays)
						if (state.relays & (relayBit1 | relayBit2)) {
							// currently moving
							if (state.relays & relayBit1) {
								// currently moving up
								sr.duration += duration;
								if (sr.duration > dr.duration1)
									sr.duration = dr.duration1;

							} else {
								// currently moving down
								sr.duration -= duration;
								if (sr.duration < 0s)
									sr.duration = 0s;

							}
						
							// stop if run time has passed
							int decimalCount;
							if (time >= sr.timeout1) {
								// stop
								state.relays &= ~(relayBit1 | relayBit2);
								decimalCount = 3;
							} else {
								// calc next timeout
								nextTimeout = min(nextTimeout, sr.timeout1);
								decimalCount = 2;
							}
							
							if (report || time >= sr.timeout1) {
								// publish current position
								float pos = sr.duration / dr.duration1;
								this->buffer << flt(pos, 0, decimalCount);
								publish(sr.stateId1, this->buffer, 1, true);
								this->buffer.clear();
							}
						}

					} else if ((f & 3) != 0) {
						// one or two relays
						if (state.relays & relayBit1) {
							// relay 1 currently on, check if timeout elapsed
							if (dr.duration1 > 0s && time >= sr.timeout1) {
								state.relays &= ~relayBit1;
								
								// report state change
								publish(sr.stateId1, &offOn[0], 1, 1, true);
							}
						}
						if (state.relays & relayBit2) {
							// relay 2 currently on, check if timeout elapsed
							if (dr.duration2 > 0s && time >= sr.timeout2) {
								state.relays &= ~relayBit2;

								// report state change
								publish(sr.stateId2, &offOn[0], 1, 1, true);
							}
						}
					}

					relayBit1 <<= 2;
					f >>= 2;
				}

				// send relay state to lin device if it has changed
				if (state.relays != relays) {
					linSend(device.id, &state.relays, 1);
				}
			}
			break;
		}
	}
	return nextTimeout;
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

	array::copy(topic.begin(), topic.end(), data);
	this->topicLength = newSize;
}

void RoomControl::publishValue(uint16_t topicId, Command::Type type, uint8_t const *value) {
	switch (type) {
	case Command::BUTTON:
		this->buffer << "#!"[*value];
		break;
	case Command::ROCKER:
		this->buffer << "#+-"[*value];
		break;
	case Command::BINARY:
		this->buffer << "01"[*value];
		break;
	case Command::VALUE8:
		this->buffer << dec(*value);
		break;
	case Command::PERCENTAGE:
		this->buffer << flt(*value * 0.01f, 0, 2);
		break;
	case Command::TEMPERATURE:
		this->buffer << flt(8.0f + *value * 0.1f, 0, 1);
		break;
	case Command::COLOR_RGB:
		this->buffer << "rgb(" << dec(value[0]) << ',' << dec(value[1]) << ',' << dec(value[2]) << ')';
		break;
	case Command::TYPE_COUNT:
		;
	}
	
	publish(topicId, this->buffer, 1);
	this->buffer.clear();
}


// Timers
// ------

int RoomControl::Timer::getFlashSize() const {
	int size = sizeof(Command) * this->commandCount;
	for (int i = 0; i < this->commandCount; ++i) {
		Command const &command = this->u.commands[i];
		size += command.getFlashSize();
	}
	return intptr_t(&((Timer*)nullptr)->u.buffer[size]);
}

int RoomControl::Timer::getRamSize() const {
	return intptr_t(&((TimerState*)nullptr)->topicIds[this->commandCount]);
}

void RoomControl::registerTimer(int index) {
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
		String topic(String(data + valueSize, command.topicLength));

		// register topic (so that we can publish on timer event)
		this->buffer << prefix << '/' << topic << "/cmd";
		timerState->topicIds[commandIndex] = registerTopic(this->buffer).topicId;
		this->buffer.clear();
		
		data += valueSize + command.topicLength;
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
