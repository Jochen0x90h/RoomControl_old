//#include <iostream>
#include "System.hpp"

// font
#include "tahoma_8pt.hpp"


static const char weekdays[7][4] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
static const char weekdaysShort[7][4] = {"M", "T", "W", "T", "F", "S", "S"};


System::System(Serial::Device device)
	: protocol(device), storage(16, 16, events, scenarios, devices)
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
				onSwitch(nodeId, state);
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
		onSwitch(0, 0);
	}
	
	// update timers
	{
		// get current time in minutes
		uint32_t time = this->clock.getTime() & ~Clock::SECONDS_MASK;
		if (time != this->lastTime) {
			for (const Event &event : this->events) {
				int weekday = (time & Clock::WEEKDAY_MASK) >> Clock::WEEKDAY_SHIFT;
				if (((event.input ^ time) & (Clock::MINUTES_MASK | Clock::HOURS_MASK)) == 0
					&& (event.value & weekday))
				{
					// trigger alarm
					doFirstActionAndToast(event);
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
			outputs |= deviceState.update(device, d);
		
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

const int EVENT_ACTIONS_INDEX[] = {1, 2};
const int SCENARIO_ACTIONS_INDEX = 1;
const int DEVICE_CONDITIONS_INDEX = 4;

// inputs
const unsigned int INPUT_COUNT = 1;
const unsigned int INPUT_MOTION_DETECTOR = 0;

// outputs
const unsigned int OUTPUT_COUNT = 9;
const unsigned int OUTPUT_HEATING = 8;

void System::updateMenu() {
	int delta = this->poti.getDelta();
	bool activated = this->poti.isActivated();
		
	// do state change due to press of poti switch
	if (activated) {
		switch (this->state) {
		case IDLE:
			push(MAIN);
			break;
		case MAIN:
			if (this->selected == 0) {
				this->u.event.type = Event::Type::SWITCH;
				push(EVENTS);
			} else if (this->selected == 1) {
				this->u.event.type = Event::Type::TIMER;
				push(EVENTS);
			} else if (this->selected == 2) {
				push(SCENARIOS);
			} else if (this->selected == 3) {
				push(DEVICES);
			} else {
				// exit
				pop();
			}
			break;
		case EVENTS:
			{
				int eventCount = getEventCount(this->u.event.type);
				if (this->selected < eventCount) {
					// edit event
					this->u.event = getEvent(this->u.event.type, this->selected);
					push(EDIT_EVENT);
				} else if (this->selected == eventCount && eventCount < EVENT_COUNT) {
					// add event
					this->u.event.input = 0xffffffff;
					this->u.event.value = 0;
					for (int i = 0; i < Event::ACTION_COUNT; ++i)
						this->u.event.actions[i] = {0xff, 0xff};
					push(ADD_EVENT);
				} else {
					// exit
					pop();
				}
			}
			break;
		case EDIT_EVENT:
		case ADD_EVENT:
			{
				bool isSwitch = this->u.event.type == Event::Type::SWITCH;
				bool isTimer = this->u.event.type == Event::Type::TIMER;
				int actionCount = this->u.event.getActionCount();
				int actionsEndIndex = EVENT_ACTIONS_INDEX[int(this->u.event.type)] + actionCount;
				int addActionIndex = actionsEndIndex + (actionCount < Event::ACTION_COUNT ? 0 : -1);

				if (isSwitch && this->selected == 0) {
					// edit switch input
					this->edit ^= 1;
				} else if (isTimer && this->selected == 0) {
					// edit timer time
					this->edit = this->edit < 2 ? this->edit + 1 : 0;
				} else if (isTimer && this->selected == 1) {
					// edit timer weekday
					if (this->edit == 0)
						this->edit = 1;
					else
						this->u.event.value ^= 1 << (this->edit - 1);
				} else if (this->selected < actionsEndIndex) {
					// select action
					//int scenarioIndex = this->button.scenarios[this->selected];
					push(SELECT_ACTION);
					this->selected = 0;//scenarioIndex;
				} else if (this->selected == addActionIndex) {
					// add action
					push(ADD_ACTION);
					this->selected = 0;//scenarioCount == 0 ? 0 : this->button.scenarios[scenarioCount - 1];
				} else if (this->selected == addActionIndex + 1) {
					// save
					this->events.write(getThisIndex(), this->u.event);
					pop();
				} else if (this->selected == addActionIndex + 2) {
					// cancel
					pop();
				} else {
					// delete event
					this->events.erase(getThisIndex());
					pop();
				}
			}
			break;
		case SCENARIOS:
			{
				int scenarioCount = this->scenarios.size();
				if (this->selected < scenarioCount) {
					// edit scenario
					this->u.scenario = scenarios[this->selected];
					push(EDIT_SCENARIO);
				} else if (this->selected == scenarioCount && scenarioCount < SCENARIO_COUNT) {
					// new scenario
					this->device.id = newScenarioId();
					strcpy(this->device.name, "New Scenario");
					for (int i = 0; i < Scenario::ACTION_COUNT; ++i)
						this->u.scenario.actions[i] = {0xff, 0xff};
					push(ADD_SCENARIO);
				} else {
					// exit
					pop();
				}
			}
			break;
		case EDIT_SCENARIO:
		case ADD_SCENARIO:
			{
				const int actionCount = this->u.scenario.getActionCount();
				int addActionIndex = SCENARIO_ACTIONS_INDEX + actionCount + (actionCount < Scenario::ACTION_COUNT ? 0 : -1);
				if (this->selected == 0) {
					// edit scenario name
				} else if (this->selected < 2 + actionCount) {
					// select scenario
					push(SELECT_ACTION);
					this->selected = 0;
				} else if (this->selected == addActionIndex) {
					// add scenario
					push(ADD_ACTION);
					this->selected = 0;
					pop();
				} else if (this->selected == addActionIndex + 1) {
					// save
					this->scenarios.write(getThisIndex(), this->u.scenario);
					pop();
				} else if (this->selected == addActionIndex + 2) {
					// cancel
					pop();
				} else {
					// delete scenario
					deleteScenarioId(this->u.scenario.id);
					this->scenarios.erase(getThisIndex());
					pop();
				}
			}
			break;
		case SELECT_ACTION:
		case ADD_ACTION:
			{
				int index = this->selected;
				
				// convert selected index into action (device/state, device/transition or scenario)
				Action action;
				for (const Device &device : this->devices) {
					Array<Device::State> states = device.getActionStates();
					if (index >= 0 && index < states.length) {
						// device state
						action.id = device.id;
						action.state = states[index].state;
					}
					index -= states.length;
					
					Array<Device::Transition> transitions = device.getTransitions();
					if (index >= 0 && index < transitions.length) {
						// device state transition
						action.id = device.id;
						action.state = Action::TRANSITION_START + index;
					}
					index -= transitions.length;
				}
				if (index >= 0 && index < this->scenarios.size()) {
					// scenario
					action.id = this->scenarios[index].id;
					action.state = Action::SCENARIO;
				}
				index -= this->scenarios.size();

				pop();
				
				int actionsBegin = 0;
				switch (this->state) {
				case EDIT_EVENT:
				case ADD_EVENT:
					actionsBegin = EVENT_ACTIONS_INDEX[int(this->u.event.type)];
					break;
				case EDIT_SCENARIO:
				case ADD_SCENARIO:
					actionsBegin = SCENARIO_ACTIONS_INDEX;
					break;
				default:
					;
				}

				int actionIndex = this->selected - actionsBegin;
				if (index < 0) {
					// add new action to event
					this->u.actions.actions[actionIndex] = action;
				} else if (index == 0) {
					// cancel
				} else {
					// remove action from event
					for (int i = actionIndex; i < Event::ACTION_COUNT - 1; ++i) {
						this->u.actions.actions[i] = this->u.actions.actions[i + 1];
					}
					this->u.actions.actions[Event::ACTION_COUNT - 1] = {0xff, 0xff};
				}
			}
			break;
		case DEVICES:
			{
				int deviceCount = this->devices.size();
				if (this->selected < deviceCount) {
					// edit device
					this->device = devices[this->selected];
					push(EDIT_DEVICE);
				} else if (this->selected == deviceCount) {
					// add device
					this->device.id = newDeviceId();
					this->device.type = Device::Type::SWITCH;
					strcpy(this->device.name, "New Device");
					this->device.speed = 0;
					this->device.output = 0;
					for (int i = 0; i < Device::CONDITION_COUNT; ++i)
						this->device.conditions[i] = {0xff, 0xff};
					push(ADD_DEVICE);
				} else {
					// exit
					pop();
				}
			}
			break;
		case EDIT_DEVICE:
		case ADD_DEVICE:
			{
				int conditionCount = this->device.getConditionsCount();
				int addConstraintIndex = DEVICE_CONDITIONS_INDEX + conditionCount + (conditionCount < Device::CONDITION_COUNT ? 0 : -1);

				if (this->selected == 0) {
					// edit device type
					this->edit ^= 1;
				} else if (this->selected == 1) {
					// edit device name
					
				} else if (this->selected == 2) {
					// edit device state
					
				} else if (this->selected == 3) {
					// edit device binding
					
				} else if (this->selected < 4 + conditionCount) {
					// edit condition
					push(SELECT_CONDITION);
					this->selected = 0;
				} else if (this->selected == addConstraintIndex) {
					// add constriant
					push(ADD_CONDITION);
					this->selected = 0;
				} else if (this->selected == addConstraintIndex + 1) {
					// save
					this->devices.write(getThisIndex(), this->device);
					pop();
				} else if (this->selected == addConstraintIndex + 2) {
					// cancel
					pop();
				} else {
					// delete devic
					deleteDeviceId(this->device.id);
					this->devices.erase(getThisIndex());
					pop();
				}
			}
			break;
		case SELECT_CONDITION:
		case ADD_CONDITION:
			{
				int index = this->selected;
				
				// convert selected index into condition (device/state)
				Device::Condition condition;
				{
					Array<Device::State> states = this->device.getActionStates();
					if (index >= 0 && index < states.length) {
						// device state
						condition.id = this->device.id;
						condition.state = states[index].state;
					}
					index -= states.length;
				}
				for (const Device &device : this->devices) {
					if (device.id != this->device.id) {
						Array<Device::State> states = device.getStates();
						if (index >= 0 && index < states.length) {
							// device state
							condition.id = device.id;
							condition.state = states[index].state;
						}
						index -= states.length;
					}
				}

				pop();

				if (index < 0) {
					// add new condition to device
					this->device.conditions[this->selected - DEVICE_CONDITIONS_INDEX] = condition;
				} else {
					// remove condition from button
					for (int i = this->selected; i < Device::CONDITION_COUNT - 1; ++i) {
						this->device.conditions[i] = this->device.conditions[i + 1];
					}
					this->device.conditions[Device::CONDITION_COUNT - 1] = {0xff, 0xff};
				}
			}
			break;
		}
	}

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
		}
		break;
	case MAIN:
		menu(delta);
		entry("Switches");
		entry("Timers");
		entry("Scenarios");
		entry("Devices");
		entry("Exit");
		break;

	case EVENTS:
		{
			int eventCount = getEventCount(this->u.event.type);
			menu(delta);
			
			if (this->u.event.type == Event::Type::SWITCH) {
				// list switches
				for (const Event &event : this->events) {
					if (event.type == Event::Type::SWITCH) {
						if (event.input == 0xffffffff) {
							this->buffer = "(No Input)";
						} else if (event.input < INPUT_COUNT) {
							this->buffer = "Motion Detector";
						} else {
							this->buffer = hex(event.input), ':', hex(event.value);
						}
						entry(this->buffer);
					}
				}
				if (this->events.size() < EVENT_COUNT)
					entry("Add Switch");
			} else {
				// list timers
				for (const Event &event : this->events) {
					if (event.type == Event::Type::TIMER) {
						int minutes = (event.input & Clock::MINUTES_MASK) >> Clock::MINUTES_SHIFT;
						int hours = (event.input & Clock::HOURS_MASK) >> Clock::HOURS_SHIFT;
						int weekdays = event.value;
						this->buffer = bcd(hours) , ':' , bcd(minutes, 2);
						this->buffer += "    ";
						entryWeekdays(this->buffer, weekdays);
					}
				}
				if (this->events.size() < EVENT_COUNT)
					entry("Add Timer");
			}
			entry("Exit");
		}
		break;
	case EDIT_EVENT:
	case ADD_EVENT:
		{
			int actionCount = this->u.event.getActionCount();

			if (this->edit == 0) {
				updateSelection(delta);
			}
			menu();

			if (this->u.event.type == Event::Type::SWITCH) {
				// edit input
				bool editEntry = isEditEntry();
				if (editEntry && delta != 0) {
					this->u.event.input = clamp(int(this->u.event.input) + delta, -1, INPUT_COUNT - 1);
					this->u.event.value = 0;
				}

				// switch menu entry
				this->buffer = "Input: ";
				int begin = this->buffer.length();
				if (this->u.event.input == 0xffffffff) {
					if (editEntry)
						this->buffer += "Enocean...";
					else
						this->buffer += "(No Input)";
				} else if (this->u.event.input < INPUT_COUNT) {
					this->buffer += "Motion Detector";
				} else {
					this->buffer += hex(this->u.event.input), ':', hex(this->u.event.value);
				}
				int end = this->buffer.length();
				entry(this->buffer, editEntry, begin, end);
			} else {
				// time
				{
					// get time
					int hours = (this->u.event.input & Clock::HOURS_MASK) >> Clock::HOURS_SHIFT;
					int minutes = (this->u.event.input & Clock::MINUTES_MASK) >> Clock::MINUTES_SHIFT;

					// edit time
					bool editEntry = isEditEntry();
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
					// get weekdays
					int weekdays = this->u.event.value;

					// edit weekdays
					bool editEntry = isEditEntry();
					if (editEntry) {
						// edit weekday
						this->edit = max(this->edit + delta, 0);
						if (this->edit > 7)
							this->edit = 0;
					}

					// weekdays menu entry
					this->buffer = "Days: ";
					entryWeekdays(this->buffer, weekdays, editEntry, this->edit - 1);
				}
			}

			// list actions of event and add save/cancel entries
			listActionsSaveCancel();

			if (this->state == EDIT_EVENT) {
				if (this->u.event.type == Event::Type::SWITCH)
					entry("Delete Switch");
				else
					entry("Delete Timer");
			}
		}
		break;

	case SCENARIOS:
		{
			int scenarioCount = scenarios.size();

			menu(delta);
			for (int i = 0; i < scenarioCount; ++i) {
				const Scenario &scenario = scenarios[i];
				this->buffer = scenario.name;
				entry(this->buffer);
			}
			if (scenarioCount < SCENARIO_COUNT)
				entry("Add Scenario");
			entry("Exit");
		}
		break;
	case EDIT_SCENARIO:
	case ADD_SCENARIO:
		{
			int actionCount = this->u.scenario.getActionCount();

			menu(delta);

			this->buffer = "Name: ", this->u.scenario.name;
			entry(this->buffer);

			// list actions of scenario and add save/cancel entries
			listActionsSaveCancel();
			
			if (this->state == EDIT_SCENARIO)
				entry("Delete Scenario");
		}
		break;

	case SELECT_ACTION:
	case ADD_ACTION:
		{
			// get number of selectable actions
			int actionCount = 0;
			for (const Device &device : this->devices) {
				actionCount += device.getActionStates().length;
				actionCount += device.getTransitions().length;
			}
			actionCount += this->scenarios.size();
			
			menu(delta);

			// list all device states and state transitions
			for (const Device &device : this->devices) {
				Array<Device::State> states = device.getActionStates();
				for (int i = 0; i < states.length; ++i) {
					this->buffer = device.name, ' ', string(states[i].name);
					entry(this->buffer);
				}

				Array<Device::Transition> transitions = device.getTransitions();
				for (int i = 0; i < transitions.length; ++i) {
					Device::Transition transition = transitions[i];
					this->buffer = device.name, ' ', device.getStateName(transition.fromState), " -> ",
						device.getStateName(transition.toState);
					entry(this->buffer);
				}
			}
			
			// list all scenarios
			for (const Scenario &scenario : this->scenarios) {
				this->buffer = scenario.name;
				entry(this->buffer);
			}
			line();
	
			entry("Cancel");
			if (this->state == SELECT_ACTION)
				entry("Remove Action");
		}
		break;

	case DEVICES:
		{
			int deviceCount = this->devices.size();
			menu(delta);
			for (int i = 0; i < deviceCount; ++i) {
				const Device &device = this->devices[i];
				DeviceState &deviceState = this->deviceStates[i];
				this->buffer = device.name, ": ";
				addDeviceStateToString(device, deviceState);
				entry(this->buffer);
			}
			entry("Add Device");
			entry("Exit");
		}
		break;
	case EDIT_DEVICE:
	case ADD_DEVICE:
		{
			int conditionCount = this->device.getConditionsCount();
			menu(delta);

			label("Device");
			line();
			
			// device type
			{
				bool editEntry = isEditEntry();
				if (editEntry) {
					int type = clamp(int(this->device.type) + delta, 0, 2);
					this->device.type = Device::Type(type);
				}
				this->buffer = "Type: ";
				int begin = this->buffer.length();
				switch (this->device.type) {
					case Device::Type::SWITCH:
						this->buffer += "Switch";
						break;
					case Device::Type::LIGHT:
						this->buffer += "Light";
						break;
					case Device::Type::HANDLE:
						this->buffer += "Handle";
						break;
					case Device::Type::BLIND:
						this->buffer += "Blind";
						break;
				}
				int end = this->buffer.length();
				entry(this->buffer, editEntry, begin, end);
			}

			// device name
			this->buffer = "Name: ", this->device.name;
			entry(this->buffer);

			// device state
			this->buffer = "State: ";
			addDeviceStateToString(device, this->deviceStates[getThisIndex()]);
			entry(this->buffer);

			// device output (to local relay or enocean device)
			{
				bool editEntry = isEditEntry();
				if (editEntry && delta != 0) {
					this->device.output = clamp(int(this->device.output) + delta, -1, OUTPUT_COUNT - 1);
				}
				this->buffer = "Output: ";
				int begin = this->buffer.length();
				if (this->device.output == 0xffffffff) {
					if (editEntry)
						this->buffer += "Enocean...";
					else
						this->buffer += "(No Output)";
				} else if (this->device.output < OUTPUT_COUNT) {
					this->buffer += "Output ", int(this->device.output);
				} else {
					this->buffer += hex(this->device.output);
				}
				int end = this->buffer.length();
				entry(this->buffer, editEntry, begin, end);
			}

			line();
			
			// list/edit conditions
			uint8_t id = this->device.id;
			if (conditionCount > 0 && this->device.conditions[0].id != id) {
				this->buffer = string(this->device.getStates()[0].name), " when";
				label(this->buffer);
			}
			bool noEffect = false;
			for (int i = 0; i < conditionCount; ++i) {
				Device::Condition condition = this->device.conditions[i];

				if (condition.id == id) {
					// check if constrained state is at last position
					bool last = (i == conditionCount - 1) || this->device.conditions[i + 1].id == id;

					this->buffer.clear();
					String stateName = this->device.getStateName(condition.state);
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
					setConstraint(this->device.conditions[i]);
				}
				entry(this->buffer);
			}
			if (conditionCount < Device::CONDITION_COUNT)
				entry("Add Condition");
			line();
			
			entry("Save");
			entry("Cancel");
			if (this->state == EDIT_DEVICE)
				entry("Delete Device");
		}
		break;
	case SELECT_CONDITION:
	case ADD_CONDITION:
		{
			// get number of selectable actions
			int conditionCount = 0;
			for (const Device &device : this->devices) {
				if (device.id == this->device.id)
					conditionCount += device.getActionStates().length;
				else
					conditionCount += device.getStates().length;
			}
			
			menu(delta);

			// list "own" device states
			{
				Array<Device::State> states = this->device.getActionStates();
				for (int i = 0; i < states.length; ++i) {
					this->buffer = string(states[i].name);
					entry(this->buffer);
				}
			}
			line();
			
			// list device states of other devices
			for (const Device &device : this->devices) {
				if (device.id != this->device.id) {
					Array<Device::State> states = device.getStates();
					for (int i = 0; i < states.length; ++i) {
						this->buffer = device.name, ' ', string(states[i].name);
						entry(this->buffer);
					}
				}
			}
			
			line();
			entry(this->state == SELECT_CONDITION ? "Remove Condition" : "Cancel");
		}
		break;
	}
	this->buffer.clear();
}
	
void System::onSwitch(uint32_t input, uint8_t state) {
	switch (this->state) {
	case IDLE:
		// do first action of switch with given input/state
		for (const Event &event : this->events) {
			if (event.type == Event::Type::SWITCH) {
				if (event.input == input && event.value == state) {
					doFirstActionAndToast(event);
				}
			}
		}
		break;
	case EVENTS:
		if (input != INPUT_MOTION_DETECTOR) {
			// check if switch already exists and enter edit menu
			int index = 0;
			for (const Event &event : this->events) {
				if (event.type == Event::Type::SWITCH) {
					if (event.input == input && event.value == state) {
						this->selected = index;
						this->u.event = event;
						push(EDIT_EVENT);
						break;
					}
					++index;
				}
			}
		}
		break;
	case EDIT_EVENT:
	case ADD_EVENT:
		// set input/state of switch if it is currently edited
		if (this->u.event.type == Event::Type::SWITCH) {
			if (this->selected == 0 && this->edit > 0 && this->u.event.input != INPUT_MOTION_DETECTOR) {
				this->u.event.input = input;
				this->u.event.value = state;
			}
		}
		break;
	default:
		;
	}

}


// actions
// -------

Action System::doFirstAction(const Event &event) {
	const Action *actions = event.actions;
	int count = event.getActionCount();
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
					if (doAllActions(scenario.actions, scenario.getActionCount())) {
						// return action that was done
						return action;
					}
				}
			}
		}
	}
	return {0xff, 0xff};
}

