//#include <iostream>
#include "System.hpp"

// font
#include "tahoma_8pt.hpp"


static const char weekdays[7][4] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
static const char weekdaysShort[7][4] = {"M", "T", "W", "T", "F", "S", "S"};


System::System(Serial::Device device)
	: protocol(device), storage(0, Flash::PAGE_COUNT, events, timers, scenarios, devices)
{
	// initialize event states

	// initialize device states
	for (int i = 0; i < this->devices.size(); ++i) {
		this->deviceStates[i].init(this->devices[i]);
	}
}
	
void System::onPacket(uint8_t packetType, const uint8_t *data, int length, const uint8_t *optionalData, int optionalLength)
{
	//std::cout << "onPacket " << length << " " << optionalLength << std::endl;

	if (packetType == EnOceanProtocol::RADIO_ERP1) {
		if (length >= 7 && data[0] == 0xf6) {
			
			uint32_t nodeId = (data[2] << 24) | (data[3] << 16) | (data[4] << 8) | data[5];
			int state = data[1];
			//std::cout << "nodeId " << std::hex << nodeId << " state " << state << std::dec << std::endl;

			if (state != 0) {
				onEvent(nodeId, state);
			}
		}
		if (optionalLength >= 7) {
			uint32_t destinationId = (optionalData[1] << 24) | (optionalData[2] << 16) | (optionalData[3] << 8) | optionalData[4];
			int dBm = -optionalData[5];
			int security = optionalData[6];
			//std::cout << "destinationId " << std::hex << destinationId << std::dec << " dBm " << dBm << " security " << security << std::endl;
		}
	}
}
	
void System::update() {
	// update motion detector
	if (this->motionDetector.isActivated()) {
		onEvent(0, 0);
	}
	
	// update timers
	{
		// get current time in minutes
		uint32_t time = this->clock.getTime() & ~Clock::SECONDS_MASK;
		if (time != this->lastTime) {
			for (const Timer &timer : this->timers) {
				int weekday = (time & Clock::WEEKDAY_MASK) >> Clock::WEEKDAY_SHIFT;
				if (((timer.time ^ time) & (Clock::MINUTES_MASK | Clock::HOURS_MASK)) == 0
					&& (timer.weekdays & weekday))
				{
					// trigger alarm
					doFirstActionAndToast(timer.actions);
				}
			}
			this->lastTime = time;
		}
	}
	
	// update enocean protocol
	if (this->protocol.update()) {
		uint8_t packetType = this->protocol.getPacketType();
		uint8_t packetLength = this->protocol.getPacketLength();
		uint8_t optionalLength = this->protocol.getOptionalLength();
		uint8_t* data = this->protocol.getData();
		
		onPacket(packetType, data, packetLength, data + packetLength, optionalLength);

		this->protocol.discardFrame();
	}
	
	// update devices
	int outputs = 0;
	{
		uint32_t ticks = this->ticker.getTicks();

		// elapsed time in milliseconds
		uint32_t d = ticks - this->lastTicks;

		for (int index = 0; index < this->devices.size(); ++index) {
			const Device &device = this->devices[index];
			DeviceState &deviceState = this->deviceStates[index];
			
			// update device
			outputs |= deviceState.update(device, d, 0);
		
			// apply conditions
			applyConditions(device, deviceState);
		}
	
		this->lastTicks = ticks;
	}
	//std::cout << outputs << std::endl;

	// update temperature
	this->temperature.update();
	
	// update menu
	updateMenu();
}


