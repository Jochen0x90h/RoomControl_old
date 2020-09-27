//#include <iostream>
#include "RoomControl.hpp"

// font
#include "tahoma_8pt.hpp"


static const char weekdays[7][4] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
static const char weekdaysShort[7][4] = {"M", "T", "W", "T", "F", "S", "S"};


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

	// register a topic name to obtain a topic id
	//registerTopic("foo").topicId;
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
	std::string s((char const*)data, length);
	std::cout << "onPublished " << topicId << " data " << s << " retain " << retain << " qos " << int(qos) << std::endl;

	// check if this is a command
	if (retain || length == 0)
		return;
		
	// iterate over local devices
	for (LocalDeviceElement e : this->localDevices) {
		switch (e.flash->device.type) {
		case Lin::Device::SWITCH2:
			{
				Switch2DeviceState *state = reinterpret_cast<Switch2DeviceState *>(e.ram);

				if (topicId == state->x || topicId == state->y) {
					// get command
					bool release = data[0] == '#';
					bool on = data[0] == '1' || data[0] == '+';
					bool off = data[0] == '0' || data[0] == '-';
					
					uint8_t relays = state->relays;
					if (topicId == state->x) {
						if (on || off)
							relays = (relays & ~0x01) | (on ? 0x01 : 0);
					} else if (topicId == state->y) {
						if (on || off)
							relays = (relays & ~0x04) | (on ? 0x04 : 0);
					}
					if (relays != state->relays) {
						state->relays = relays;
						linSend(e.flash->device.id, &relays, 1);
					}
				}
			}
			break;
		}
	}
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
			if (e.flash->device.id == linDevice.id) {
				found = true;
				break;
			}
		}
		
		// add new device if not found
		if (!found) {
			// set device id and type
			this->temp.localDevice.device = linDevice;

			// initialize name with hex id
			this->buffer.clear();
			this->buffer << hex(linDevice.id);
			this->temp.localDevice.setName(this->buffer);
			
			// add new device
			int index = this->localDevices.size();
			this->localDevices.write(index, &this->temp.switch2Device);
		}
	}
}

void RoomControl::onLinReceived(uint32_t deviceId, uint8_t const *data, int length) {
	for (LocalDeviceElement e : this->localDevices) {
		if (e.flash->device.id == deviceId) {
			switch (e.flash->device.type) {
			case Lin::Device::SWITCH2:
				{
					uint8_t const commands[] = "#+-";
					Switch2DeviceState *state = reinterpret_cast<Switch2DeviceState *>(e.ram);
					
					publish(state->a, &commands[data[0] & 3], 1, 1);
					publish(state->b, &commands[(data[0] >> 2) & 3], 1, 1);
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

}


// Clock
// -----

void RoomControl::onSecondElapsed() {
	updateMenu(0, false);
	setDisplay(this->bitmap);
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
	if (this->activated) {
		this->state = this->stack[this->stackIndex].state;
		this->selected = this->stack[this->stackIndex].selected;
		this->selectedY = this->stack[this->stackIndex].selectedY;
		this->yOffset = this->stack[this->stackIndex].yOffset;

		this->activated = false;

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
				<< decimal(time.getHours()) << ':'
				<< decimal(time.getMinutes(), 2) << ':'
				<< decimal(time.getSeconds(), 2);
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
				this->activated = true;
				this->stack[0] = {MAIN, 0, 0, 0};
			}
		}
		break;
	case MAIN:
		menu(delta, activated);

		if (entry("Local Devices"))
			push(LOCAL_DEVICES);

		/*if (entry("Switches"))
			push(EVENTS);
		if (entry("Timers"))
			push(TIMERS);
		if (entry("Scenarios"))
			push(SCENARIOS);
		if (entry("Devices"))
			push(DEVICES);*/
		if (entry("Exit")) {
			this->activated = false;
			this->state = IDLE;
		}
		break;

	case LOCAL_DEVICES:
		{
			// list devices
			menu(delta, activated);
			int deviceCount = this->localDevices.size();
			for (int i = 0; i < deviceCount; ++i) {
				LocalDeviceElement e = this->localDevices[i];
				/*DeviceState &deviceState = this->deviceStates[i];
				this->buffer = device.name, ": ";
				addDeviceStateToString(device, deviceState);
				if (entry(this->buffer)) {
					// edit device
					this->u.device = devices[this->selected];
					this->actions = &this->u.device.actions;
					push(EDIT_DEVICE);
				}*/
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
	// update selected according to delta motion of poti
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

bool RoomControl::entryWeekdays(const char* s, int weekdays, bool underline, int index) {
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

bool RoomControl::isEdit(int editCount) {
	if (this->selected == this->entryIndex) {
		// cycle edit mode if activated
		if (this->activated) {
			this->edit = this->edit < editCount ? this->edit + 1 : 0;
			this->activated = false;
		}
		return this->edit > 0;
	}
	return false;
}

void RoomControl::push(State state) {
	this->stack[this->stackIndex] = {this->state, this->selected, this->selectedY, this->yOffset};
	++this->stackIndex;
	this->stack[this->stackIndex] = {state, 0, 0, 0};
}

void RoomControl::pop() {
	--this->stackIndex;
}


// Devices
// -------

int RoomControl::LocalDevice::flashSize() const {
	switch (this->device.type) {
	case Lin::Device::SWITCH2:
		{
			Switch2Device const *device = reinterpret_cast<Switch2Device const *>(this);
			
			int length = size(device->name);
			for (int i = 0; i < size(device->name); ++i) {
				if (device->name[i] == 0) {
					length = i + 1;
					break;
				}
			}
			
			return offsetof(Switch2Device, name) + length;
		}
	}
}

int RoomControl::LocalDevice::ramSize() const {
	switch (this->device.type) {
	case Lin::Device::SWITCH2:
		return sizeof(Switch2DeviceState);
	}
}

String RoomControl::LocalDevice::getName() const {
	switch (this->device.type) {
	case Lin::Device::SWITCH2:
		return String(reinterpret_cast<Switch2Device const *>(this)->name);
	}
}

void RoomControl::LocalDevice::setName(String name) {
	switch (this->device.type) {
	case Lin::Device::SWITCH2:
		assign(reinterpret_cast<Switch2Device *>(this)->name, name);
		break;
	}
}

// todo don't register all topics in one go, message buffer may overflow
void RoomControl::subscribeDevices() {
	// prifix of mqtt topic
	this->buffer.clear();
	this->buffer << "huasi/";
	int prefixLength = this->buffer.length();
	
	// register topics for devices
	for (LocalDeviceElement e : this->localDevices) {
		// append device name to topic
		this->buffer.setLength(prefixLength);
		this->buffer << e.flash->getName() << '/';
		int deviceLength = this->buffer.length();

		switch (e.flash->device.type) {
		case Lin::Device::SWITCH2:
			{
				Switch2DeviceState *state = reinterpret_cast<Switch2DeviceState *>(e.ram);
				
				this->buffer.setLength(deviceLength);
				this->buffer << "a";
				state->a = registerTopic(this->buffer).topicId;

				this->buffer.setLength(deviceLength);
				this->buffer << "b";
				state->b = registerTopic(this->buffer).topicId;
			
				this->buffer.setLength(deviceLength);
				this->buffer << "x";
				state->x = subscribeTopic(this->buffer).topicId;

				this->buffer.setLength(deviceLength);
				this->buffer << "y";
				state->y = subscribeTopic(this->buffer).topicId;
			}
			break;
		}
	}

	this->buffer.clear();
}
