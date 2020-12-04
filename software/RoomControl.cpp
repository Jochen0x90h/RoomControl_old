//#include <iostream>
#include "RoomControl.hpp"

// font
#include "tahoma_8pt.hpp"

constexpr String weekdays[7] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
constexpr String weekdaysShort[7] = {"M", "T", "W", "T", "F", "S", "S"};
constexpr String buttonStates[] = {"release", "press"};
constexpr String rockerStates[] = {"release", "up", "down", ""};
constexpr String binaryStates[] = {"off", "on"};

constexpr uint8_t offOn[2] = {'0', '1'};

constexpr int8_t QOS = 1;


RoomControl::RoomControl()
	: storage(0, FLASH_PAGE_COUNT, room, localDevices, routes, timers)
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
		publish(roomState.houseTopicId, this->room[0].flash->name, QOS);
		return;
	}

	// check for room command
	if (topicId == roomState.roomTopicId && message == "?") {
		// publish device names
		for (int i = 0; i < this->localDevices.size(); ++i) {
			LocalDeviceElement e = this->localDevices[i];
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
							StringBuffer<8> b;
							b = "ab"[i];
							if (s == 2)
								b += '0';
							b += " s"; // only state
							publish(state.deviceTopicId, b, QOS);
							if (s == 2) {
								b[1] = '1';
								publish(state.deviceTopicId, b, QOS);
							}
						}

						// relays
						Switch2State::Relays &sr = state.r[i];
						uint8_t r = (flags >> 4) & 3;
						if (r != 0) {
							StringBuffer<8> b;
							b = "xy"[i];
							if (r == 2)
								b += '0';
							b += " c"; // command and state
							publish(state.deviceTopicId, b, QOS);
							if (r == 2) {
								b[1] = '1';
								publish(state.deviceTopicId, b, QOS);
							}
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
											//int pos = (sr.duration * 100 + dr.duration1 / 2) / dr.duration1;
											float pos = sr.duration / dr.duration1;
											//std::cout << "pos " << pos << std::endl;
											StringBuffer<8> b = flt(pos, 0, 3);
											publish(sr.stateId1, b, QOS, true);
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
										publish(sr.stateId1, &offOn[int(on)], 1, QOS, true);
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
										publish(sr.stateId2, &offOn[int(on)], 1, QOS, true);
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
			StringBuffer<8> b = hex(linDevice.id);
			this->temp.localDevice.setName(b);
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
								publish(ss.stateId1, &commands[switches & 3], 1, QOS, true);
							}
						} else if (s != 0) {
							// individual switches
							uint8_t const commands[] = "#!";

							// check if state has changed
							if (change & 1) {
								publish(ss.stateId1, &commands[switches & 1], 1, QOS, true);
							}
							if (change & 2) {
								publish(ss.stateId2, &commands[(switches >> 1) & 1], 1, QOS, true);
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

		if (entry("Local Devices"))
			push(LOCAL_DEVICES);
		if (entry("Routes"))
			push(ROUTES);
		if (entry("Timers"))
			push(TIMERS);
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
				StringBuffer<24> b = device.getName() + ": ";
				switch (device.type) {
				case Lin::Device::SWITCH2:
					this->buffer += "Switch";
					break;
				}
				if (entry(this->buffer)) {
					// edit device
					this->temp.localDevice = device;
					this->tempState.localDevice = *e.ram; // todo polymorphic assignment 
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
				//Switch2State *state = reinterpret_cast<Switch2State *>(this->tempState);
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
					copy(this->temp.route, this->tempState.route, route, *e.ram);
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

			String srcTopic = route.getSrcTopic();
			String dstTopic = route.getDstTopic();
			if (entry(this->tempState.route.srcTopicId != 0 ? srcTopic : "Select Source Topic")) {
				enterTopicSelector(srcTopic, false, 0);
			}
			if (entry(this->tempState.route.dstTopicId != 0 ? dstTopic : "Select Destination Topic")) {
				enterTopicSelector(dstTopic, true, 1);
			}
			
			// save, delete, cancel
			if (this->tempState.route.srcTopicId != 0 && this->tempState.route.dstTopicId != 0) {
				if (entry("Save Route")) {
					int index = getThisIndex();
					
					// unsubscribe old topic
					if (index < this->routes.size()) {
						TopicBuffer topic = this->routes[index].flash->getSrcTopic();
						unsubscribeTopic(topic.status());
					}
					this->routes.write(index, &this->temp.route, &this->tempState.route);
					pop();
				}
			}
			if (this->state == EDIT_ROUTE) {
				if (entry("Delete Route")) {
					int index = getThisIndex();

					// unsubscribe new topic
					TopicBuffer topic = this->temp.route.getSrcTopic();
					unsubscribeTopic(topic.status());

					// unsubscribe old topic
					topic = this->routes[index].flash->getSrcTopic();
					unsubscribeTopic(topic.status());
					
					this->routes.erase(getThisIndex());
					pop();
				}
			}
			if (entry("Cancel")) {
				// unsubscribe new topic
				TopicBuffer topic = this->temp.route.getSrcTopic();
				unsubscribeTopic(topic.status());
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
					copy(this->temp.timer, this->tempState.timer, timer, *e.ram);
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
			this->buffer += dec(hours);
			editEnd[1] = this->buffer.length();
			this->buffer += ':';
			editBegin[2] = this->buffer.length();
			this->buffer += dec(minutes, 2);
			editEnd[2] = this->buffer.length();
			entry(this->buffer, edit > 0, editBegin[edit], editEnd[edit]);
			this->buffer.clear();

			// edit weekdays
			edit = getEdit(7);
			if (edit > 0 && delta != 0)
				weekdays ^= 1 << (edit - 1);

			// weekdays menu entry
			b = "Days: ";
			entryWeekdays(this->buffer, weekdays, edit > 0, this->edit - 1);
			this->buffer.clear();

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
					editBegin[1] = this->buffer.length();
					switch (command.type) {
					case Command::BUTTON:
						// button state is release or press, one data byte
						b = "Button: ";
						editEnd[1] = this->buffer.length() - 1;

						if (edit == 2 && delta != 0)
							value[1] ^= 1;
						
						editBegin[2] = this->buffer.length();
						b += buttonStates[data[0] & 1];
						editEnd[2] = this->buffer.length();
						break;
					case Command::ROCKER:
						// rocker state is release, up or down, one data byte
						b = "Rocker: ";
						editEnd[1] = this->buffer.length() - 1;

						if (edit == 2)
							value[0] = (value[0] + delta + 30) % 3;
						
						editBegin[2] = this->buffer.length();
						b += rockerStates[data[0] & 3];
						editEnd[2] = this->buffer.length();
						break;
					case Command::BINARY:
						// binary state is off or on, one data byte
						b = "Binary: ";
						editEnd[1] = this->buffer.length() - 1;

						if (edit == 2 && delta != 0)
							value[0] ^= 1;

						editBegin[2] = this->buffer.length();
						b += binaryStates[data[0] & 1];
						editEnd[2] = this->buffer.length();
						break;
					case Command::VALUE8:
						// value 0-255, one data byte
						b = "Value 0-255: ";
						editEnd[1] = this->buffer.length() - 1;

						if (edit == 2)
							value[0] += delta;
						
						editBegin[2] = this->buffer.length();
						b += dec(data[0]);
						editEnd[2] = this->buffer.length();
						break;
					case Command::PERCENTAGE:
						// percentage 0-100, one data byte
						b = "Percentage: ";
						editEnd[1] = this->buffer.length() - 1;

						if (edit == 2)
							value[0] = (value[0] + delta + 100) % 100;
						
						editBegin[2] = this->buffer.length();
						b += dec(data[0]) + '%';
						editEnd[2] = this->buffer.length();
						break;
					case Command::TEMPERATURE:
						// room temperature 8° - 33,5°
						b = "Temperature: ";
						editEnd[1] = this->buffer.length() - 1;

						if (edit == 2)
							value[0] += delta;
						
						editBegin[2] = this->buffer.length();
						b += dec(8 + data[0] / 10) + '.' + dec(data[0] % 10);
						editEnd[2] = this->buffer.length();
						b += "oC";
						break;
					case Command::COLOR_RGB:
						b = "RGB Color: ";
						editEnd[1] = this->buffer.length() - 1;

						if (edit == 2)
							value[0] += delta;
						if (edit == 3)
							value[1] += delta;
						if (edit == 4)
							value[2] += delta;

						editBegin[2] = this->buffer.length();
						b += dec(data[0]);
						editEnd[2] = this->buffer.length();
						b += ' ';
						
						editBegin[3] = this->buffer.length();
						b += dec(data[1]);
						editEnd[3] = this->buffer.length();
						b += ' ';

						editBegin[4] = this->buffer.length();
						b += dec(data[2]);
						editEnd[4] = this->buffer.length();
						
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
						command.type = Command::BINARY;
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
				unsubscribeTopic(this->selectedTopic.enumeration());

				// go to "parent directory"
				this->selectedTopic.removeLast();
				--this->topicDepth;
				
				// subscribe to new topic
				this->selectedTopicId = subscribeTopic(this->selectedTopic.enumeration(), QOS).topicId;

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
			//if (this->selectedTopicId != roomState->houseTopicId && this->selectedTopicId != roomState->roomTopicId)
			unsubscribeTopic(this->selectedTopic.enumeration());
			
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
			//if (this->selectedTopicId != roomState->houseTopicId && this->selectedTopicId != roomState->roomTopicId)
			unsubscribeTopic(this->selectedTopic.enumeration());
			
			// append new element (room, device or attribute) to selected topic
			this->selectedTopic /= selected;// << '/' << selected;
			++this->topicDepth;

			// clear list of elements
			this->topicSet.clear();
			
			if (this->topicDepth < 3) {
				// enter next level
				this->selectedTopicId = subscribeTopic(this->selectedTopic.enumeration(), QOS).topicId;
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
						RouteState &routeState = this->tempState.route;
						
						if (this->tempIndex == 0) {
							TopicBuffer oldTopic = route.getSrcTopic();
							//if (routeState.srcTopicId != 0)
							unsubscribeTopic(oldTopic.status());
							route.setSrcTopic(topic);
							routeState.srcTopicId = subscribeTopic(this->selectedTopic.status(), QOS).topicId;
						} else {
							route.setDstTopic(topic);
							routeState.dstTopicId = registerTopic(this->selectedTopic.command()).topicId;
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
						
						// register topic for command
						timerState.topicIds[commandIndex] = registerTopic(this->selectedTopic.command()).topicId;
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
	TopicBuffer topic;
	roomState.houseTopicId = subscribeTopic(topic.enumeration(), QOS).topicId;

	// toom topic (device names are published when "?" is received on this topic)
	topic /= room.name;
	roomState.roomTopicId = subscribeTopic(topic.enumeration(), QOS).topicId;


	// register/subscribe device topics
	for (int i = 0; i < this->localDevices.size(); ++i) {
		subscribeDevice(i);
	}

	// register routes
	for (int i = 0; i < this->routes.size(); ++i) {
		initRoute(i);
	}

	// register timers
	for (int i = 0; i < this->timers.size(); ++i) {
		initTimer(i);
	}
}

void RoomControl::subscribeDevice(int index) {
	LocalDeviceElement e = this->localDevices[index];
	
	// register device (<prefix>/<room>/<device>)
	TopicBuffer topic = this->room[0].flash->name / e.flash->getName();
	e.ram->deviceTopicId = subscribeTopic(topic.enumeration(), QOS).topicId;

	// register/subscribe device attributes (<prefix>/<room>/<device>/<attribute>)
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
					StringBuffer<4> b = "ab"[i];
					if (s == 2)
						b += '0';
					topic /= b;
					ss.stateId1 = registerTopic(topic.status()).topicId;
					topic.removeLast();
					if (s == 2) {
						b[1] = '1';
						topic /= b;
						ss.stateId2 = registerTopic(topic.status()).topicId;
						topic.removeLast();
					}
				}

				// relays
				Switch2State::Relays &sr = state.r[i];
				uint8_t r = (flags >> 4) & 3;
				if (r != 0) {
					StringBuffer<4> b = "xy"[i];
					if (r == 2)
						b += '0';
					topic /= b;
					sr.stateId1 = registerTopic(topic.status()).topicId;
					sr.commandId1 = subscribeTopic(topic.command(), QOS).topicId;
					topic.removeLast();
					if (r == 2) {
						b[1] = '1';
						topic /= b;
						sr.stateId2 = registerTopic(topic.status()).topicId;
						sr.commandId2 = subscribeTopic(topic.command(), QOS).topicId;
						topic.removeLast();
					}
				}
			}
		}
		break;
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
								StringBuffer<8> b = flt(pos, 0, decimalCount);
								publish(sr.stateId1, b, QOS, true);
							}
						}

					} else if ((f & 3) != 0) {
						// one or two relays
						if (state.relays & relayBit1) {
							// relay 1 currently on, check if timeout elapsed
							if (dr.duration1 > 0s && time >= sr.timeout1) {
								state.relays &= ~relayBit1;
								
								// report state change
								publish(sr.stateId1, &offOn[0], 1, QOS, true);
							}
						}
						if (state.relays & relayBit2) {
							// relay 2 currently on, check if timeout elapsed
							if (dr.duration2 > 0s && time >= sr.timeout2) {
								state.relays &= ~relayBit2;

								// report state change
								publish(sr.stateId2, &offOn[0], 1, QOS, true);
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

void RoomControl::initRoute(int index) {
	auto e = this->routes[index];
	Route const &route = *e.flash;
	RouteState &routeState = *e.ram;
	
	TopicBuffer topic = route.getSrcTopic();
	routeState.srcTopicId = subscribeTopic(topic.status(), QOS).topicId;
	
	topic = route.getDstTopic();
	routeState.dstTopicId = registerTopic(topic.command()).topicId;
}

void RoomControl::copy(Route &dstRoute, RouteState &dstRouteState,
	Route const &srcRoute, RouteState const &srcRouteState)
{
	// copy route
	uint8_t *dst = reinterpret_cast<uint8_t *>(&dstRoute);
	uint8_t const *src = reinterpret_cast<uint8_t const *>(&srcRoute);
	array::copy(dst, dst + srcRoute.getFlashSize(), src);

	// copy state
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
		b += "#!"[*value];
		break;
	case Command::ROCKER:
		b += "#+-"[*value];
		break;
	case Command::BINARY:
		b += "01"[*value];
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

void RoomControl::initTimer(int index) {
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
		timerState->topicIds[commandIndex] = registerTopic(topic.command()).topicId;
		
		data += valueSize + command.topicLength;
	}
}

void RoomControl::copy(Timer &dstTimer, TimerState &dstTimerState,
	Timer const &srcTimer, TimerState const &srcTimerState)
{
	// copy timer
	uint8_t *dst = reinterpret_cast<uint8_t *>(&dstTimer);
	uint8_t const *src = reinterpret_cast<uint8_t const *>(&srcTimer);
	array::copy(dst, dst + srcTimer.getFlashSize(), src);

	// copy state
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
		this->selectedTopic = String(this->room[0].flash->name);
		this->topicDepth = 1;
	}

	// only list attributes that have a command topic
	this->onlyCommands = onlyCommands;

	// subscribe to selected topic
	this->selectedTopicId = subscribeTopic(this->selectedTopic.enumeration(), QOS).topicId;

	// request enumeration of rooms, devices or attributes
	publish(this->selectedTopicId, "?", QOS);

	// enter topic selector menu
	push(SELECT_TOPIC);
	this->tempIndex = index;
}