void System::updateMenu() {
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

	int delta = this->poti.getDelta();
	bool activated = this->poti.isActivated();

	// clear bitmap
	bitmap.clear();

	// toast
	if (!this->buffer.empty() && this->ticker.getTicks() - this->toastTime < 3000) {
		const char *name = this->buffer;
		int y = 10;
		int len = tahoma_8pt.calcWidth(name, 1);
		bitmap.drawText((bitmap.WIDTH - len) >> 1, y, tahoma_8pt, name, 1);
		return;
	}

	// draw menu
	switch (this->state) {
	case IDLE:
		{
			// get current clock time
			uint32_t time = clock.getTime();
			
			// get index of weekday
			int seconds = time & Clock::SECONDS_MASK;
			int minutes = (time & Clock::MINUTES_MASK) >> Clock::MINUTES_SHIFT;
			int hours = (time & Clock::HOURS_MASK) >> Clock::HOURS_SHIFT;
			int weekday = (time & Clock::WEEKDAY_MASK) >> Clock::WEEKDAY_SHIFT;
			
			// display weekday and clock time
			this->buffer = weekdays[weekday] , "  " , bcd(hours) , ':' , bcd(minutes, 2) , ':' , bcd(seconds, 2);
			bitmap.drawText(20, 10, tahoma_8pt, this->buffer, 1);
			
			// update target temperature
			int targetTemperature = this->targetTemperature = clamp(this->targetTemperature + delta, 10 << 1, 30 << 1);
			this->temperature.setTargetValue(targetTemperature);

			// get current temperature
			int currentTemperature = this->temperature.getCurrentValue();

			this->buffer = decimal(currentTemperature >> 1), (currentTemperature & 1) ? ".5" : ".0" , " oC";
			bitmap.drawText(20, 30, tahoma_8pt, this->buffer, 1);
			this->buffer = decimal(targetTemperature >> 1), (targetTemperature & 1) ? ".5" : ".0" , " oC";
			bitmap.drawText(70, 30, tahoma_8pt, this->buffer, 1);
		
			if (activated) {
				this->activated = true;
				this->stack[0] = {MAIN, 0, 0, 0};
			}
		}
		break;
	case MAIN:
		menu(delta, activated);
		if (entry("Switches"))
			push(EVENTS);
		if (entry("Timers"))
			push(TIMERS);
		if (entry("Scenarios"))
			push(SCENARIOS);
		if (entry("Devices"))
			push(DEVICES);
		if (entry("Exit")) {
			this->activated = false;
			this->state = IDLE;
		}
		break;

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
	}
	this->buffer.clear();
}
	
void System::onEvent(uint32_t input, uint8_t state) {
	switch (this->state) {
	case IDLE:
		// do first action of switch with given input/state
		for (const Event &event : this->events) {
			if (event.input == input && event.value == state) {
				doFirstActionAndToast(event.actions);
			}
		}
		break;
	case EVENTS:
		if (input != INPUT_MOTION_DETECTOR) {
			// check if switch already exists and enter edit menu
			int index = 0;
			for (const Event &event : this->events) {
				if (event.input == input && event.value == state) {
					this->selected = index;
					this->u.event = event;
					this->actions = &u.event.actions;
					push(EDIT_EVENT);
					this->activated = true;
					break;
				}
				++index;
			}
		}
		break;
	case EDIT_EVENT:
	case ADD_EVENT:
		// set input/state of switch if it is currently edited
		if (isEditEntry(0) && this->u.event.input != INPUT_MOTION_DETECTOR) {
			this->u.event.input = input;
			this->u.event.value = state;
		}
		break;
	case EDIT_DEVICE:
	case ADD_DEVICE:
		// set output of device if it is currently edited
		if (isEditEntry(3)) {
			this->u.device.output = input;
		}
		break;
	default:
		;
	}
}


// actions
// -------

Action System::doFirstAction(const Actions &actions) {
	int count = actions.size();
	for (int i = 0; i < count; ++i) {
		Action action = actions.actions[i];
		if (action.state != Action::SCENARIO) {
			// device
			for (int j = 0; j < this->devices.size(); ++j) {
				const Device &device = this->devices[j];
				if (device.id == action.id) {
					DeviceState &deviceState = this->deviceStates[j];
					if (action.state < Action::TRANSITION_START) {
						// set state
						if (!deviceState.isActive(device, action.state)) {
							deviceState.setState(device, action.state);
						
							// return action that was done
							return action;
						}
					} else {
						// do transition
						int transitionIndex = action.state - Action::TRANSITION_START;
						Device::Transition transition = device.getTransitions()[transitionIndex];
						if (deviceState.isActive(device, transition.fromState)) {
							deviceState.setState(device, transition.toState);
							
							// return action that was done
							return action;
						}
					}
				}
			}
		} else {
			// scenario
			for (const Scenario &scenario : this->scenarios) {
				// do all actions of scenario
				if (scenario.id == action.id) {
					if (doAllActions(scenario.actions)) {
						// return action that was done
						return action;
					}
				}
			}
		}
	}
	return {0xff, 0xff};
}

void System::doFirstActionAndToast(const Actions &actions) {
	Action action = doFirstAction(actions);
	setAction(action);
	toast();
}

