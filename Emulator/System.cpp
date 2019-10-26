//#include <iostream>
#include "System.hpp"

// font
#include "tahoma_8pt.hpp"


static const char weekdays[7][4] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
static const char weekdaysShort[7][4] = {"M", "T", "W", "T", "F", "S", "S"};


System::System(Serial::Device device)
	: EnOceanProtocol(device), storage(16, 16, buttons, timers, scenarios, devices)
{
	for (int i = 0; i < this->devices.size(); ++i) {
		this->deviceStates[i].init(this->devices[i]);
	}
}
	
void System::onPacket(uint8_t packetType, const uint8_t *data, int length, const uint8_t *optionalData, int optionalLength)
{
	//std::cout << "onPacket " << length << " " << optionalLength << std::endl;

	if (packetType == RADIO_ERP1) {
		if (length >= 7 && data[0] == 0xf6) {
			
			uint32_t nodeId = (data[2] << 24) | (data[3] << 16) | (data[4] << 8) | data[5];
			int state = data[1];
			//std::cout << "nodeId " << std::hex << nodeId << " state " << state << std::dec << std::endl;
			
			if (this->state == System::IDLE) {
				// find button
				const Button *button = findButton(nodeId, state);
				if (button != nullptr) {
					Action action = doFirstAction(button->actions, button->getActionCount());
					setAction(action);
					toast();
				}
			} else {
				if (state != 0)
					onButton(nodeId, state);
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
	// update enocean protocol
	EnOceanProtocol::update();
	
	// update timers
	{
		// get current time in minutes
		uint32_t time = this->clock.getTime() & ~Clock::SECONDS_MASK;
		if (time != this->lastTime) {
			for (int i = 0; i < this->timers.size(); ++i) {
				const Timer &timer = this->timers[i];
				
				int weekday = (time & Clock::WEEKDAY_MASK) >> Clock::WEEKDAY_SHIFT;
				if (((timer.time ^ time) & (Clock::MINUTES_MASK | Clock::HOURS_MASK)) == 0
					&& (timer.time & (Timer::MONDAY << weekday)))
				{
					// trigger alarm
					Action action = doFirstAction(timer.actions, timer.getActionCount());
					setAction(action);
					toast();
				}
			}
			this->lastTime = time;
		}
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
		
			// apply constraints
			applyConstraints(device, deviceState);
		}
	
		this->lastTicks = ticks;
	}
	//std::cout << outputs << std::endl;

	// update temperature
	this->temperature.update();
	
	// update system
	updateMenu();
	
	// update display
	this->display.update(this->bitmap);
}
	
void System::updateMenu() {
	int delta = this->poti.getDelta();
	bool press = this->poti.getPress();
		
	const int BUTTON_ACTIONS_INDEX = 0;
	const int TIMER_ACTIONS_INDEX = 2;
	const int SCENARIO_ACTIONS_INDEX = 1;
	const int DEVICE_CONSTRAINTS_INDEX = 4;

	// do state change due to press of poti switch
	if (press) {
		switch (this->state) {
		case IDLE:
			push(MAIN);
			break;
		case MAIN:
			if (this->selected == 0)
				push(BUTTONS);
			else if (this->selected == 1)
				push(TIMERS);
			else if (this->selected == 2)
				push(SCENARIOS);
			else if (this->selected == 3)
				push(DEVICES);
			else {
				// exit
				pop();
			}
			break;
		case BUTTONS:
			{
				int buttonCount = this->buttons.size();
				if (this->selected < buttonCount) {
					// edit button
					this->button = buttons[this->selected];
					push(EDIT_BUTTON);
				} else if (this->selected == buttonCount && buttonCount < BUTTON_COUNT) {
					// add button
					this->button.id = 0;
					this->button.state = 0;
					for (int i = 0; i < Button::ACTION_COUNT; ++i)
						this->button.actions[i] = {0xff, 0xff};
					push(ADD_BUTTON);
				} else {
					// exit
					pop();
				}
			}
			break;
		case EDIT_BUTTON:
		case ADD_BUTTON:
			{
				int actionCount = this->button.getActionCount();
				int addActionIndex = BUTTON_ACTIONS_INDEX + actionCount + (actionCount < Button::ACTION_COUNT ? 0 : -1);
				if (this->selected < actionCount) {
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
					if (this->button.id != 0) {
						this->buttons.write(getThisIndex(), this->button);
						pop();
					} else {
						toast() = "No Button Defined";
					}
				} else if (this->selected == addActionIndex + 2) {
					// cancel
					pop();
				} else {
					// delete button
					this->buttons.erase(getThisIndex());
					pop();
				}
			}
			break;
		case TIMERS:
			{
				int timerCount = timers.size();
				if (this->selected < timerCount) {
					// edit timer
					this->timer = timers[this->selected];
					push(EDIT_TIMER);
				} else if (this->selected == timerCount && timerCount < TIMER_COUNT) {
					// add timer
					this->timer.time = 0;
					for (int i = 0; i < Timer::ACTION_COUNT; ++i)
						this->timer.actions[i] = {0xff, 0xff};
					push(ADD_TIMER);
				} else {
					// exit
					pop();
				}
			}
			break;
		case EDIT_TIMER:
		case ADD_TIMER:
			{
				int actionCount = this->timer.getActionCount();
				int addActionIndex = TIMER_ACTIONS_INDEX + actionCount + (actionCount < Button::ACTION_COUNT ? 0 : -1);
				if (this->selected == 0) {
					// edit time
					this->edit = this->edit < 2 ? this->edit + 1 : 0;
				} else if (this->selected == 1) {
					// edit weekday
					if (this->edit == 0) {
						this->edit = 1;
					} else {
						this->timer.time ^= 1 << (Timer::WEEKDAYS_SHIFT + this->edit - 1);
					}
				} else if (this->selected < 2 + actionCount) {
					// select action
					//int scenarioIndex = this->timer.scenarios[this->selected - 2];
					push(SELECT_ACTION);
					this->selected = 0;//scenarioIndex;
				} else if (this->selected == addActionIndex) {
					// add action
					push(ADD_ACTION);
					this->selected = 0;//scenarioCount == 0 ? 0 : this->timer.scenarios[scenarioCount - 1];
				} else if (this->selected == addActionIndex + 1) {
					// save
					this->timers.write(getThisIndex(), this->timer);
					pop();
				} else if (this->selected == addActionIndex + 2) {
					// cancel
					pop();
				} else {
					// delete timer
					this->timers.erase(getThisIndex());
					pop();
				}
			}
			break;
		case SCENARIOS:
			{
				int scenarioCount = this->scenarios.size();
				if (this->selected < scenarioCount) {
					// edit scenario
					this->scenario = scenarios[this->selected];
					push(EDIT_SCENARIO);
				} else if (this->selected == scenarioCount && scenarioCount < SCENARIO_COUNT) {
					// new scenario
					this->device.id = newScenarioId();
					strcpy(this->device.name, "New Scenario");
					for (int i = 0; i < Scenario::ACTION_COUNT; ++i)
						this->scenario.actions[i] = {0xff, 0xff};
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
				const int actionCount = this->scenario.getActionCount();
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
					this->scenarios.write(getThisIndex(), this->scenario);
					pop();
				} else if (this->selected == addActionIndex + 2) {
					// cancel
					pop();
				} else {
					// delete timer
					deleteScenarioId(this->scenario.id);
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

				switch (this->state) {
				case EDIT_BUTTON:
				case ADD_BUTTON:
					if (index < 0) {
						// add new action to button
						this->button.actions[this->selected - BUTTON_ACTIONS_INDEX] = action;
					} else {
						// remove action from button
						for (int i = this->selected; i < Button::ACTION_COUNT - 1; ++i) {
							this->button.actions[i] = this->button.actions[i + 1];
						}
						this->button.actions[Button::ACTION_COUNT - 1] = {0xff, 0xff};
					}
					break;
				case EDIT_TIMER:
				case ADD_TIMER:
					if (index < 0) {
						// add new action to timer
						this->timer.actions[this->selected - TIMER_ACTIONS_INDEX] = action;
					} else {
						// remove action from timer
						for (int i = this->selected; i < Timer::ACTION_COUNT - 1; ++i) {
							this->timer.actions[i] = this->timer.actions[i + 1];
						}
						this->timer.actions[Timer::ACTION_COUNT - 1] = {0xff, 0xff};
					}
					break;
				case EDIT_SCENARIO:
				case ADD_SCENARIO:
					if (index < 0) {
						// add new action to scenario
						this->scenario.actions[this->selected - SCENARIO_ACTIONS_INDEX] = action;
					} else {
						// remove action from scenario
						for (int i = this->selected; i < Scenario::ACTION_COUNT - 1; ++i) {
							this->scenario.actions[i] = this->scenario.actions[i + 1];
						}
						this->scenario.actions[Scenario::ACTION_COUNT - 1] = {0xff, 0xff};
					}
					break;
				default:
					// should not happen
					;
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
					this->device.binding = 0;
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
				int addConstraintIndex = DEVICE_CONSTRAINTS_INDEX + conditionCount + (conditionCount < Device::CONDITION_COUNT ? 0 : -1);

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
					push(SELECT_CONSTRAINT);
					this->selected = 0;
				} else if (this->selected == addConstraintIndex) {
					// add constriant
					push(ADD_CONSTRAINT);
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
		case SELECT_CONSTRAINT:
		case ADD_CONSTRAINT:
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
					this->device.conditions[this->selected - DEVICE_CONSTRAINTS_INDEX] = condition;
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
	if (!this->string.empty() && this->ticker.getTicks() - this->toastTime < 3000) {
		const char *name = this->string;
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
			this->string = weekdays[weekday] , "  " , bcd(hours) , ':' , bcd(minutes, 2) , ':' , bcd(seconds, 2);
			bitmap.drawText(20, 10, tahoma_8pt, this->string, 1);
			
			// update target temperature
			int targetTemperature = this->targetTemperature = clamp(this->targetTemperature + delta, 10 << 1, 30 << 1);
			this->temperature.setTargetValue(targetTemperature);

			// get current temperature
			int currentTemperature = this->temperature.getCurrentValue();

			this->string = decimal(currentTemperature >> 1), (currentTemperature & 1) ? ".5" : ".0" , " oC";
			bitmap.drawText(20, 30, tahoma_8pt, this->string, 1);
			this->string = decimal(targetTemperature >> 1), (targetTemperature & 1) ? ".5" : ".0" , " oC";
			bitmap.drawText(70, 30, tahoma_8pt, this->string, 1);
		}
		break;
	case MAIN:
		{
			menu(delta, 5);
			entry("Buttons");
			entry("Timers");
			entry("Scenarios");
			entry("Devices");
			entry("Exit");
		}
		break;

	case BUTTONS:
		{
			int buttonCount = buttons.size();
			menu(delta, buttonCount + 2);
			
			// list buttons
			for (int i = 0; i < buttonCount; ++i) {
				const Button &button = buttons[i];
				
				this->string = hex(button.id), ':', hex(button.state);
				entry(this->string);
			}
			if (buttonCount < BUTTON_COUNT)
				entry("Add Button");
			entry("Exit");
		}
		break;
	case EDIT_BUTTON:
	case ADD_BUTTON:
		{
			int actionCount = this->button.getActionCount();
			menu(delta, actionCount
				+ (actionCount < Button::ACTION_COUNT ? 1 : 0)
				+ 2
				+ (this->state == EDIT_BUTTON ? 1 : 0));
			
			this->string = "Button: ", hex(this->button.id), ':', hex(this->button.state);
			label(this->string);
			line();

			// list actions of button
			for (int i = 0; i < actionCount; ++i) {
				setAction(this->button.actions[i]);
				entry(this->string);
			}
			if (actionCount < Button::ACTION_COUNT)
				entry("Add Action");
			line();
			
			entry("Save");
			entry("Cancel");
			if (this->state == EDIT_BUTTON)
				entry("Delete Button");
		}
		break;

	case TIMERS:
		{
			int timerCount = timers.size();
			menu(delta, timerCount + 2);

			// list timers
			for (int i = 0; i < timerCount; ++i) {
				const Timer &timer = timers[i];
				int minutes = (timer.time & Clock::MINUTES_MASK) >> Clock::MINUTES_SHIFT;
				int hours = (timer.time & Clock::HOURS_MASK) >> Clock::HOURS_SHIFT;
				int weekdays = (timer.time & Timer::WEEKDAYS_MASK) >> Timer::WEEKDAYS_SHIFT;
				this->string = bcd(hours) , ':' , bcd(minutes, 2);
				this->string += "    ";
				entryWeekdays(this->string, weekdays);
			}
			if (timerCount < TIMER_COUNT)
				entry("Add Timer");
			entry("Exit");
		}
		break;
	case EDIT_TIMER:
	case ADD_TIMER:
		{
			int actionCount = this->timer.getActionCount();
			menu(delta, 2 + actionCount
				+ (actionCount < Timer::ACTION_COUNT ? 1 : 0)
				+ 2
				+ (this->state == EDIT_TIMER ? 1 : 0));
			
			// get time
			int hours = (this->timer.time & Clock::HOURS_MASK) >> Clock::HOURS_SHIFT;
			int minutes = (this->timer.time & Clock::MINUTES_MASK) >> Clock::MINUTES_SHIFT;
			
			// edit time
			if (isSelected() && this->edit > 0) {
				if (this->edit == 1)
					hours = Clock::addTime(hours, delta, 0x23);
				else
					minutes = Clock::addTime(minutes, delta, 0x59);
				
				// write back
				this->timer.time = (this->timer.time & ~(Clock::HOURS_MASK | Clock::MINUTES_MASK))
					| (hours << Clock::HOURS_SHIFT) | (minutes << Clock::MINUTES_SHIFT);
			}
			
			// time menu entry
			this->string = "Time: ";
			int begin1 = this->string.length();
			this->string += bcd(hours);
			int end1 = this->string.length();
			this->string += ':';
			int begin2 = this->string.length();
			this->string += bcd(minutes, 2);
			int end2 = this->string.length();
			entry(this->string, this->edit == 1 ? begin1 : begin2, this->edit == 1 ? end1 : end2);
			
			// weekdays menu entry
			int weekdays = this->timer.time >> Timer::WEEKDAYS_SHIFT;
			if (isSelected() && this->edit > 0) {
				// edit weekday
				this->edit = max(this->edit + delta, 0);
				if (this->edit > 7)
					this->edit = 0;
			}
			this->string = "Days: ";
			entryWeekdays(this->string, weekdays, this->edit - 1);
			line();

			// list actions of button
			for (int i = 0; i < actionCount; ++i) {
				setAction(this->timer.actions[i]);
				entry(this->string);
			}
			if (actionCount < Timer::ACTION_COUNT)
				entry("Add Action");
			line();
			
			entry("Save");
			entry("Cancel");
			if (this->state == EDIT_TIMER)
				entry("Delete Timer");
		}
		break;

	case SCENARIOS:
		{
			int scenarioCount = scenarios.size();

			menu(delta, scenarioCount + 2);
			for (int i = 0; i < scenarioCount; ++i) {
				const Scenario &scenario = scenarios[i];
				this->string = scenario.name;
				entry(this->string);
			}
			if (scenarioCount < SCENARIO_COUNT)
				entry("Add Scenario");
			entry("Exit");
		}
		break;
	case EDIT_SCENARIO:
	case ADD_SCENARIO:
		{
			int actionCount = this->scenario.getActionCount();

			menu(delta, 1 + actionCount
				+ (actionCount < Timer::ACTION_COUNT ? 1 : 0)
				+ 2
				+ (this->state == EDIT_SCENARIO ? 1 : 0));

			this->string = "Scenario: ", this->scenario.name;
			entry(this->string);
			line();

			// list actions of scenario
			for (int i = 0; i < actionCount; ++i) {
				setAction(this->scenario.actions[i]);
				entry(this->string);
			}
			if (actionCount < Scenario::ACTION_COUNT)
				entry("Add Action");
			line();

			entry("Save");
			entry("Cancel");
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
			
			menu(delta, actionCount + 1);

			// list all device states and state transitions
			for (const Device &device : this->devices) {
				Array<Device::State> states = device.getActionStates();
				for (int i = 0; i < states.length; ++i) {
					this->string = device.name, ' ', states[i].name;
					entry(this->string);
				}

				Array<Device::Transition> transitions = device.getTransitions();
				for (int i = 0; i < transitions.length; ++i) {
					Device::Transition transition = transitions[i];
					this->string = device.name, ' ', device.getStateName(transition.fromState), " -> ",
						device.getStateName(transition.toState);
					entry(this->string);
				}
			}
			
			// list all scenarios
			for (const Scenario &scenario : this->scenarios) {
				this->string = scenario.name;
				entry(this->string);
			}
			
			line();
			entry(this->state == SELECT_ACTION ? "Remove Action" : "Cancel");
		}
		break;

	case DEVICES:
		{
			int deviceCount = this->devices.size();
			menu(delta, deviceCount + 2);
			for (int i = 0; i < deviceCount; ++i) {
				const Device &device = this->devices[i];
				DeviceState &deviceState = this->deviceStates[i];
				this->string = device.name, ": ";
				addDeviceStateToString(device, deviceState);
				entry(this->string);
			}
			entry("Add Device");
			entry("Exit");
		}
		break;
	case EDIT_DEVICE:
	case ADD_DEVICE:
		{
			int conditionCount = this->device.getConditionsCount();
			menu(delta, 4 + conditionCount
				+ (conditionCount < Device::CONDITION_COUNT ? 1 : 0)
				+ 2
				+ (this->state == EDIT_DEVICE ? 1 : 0));

			label("Device");
			line();
			
			// edit device type
			if (isSelected() && this->edit == 1) {
				int type = clamp(int(device.type) + delta, 0, 2);
				device.type = Device::Type(type);
			}
			this->string = "Type: ";
			int s = this->string.length();
			switch (this->device.type) {
			case Device::Type::SWITCH:
				this->string += "Switch";
				break;
			case Device::Type::LIGHT:
				this->string += "Light";
				break;
			case Device::Type::HANDLE:
				this->string += "Handle";
				break;
			case Device::Type::BLIND:
				this->string += "Blind";
				break;
			}
			int e = this->string.length();
			entry(this->string, s, e);
			
			// edit device name
			this->string = "Name: ", this->device.name;
			entry(this->string);

			// edit device state
			this->string = "State: ";
			addDeviceStateToString(device, this->deviceStates[getThisIndex()]);
			entry(this->string);

			// edit device binding (to local output or enocean device)
			entry("Binding: ");

			line();
			
			// list/edit constraints
			uint8_t id = this->device.id;
			if (conditionCount > 0 && this->device.conditions[0].id != id) {
				this->string = this->device.getStates()[0].name, " when";
				label(this->string);
			}
			bool noEffect = false;
			for (int i = 0; i < conditionCount; ++i) {
				Device::Condition condition = this->device.conditions[i];

				if (condition.id == id) {
					// check if constrained state is at last position
					bool last = (i == conditionCount - 1) || this->device.conditions[i + 1].id == id;
					noEffect |= last && i == 0;
					
					this->string.clear();
					if (!noEffect) {
						if (last)
							this->string = "Else ";
						this->string += this->device.getStateName(condition.state);
						if (!last)
							this->string += " When";
					} else {
						this->string += '(', this->device.getStateName(condition.state), ')';
					}
					
					noEffect |= last;
				} else {
					setConstraint(this->device.conditions[i]);
				}
				entry(this->string);
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
	case SELECT_CONSTRAINT:
	case ADD_CONSTRAINT:
		{
			// get number of selectable actions
			int conditionCount = 0;
			for (const Device &device : this->devices) {
				if (device.id == this->device.id)
					conditionCount += device.getActionStates().length;
				else
					conditionCount += device.getStates().length;
			}
			
			menu(delta, conditionCount + 1);

			// list "own" device states
			{
				Array<Device::State> states = this->device.getActionStates();
				for (int i = 0; i < states.length; ++i) {
					this->string = states[i].name;
					entry(this->string);
				}
			}
			line();
			
			// list device states of other devices
			for (const Device &device : this->devices) {
				if (device.id != this->device.id) {
					Array<Device::State> states = device.getStates();
					for (int i = 0; i < states.length; ++i) {
						this->string = device.name, ' ', states[i].name;
						entry(this->string);
					}
				}
			}
			
			line();
			entry(this->state == SELECT_CONSTRAINT ? "Remove Condition" : "Cancel");
		}
		break;
	}
	this->string.clear();
}
	
void System::onButton(uint32_t id, uint8_t state) {
	switch (this->state) {
	case BUTTONS:
		// select button from list
		{
			int buttonCount = buttons.size();
			for (int i = 0; i < buttonCount; ++i) {
				const Button &button = buttons[i];
				if (button.id == id && button.state == state) {
					this->selected = i;
					this->button = buttons[i];
					push(EDIT_BUTTON);
					break;
				}
			}
		}
		break;
	case ADD_BUTTON:
		// set id/state of button
		{
			// check if button already exists to prevent duplicates
			for (int i = 0; i < buttons.size(); ++i) {
				const Button &button = buttons[i];
				if (button.id == id && button.state == state) {
					// display message
					toast() = hex(id), ':', decimal(state), " not new";
					return;
				}
			}

			this->button.id = id;
			this->button.state = state;
		}
		break;
	default:
		;
	}
}


// actions
// -------

Action System::doFirstAction(const Action *actions, int count) {
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
	this->string.clear();
	if (action.state != Action::SCENARIO) {
		// device
		for (const Device &device : this->devices) {
			if (device.id == action.id) {
				this->string += device.name, ' ';
				if (action.state < Action::TRANSITION_START) {
					this->string += device.getStateName(action.state);
				} else {
					auto transition = device.getTransitions()[action.state - Action::TRANSITION_START];
					this->string += device.getStateName(transition.fromState), " -> ",
						device.getStateName(transition.toState);
				}
			}
		}
	} else {
		// scenario
		for (const Scenario &scenario : scenarios) {
			if (scenario.id == action.id)
				this->string = scenario.name;
		}
	}
}

// set temp string with condition (device/state)
void System::setConstraint(Action action) {
	this->string = '\t';
	for (const Device &device : this->devices) {
		if (device.id == action.id) {
			this->string += device.name, ' ';
			this->string += device.getStateName(action.state);
		}
	}
}

	
// buttons
// -------

const Button *System::findButton(uint32_t id, uint8_t state) {
	for (int i = 0; i < buttons.size(); ++i) {
		const Button &button = buttons[i];
		if (button.id == id && button.state == state)
			return &button;
	}
	return nullptr;
}


// scenarios
// ---------

const Scenario *System::findScenario(uint8_t id) {
	for (int i = 0; i < scenarios.size(); ++i) {
		const Scenario &scenario = scenarios[i];
		if (scenario.id == id)
			return &scenario;
	}
	return nullptr;
}

uint8_t System::newScenarioId() {
	return newId(this->scenarios);
}

void System::deleteScenarioId(uint8_t id) {
	deleteId(this->buttons, id);
	deleteId(this->timers, id);
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

void System::applyConstraints(const Device &device, DeviceState &deviceState) {
	int conditionCount = device.getConditionsCount();
	uint8_t state = 0;
	bool active = false;
	if (conditionCount > 0 && device.conditions[0].id != device.id) {
		// use default state if constraints don't start with a constrained state
		state = device.getActionStates()[0].state;
		active = true;
	}
	for (int i = 0; i < conditionCount; ++i) {
		Device::Condition condition = device.conditions[i];
		
		if (condition.id == device.id) {
			// next constrained state: check if current state is active
			if (active) {
				if (deviceState.condition != i) {
					deviceState.setState(device, state);
					deviceState.condition = i;
				}
				return;
			}
			state = condition.state;
			active = true;
		} else {
			// check if all referenced states are active
			active &= isDeviceStateActive(condition.id, condition.state);
		}
	}
	if (active) {
		if (deviceState.condition != conditionCount) {
			deviceState.setState(device, state);
			deviceState.condition = conditionCount;
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
				this->string += deviceState.state;
			else
				this->string += state.name;
			break;
		}
	}
}
	
uint8_t System::newDeviceId() {
	return newId(this->devices);
}

void System::deleteDeviceId(uint8_t id) {
	deleteId(this->buttons, id);
	deleteId(this->timers, id);
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

void System::menu(int delta, int entryCount) {
	const int lineHeight = tahoma_8pt.height + 4;

	// update selected according to delta motion of poti
	if (this->edit == 0) {
		this->selected += delta;
		if (this->selected < 0) {
			this->selected = 0;
			
			// also clear yOffset in case the menu has a non-selectable header
			this->yOffset = 0;
		} else if (this->selected >= entryCount) {
			this->selected = entryCount - 1;
		}
	}
	
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

void System::entry(const char* s, int editBegin, int editEnd) {
	int x = 10;
	int y = this->entryY + 2 - this->yOffset;//this->entryIndex * lineHeight + 2 - this->yOffset;
	this->bitmap.drawText(x, y, tahoma_8pt, s, 1);
	if (this->entryIndex == this->selected) {
		this->bitmap.drawText(0, y, tahoma_8pt, ">", 0);
		if (this->edit > 0) {
			int start = tahoma_8pt.calcWidth(s, editBegin, 1);
			int width = tahoma_8pt.calcWidth(s + editBegin, editEnd - editBegin, 1);
			this->bitmap.fillRectangle(x + start, y + tahoma_8pt.height, width, 1);
		}
		this->lastSelected = this->selected;
		this->selectedY = this->entryY;
	}

	++this->entryIndex;
	this->entryY += tahoma_8pt.height + 4;
}

void System::entryWeekdays(const char* s, int weekdays, int index) {
	int x = 10;
	int y = this->entryY + 2 - this->yOffset;
	int x2 = this->bitmap.drawText(x, y, tahoma_8pt, s, 1) + 1;
	int start;
	int width;
	for (int i = 0; i < 7; ++i) {
		bool enabled = weekdays & (1 << i);
		int x3 = this->bitmap.drawText(x2 + 1, y, tahoma_8pt, weekdaysShort[i], 1);
		if (enabled)
			this->bitmap.fillRectangle(x2, y, x3 - x2, tahoma_8pt.height - 1, Mode::FLIP);
		if (i == index) {
			start = x2;
			width = x3 - x2;
		}
		x2 = x3 + 4;
	}
	if (this->entryIndex == this->selected) {
		this->bitmap.drawText(0, y, tahoma_8pt, ">", 1);
		if (this->edit > 0)
			this->bitmap.fillRectangle(start, y + tahoma_8pt.height, width, 1);
		this->lastSelected = this->selected;
		this->selectedY = this->entryY;
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
}

void System::pop() {
	--this->stackIndex;
	this->state = this->stack[this->stackIndex].state;
	this->selected = this->stack[this->stackIndex].selected;
	this->selectedY = this->stack[this->stackIndex].selectedY;
	this->yOffset = this->stack[this->stackIndex].yOffset;
}