void System::doFirstActionAndToast(const Event &event) {
	Action action = doFirstAction(event);
	setAction(action);
	toast();
}

bool System::doAllActions(const Action *actions, int count) {
	// test if all actions can be done
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

	// fail if no action will have effect
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

void System::listActionsSaveCancel() {
	line();

	int actionCount = this->u.actions.getActionCount();
	for (int i = 0; i < actionCount; ++i) {
		setAction(this->u.actions.actions[i]);
		entry(this->buffer);
	}
	if (actionCount < Scenario::ACTION_COUNT)
		entry("Add Action");
	line();

	entry("Save");
	entry("Cancel");
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

	
// events
// ------

int System::getEventCount(Event::Type type) {
	int count = 0;
	for (const Event &event : this->events) {
		if (event.type == type)
			++count;
	}
	return count;
}

const Event &System::getEvent(Event::Type type, int index) {
	int count = 0;
	for (const Event &event : this->events) {
		if (event.type == type) {
			if (count == index)
				return event;
			++count;
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
	deleteId(this->events, id);
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
	int conditionCount = device.getConditionsCount();
	uint8_t state = 0;
	bool active = false;
	if (conditionCount > 0 && device.conditions[0].id != device.id) {
		// use default state if conditions don't start with a state of this device
		state = device.getActionStates()[0].state;
		active = true;
	}
	for (int i = 0; i < conditionCount; ++i) {
		Device::Condition condition = device.conditions[i];
		
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
}
	
uint8_t System::newDeviceId() {
	return newId(this->devices);
}

void System::deleteDeviceId(uint8_t id) {
	deleteId(this->events, id);
	deleteId(this->scenarios, id);

	// delete device id from device constraints
	for (int index = 0; index < this->devices.size(); ++index) {
		Device device = this->devices[index];
		
		// delete all actions with given id
		int conditionCount = device.getConditionsCount();
		int j = 0;
		for (int i = 0; i < conditionCount; ++i) {
			if (device.conditions[i].id != id) {
				device.conditions[j] = device.conditions[i];
				++j;
			}
		}
		if (j < conditionCount) {
			for (; j < Device::CONDITION_COUNT; ++j) {
				device.conditions[j] = {0xff, 0xff};
			}
			this->devices.write(index, device);
		}
	}
}


// menu
// ----

void System::updateSelection(int delta) {
	// update selected according to delta motion of poti
	this->selected += delta;
	if (this->selected < 0) {
		this->selected = 0;

		// also clear yOffset in case the menu has a non-selectable header
		this->yOffset = 0;
	} else if (this->selected >= this->entryIndex) {
		this->selected = this->entryIndex - 1;
	}
}

void System::menu() {
	const int lineHeight = tahoma_8pt.height + 4;

	// adjust yOffset so that selected entry is visible
	int upper = this->selectedY;//this->selected * lineHeight;
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

void System::entry(const char* s, bool underline, int begin, int end) {
	int x = 10;
	int y = this->entryY + 2 - this->yOffset;//this->entryIndex * lineHeight + 2 - this->yOffset;
	this->bitmap.drawText(x, y, tahoma_8pt, s, 1);
	if (this->entryIndex == this->selected) {
		this->bitmap.drawText(0, y, tahoma_8pt, ">", 0);
		this->lastSelected = this->selected;
		this->selectedY = this->entryY;
	}
	if (underline) {
		int start = tahoma_8pt.calcWidth(s, begin, 1);
		int width = tahoma_8pt.calcWidth(s + begin, end - begin, 1) - 1;
		this->bitmap.hLine(x + start, y + tahoma_8pt.height, width);
	}

	++this->entryIndex;
	this->entryY += tahoma_8pt.height + 4;
}

void System::entryWeekdays(const char* s, int weekdays, bool underline, int index) {
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
	if (this->entryIndex == this->selected) {
		this->bitmap.drawText(0, y, tahoma_8pt, ">", 1);
		this->lastSelected = this->selected;
		this->selectedY = this->entryY;
	}
	if (underline) {
		this->bitmap.hLine(start, y + tahoma_8pt.height, width);
	}

	++this->entryIndex;
	this->entryY += tahoma_8pt.height + 4;
}

void System::push(State state) {
	this->stack[this->stackIndex++] = {this->state, this->selected, this->selectedY, this->yOffset};
	this->state = state;
	this->selected = 0;
	this->selectedY = 0;
	this->lastSelected = -1;
	this->yOffset = 0;

	// set entryIndex to a large value so that we can pre-select an entry and "survive" first call to updateSelection()
	this->entryIndex = 0xffff;
}

void System::pop() {
	--this->stackIndex;
	this->state = this->stack[this->stackIndex].state;
	this->selected = this->stack[this->stackIndex].selected;
	this->selectedY = this->stack[this->stackIndex].selectedY;
	this->yOffset = this->stack[this->stackIndex].yOffset;

	// set entryIndex to a large value so that the value of selected "survives" first call to updateSelection()
	this->entryIndex = 0xffff;
}