bool System::doAllActions(const Actions &actions) {
	int count = actions.size();

	// fail if at least one action can't be done
	// test if at least one action has an effect
	bool effect = false;
	for (int i = 0; i < count; ++i) {
		Action action = actions[i];
		if (action.state != Action::SCENARIO) {
			// device
			for (int j = 0; j < this->devices.size(); ++j) {
				const Device &device = this->devices[j];
				if (device.id == action.id) {
					DeviceState &deviceState = this->deviceStates[j];
					if (action.state < Action::TRANSITION_START) {
						// set state
						if (!deviceState.isActive(device, action.state)) {
							// state change has effect
							effect = true;
						}
					} else {
						// do transition
						int transitionIndex = action.state - Action::TRANSITION_START;
						Device::Transition transition = device.getTransitions()[transitionIndex];
						if (deviceState.isActive(device, transition.fromState)) {
							// can do transition
							effect = true;
						} else if (!deviceState.isActive(device, transition.toState)) {
							// not in target state and can't do transition, therefore fail
							return false;
						}
					}
				}
			}
		}
	}

	// fail if no action has an effect
	if (!effect)
		return false;

	// do all actions
	for (int i = 0; i < count; ++i) {
		Action action = actions[i];
		if (action.state != Action::SCENARIO) {
			// device
			for (int j = 0; j < this->devices.size(); ++j) {
				const Device &device = this->devices[j];
				if (device.id == action.id) {
					DeviceState &deviceState = this->deviceStates[j];
					if (action.state < Action::TRANSITION_START) {
						// set state
						deviceState.setState(device, action.state);
					} else {
						// do transition
						int transitionIndex = action.state - Action::TRANSITION_START;
						Device::Transition transition = device.getTransitions()[transitionIndex];
						if (deviceState.isActive(device, transition.fromState))
							deviceState.setState(device, transition.toState);
					}
				}
			}
		}
	}
	
	// indicate success
	return true;
}

// set temp string with action (device/state, device/transition or scenario)
void System::setAction(Action action) {
	this->buffer.clear();
	if (action.state != Action::SCENARIO) {
		// device
		for (const Device &device : this->devices) {
			if (device.id == action.id) {
				this->buffer += device.name, ' ';
				if (action.state < Action::TRANSITION_START) {
					this->buffer += device.getStateName(action.state);
				} else {
					auto transition = device.getTransitions()[action.state - Action::TRANSITION_START];
					this->buffer += device.getStateName(transition.fromState), " -> ",
						device.getStateName(transition.toState);
				}
			}
		}
	} else {
		// scenario
		for (const Scenario &scenario : scenarios) {
			if (scenario.id == action.id)
				this->buffer = scenario.name;
		}
	}
}

void System::listActions() {
	line();

	int actionCount = this->actions->size();
	for (int i = 0; i < actionCount; ++i) {
		setAction((*this->actions)[i]);
		if (entry(this->buffer)) {
			//int scenarioIndex = this->button.scenarios[this->selected];
			push(SELECT_ACTION);
			this->selected = 0;//scenarioIndex;

			// store current action index
			this->actionIndex = i;
		}
	}
	if (actionCount < Actions::ACTION_COUNT && this->storage.hasSpace(0, sizeof(Action)))
		if (entry("Add Action")) {
			// add action
			push(ADD_ACTION);
			this->selected = 0;//scenarioCount == 0 ? 0 : this->button.scenarios[scenarioCount - 1];
			
			// store current action index
			this->actionIndex = actionCount;
		}
	line();
}

// set temp string with condition (device/state)
void System::setConstraint(Action action) {
	this->buffer = '\t';
	for (const Device &device : this->devices) {
		if (device.id == action.id) {
			this->buffer += device.name, ' ';
			this->buffer += device.getStateName(action.state);
		}
	}
}


// scenarios
// ---------
/*
const Scenario *System::findScenario(uint8_t id) {
	for (int i = 0; i < scenarios.size(); ++i) {
		const Scenario &scenario = scenarios[i];
		if (scenario.id == id)
			return &scenario;
	}
	return nullptr;
}
*/
uint8_t System::newScenarioId() {
	return newId(this->scenarios);
}

void System::deleteScenarioId(uint8_t id) {
	deleteScenarioId(this->events, id);
	deleteScenarioId(this->timers, id);
}


// devices
// -------

bool System::isDeviceStateActive(uint8_t id, uint8_t state) {
	int deviceCount = this->devices.size();
	for (int i = 0; i < deviceCount; ++i) {
		const Device &device = this->devices[i];
		if (device.id == id)
			return this->deviceStates[i].isActive(device, state);
	}
	return false;
}

void System::applyConditions(const Device &device, DeviceState &deviceState) {
	int conditionCount = device.actions.size();
	uint8_t state = 0;
	bool active = false;
	if (conditionCount > 0 && device.actions[0].id != device.id) {
		// use default state if conditions don't start with a state of this device
		state = device.getActionStates()[0].state;
		active = true;
	}
	for (int i = 0; i < conditionCount; ++i) {
		Action condition = device.actions[i];
		
		// check if entry is a state of this device or a condition
		if (condition.id == device.id) {
			// next state: check if current condition is active
			if (active) {
				if (deviceState.condition != state) {
					deviceState.setState(device, state);
					deviceState.condition = state;
				}
				return;
			}
			state = condition.state;
			active = true;
		} else {
			// check if all device states of the current condition are active
			active &= isDeviceStateActive(condition.id, condition.state);
		}
	}
	if (active) {
		if (deviceState.condition != state) {
			deviceState.setState(device, state);
			deviceState.condition = state;
		}
	} else {
		deviceState.condition = 0xff;
	}
}

void System::addDeviceStateToString(const Device &device, DeviceState &deviceState) {
	Array<Device::State> states = device.getStates();
	
	if (deviceState.mode & DeviceState::TRANSITION) {
		for (const Device::State &state : states) {
			if (state.state == deviceState.lastState) {
				this->buffer += string(state.name);
				goto found1;
			}
		}
		this->buffer += decimal(deviceState.lastState);
	found1:
		this->buffer += " -> ";
	}
	for (const Device::State &state : states) {
		if (state.state == deviceState.state) {
			this->buffer += string(state.name);
			return;
		}
	}
	this->buffer += decimal(deviceState.state);
/*
	for (int i = 0; i < states.length; ++i) {
		auto &state = states[i];
		if (deviceState.isActive(device, state.state)) {
			if (state.state == Device::VALUE)
				this->buffer += deviceState.state;
			else
				this->buffer += string(state.name);
			break;
		}
	}
*/
}
	
uint8_t System::newDeviceId() {
	return newId(this->devices);
}

void System::deleteDeviceId(uint8_t id) {
	deleteDeviceId(this->events, id);
	deleteDeviceId(this->timers, id);
	deleteDeviceId(this->scenarios, id);
	deleteDeviceId(this->devices, id);
}


// menu
// ----

void System::menu(int delta, bool activated) {
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
	
void System::label(const char* s) {
	int x = 10;
	int y = this->entryY + 2 - this->yOffset;
	this->bitmap.drawText(x, y, tahoma_8pt, s, 1);
	this->entryY += tahoma_8pt.height + 4;
}

void System::line() {
	int x = 10;
	int y = this->entryY + 2 - this->yOffset;
	this->bitmap.fillRectangle(x, y, 108, 1);
	this->entryY += 1 + 4;
}

bool System::entry(const char* s, bool underline, int begin, int end) {
	int x = 10;
	int y = this->entryY + 2 - this->yOffset;//this->entryIndex * lineHeight + 2 - this->yOffset;
	this->bitmap.drawText(x, y, tahoma_8pt, s, 1);
	
	bool selected = this->entryIndex == this->selected;
	if (selected) {
		this->bitmap.drawText(0, y, tahoma_8pt, ">", 0);
		this->selectedY = this->entryY;
	}
	
	if (underline) {
		int start = tahoma_8pt.calcWidth(s, begin, 1);
		int width = tahoma_8pt.calcWidth(s + begin, end - begin, 1) - 1;
		this->bitmap.hLine(x + start, y + tahoma_8pt.height, width);
	}

	++this->entryIndex;
	this->entryY += tahoma_8pt.height + 4;

	return selected && this->activated;
}

bool System::entryWeekdays(const char* s, int weekdays, bool underline, int index) {
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

bool System::isEdit(int editCount) {
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

void System::push(State state) {
	this->stack[this->stackIndex] = {this->state, this->selected, this->selectedY, this->yOffset};
	++this->stackIndex;
	this->stack[this->stackIndex] = {state, 0, 0, 0};
}

void System::pop() {
	--this->stackIndex;
}
