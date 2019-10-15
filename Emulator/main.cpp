
#include <iostream>
#include <fstream>
#include <vector>
#include <thread>
#include <iterator>

#include "Flash.hpp"
#include "Storage.hpp"
#include "Clock.hpp"
#include "Actor.hpp"
#include "Scenario.hpp"
#include "Button.hpp"
#include "Timer.hpp"
#include "String.hpp"
#include "Ticker.hpp"
#include "Serial.hpp"
#include "EnOceanProtocol.hpp"
#include "ActorValues.hpp"

#include "gui/Display.hpp"
#include "gui/Poti.hpp"
#include "gui/Temperature.hpp"
#include "gui/Light.hpp"
#include "gui/Blind.hpp"

#include "glad/glad.h"
#include <GLFW/glfw3.h> // http://www.glfw.org/docs/latest/quick_guide.html


// fonts
#include "tahoma_8pt.hpp"


static const int SCENARIO_COUNT = 32;
static const int BUTTON_COUNT = 32;
static const int TIMER_COUNT = 32;

static const char weekdays[7][4] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
static const char weekdaysShort[7][4] = {"M", "T", "W", "T", "F", "S", "S"};



class System : public EnOceanProtocol {
public:

	System(Serial::Device device)
		: EnOceanProtocol(device), storage(16, 16, actors, scenarios, buttons, timers)
	{
	}
	
	void onPacket(uint8_t packetType, const uint8_t *data, int length, const uint8_t *optionalData, int optionalLength)
		override
	{
		//std::cout << "onPacket " << length << " " << optionalLength << std::endl;

		if (packetType == RADIO_ERP1) {
			if (length >= 7 && data[0] == 0xf6) {
				
				uint32_t nodeId = (data[2] << 24) | (data[3] << 16) | (data[4] << 8) | data[5];
				int state = data[1];
				std::cout << "nodeId " << std::hex << nodeId << " state " << state << std::dec << std::endl;
				
				if (this->state == System::IDLE || this->state == System::TOAST) {
					// find scenario
					const Button *button = findButton(nodeId, state);
					if (button != nullptr)
						setNextScenario(button->scenarios, Button::SCENARIO_COUNT);
				} else {
					if (state != 0)
						onButton(nodeId, state);
				}
			}
			if (optionalLength >= 7) {
				uint32_t destinationId = (optionalData[1] << 24) | (optionalData[2] << 16) | (optionalData[3] << 8) | optionalData[4];
				int dBm = -optionalData[5];
				int security = optionalData[6];
				std::cout << "destinationId " << std::hex << destinationId << std::dec << " dBm " << dBm << " security " << security << std::endl;
			}
		}
		
	}
	
	void loop() {
		// update enocean protocol
		EnOceanProtocol::update();
		
		// update timers
		{
			// get current time in minutes
			uint32_t time = this->clock.getTime() & ~Clock::SECONDS_MASK;
			if (time != lastTime) {
				for (int i = 0; i < this->timers.size(); ++i) {
					const Timer &timer = this->timers.get(i);
					
					int weekday = (time & Clock::WEEKDAY_MASK) >> Clock::WEEKDAY_SHIFT;
					if (((timer.time ^ time) & (Clock::MINUTES_MASK | Clock::HOURS_MASK)) == 0
						&& (timer.time & (Timer::MONDAY << weekday)))
					{
						// trigger alarm
						setNextScenario(timer.scenarios, timer.SCENARIO_COUNT);
					}
				}
				this->lastTime = time;
			}
		}
		
		// update temperature
		this->temperature.update();
		
		// update system
		update();
		
		// update display
		this->display.update(this->bitmap);

		// update actor values
		this->actorValues.update(this->actors);

		// get relay outputs
		uint8_t outputs = 0;
		for (int i = 0; i < actors.size(); ++i) {
			const Actor &actor = actors.get(i);
			if (actor.type == Actor::Type::SWITCH) {
				outputs |= actorValues.getSwitchState(i) << actor.outputIndex;
			} else {
				outputs |= actorValues.getBlindState(i) << actor.outputIndex;
			}
		}
		//std::cout << int(outputs) << std::endl;
	}
	
	/**
	 * Set the next scenario in the given list of scenarios. The current scenario is determined by comparing each scenario
	 * with the actor target values
	 */
	void setNextScenario(const uint8_t *scenarios, int scenarioCount) {
		// find current scenario by comparing similarity to actor target values
		int bestSimilarity = 0x7fffffff;
		int bestI = -1;
		for (int i = 0; i < scenarioCount && scenarios[i] != 0xff; ++i) {
			const Scenario *scenario = findScenario(scenarios[i]);
			if (scenario != nullptr) {
				int similarity = actorValues.getSimilarity(scenario->actorValues);
				if (similarity < bestSimilarity) {
					bestSimilarity = similarity;
					bestI = i;
				}
			}
		}
		if (bestI != -1) {
			int currentScenario = scenarios[bestI];

			// check if actor (e.g. blind) is still moving
			if (actorValues.stop(findScenario(currentScenario)->actorValues)) {
				// stopped
				//std::cout << "stopped" << std::endl;
				toast() = "Stopped";
			} else {
				// next scenario
				//std::cout << "next" << std::endl;
				int nextI = bestI + 1;
				if (nextI >= Button::SCENARIO_COUNT || scenarios[nextI] == 0xff)
					nextI = 0;
				const Scenario *nextScenario = findScenario(scenarios[nextI]);

				// set actor values for next scenario
				actorValues.set(nextScenario->actorValues);
			
				// display scenario name
				toast() = nextScenario->name;
			}
		}
	}


	// menu state
	enum State {
		IDLE,
		TOAST,
		MAIN,
		ACTORS,
		SCENARIOS,
		EDIT_SCENARIO,
		ADD_SCENARIO,
		BUTTONS,
		EDIT_BUTTON,
		ADD_BUTTON,
		TIMERS,
		EDIT_TIMER,
		ADD_TIMER,
		SELECT_SCENARIO,
		HEATING
	};

	State getState() {return this->state;}

	void update() {
		int delta = this->poti.getDelta();
		
		if (this->poti.getPress()) {
			switch (this->state) {
			case IDLE:
				push(MAIN);
				break;
			case MAIN:
				if (this->selected == 0)
					push(ACTORS);
				else if (this->selected == 1)
					push(SCENARIOS);
				else if (this->selected == 2)
					push(BUTTONS);
				else if (this->selected == 3)
					push(TIMERS);
				else {
					// exit
					pop();
				}
				break;
			case ACTORS:
				{
					int actorCount = actors.size();
					if (this->selected < actorCount) {
					
					} else {
						// exit
						pop();
					}
				}
				break;
			case SCENARIOS:
				{
					int scenarioCount = scenarios.size();
					if (this->selected < scenarioCount) {
						// edit scenario
						this->scenario = scenarios.get(this->selected);
						push(EDIT_SCENARIO);
					} else if (this->selected == scenarioCount) {
						// new scenario
						
					} else {
						// exit
						pop();
					}
				}
				break;
			case EDIT_SCENARIO:
			case ADD_SCENARIO:
				{
					const int actorCount = actors.size();
					if (this->selected == 0) {
						// edit scenario name
					} else if (this->selected < 1 + actorCount) {
						// edit actor value
						this->edit ^= 1;
					} else if (this->selected == 1 + actorCount) {
						// delete scenario
						scenarios.erase(getThisIndex());
						pop();
					} else {
						// exit
						scenarios.write(getThisIndex(), this->scenario);
						pop();
					}
				}
				break;
			case BUTTONS:
				{
					int buttonCount = buttons.size();
					if (this->selected < buttonCount) {
						// edit button
						this->button = buttons.get(this->selected);
						push(EDIT_BUTTON);
					} else if (this->selected == buttonCount) {
						// new button
						this->button.id = 0;
						this->button.state = 0;
						for (int i = 0; i < Button::SCENARIO_COUNT; ++i)
							this->button.scenarios[i] = 0xff;
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
					int scenarioCount = this->button.getScenarioCount();
					if (this->selected < scenarioCount) {
						// select scenario
						int scenarioIndex = this->button.scenarios[this->selected];
						push(SELECT_SCENARIO);
						this->selected = scenarioIndex;
					} else if (this->selected == scenarioCount) {
						// add scenario
						push(SELECT_SCENARIO);
						this->selected = scenarioCount == 0 ? 0 : this->button.scenarios[scenarioCount - 1];
					} else if (this->selected == scenarioCount + 1) {
						// delete button
						this->buttons.erase(getThisIndex());
						pop();
					} else {
						// exit
						this->buttons.write(getThisIndex(), this->button);
						pop();
					}
				}
				break;
			case TIMERS:
				{
					int timerCount = timers.size();
					if (this->selected < timerCount) {
						// edit timer
						this->timer = timers.get(this->selected);
						push(EDIT_TIMER);
					} else if (this->selected == timerCount) {
						// new timer
						this->timer.time = 0;
						for (int i = 0; i < Button::SCENARIO_COUNT; ++i)
							this->timer.scenarios[i] = 0xff;
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
					int scenarioCount = this->timer.getScenarioCount();
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
					} else if (this->selected < 2 + scenarioCount) {
						// select scenario
						int scenarioIndex = this->timer.scenarios[this->selected - 2];
						push(SELECT_SCENARIO);
						this->selected = scenarioIndex;
					} else if (this->selected == 2 + scenarioCount) {
						// add scenario
						push(SELECT_SCENARIO);
						this->selected = scenarioCount == 0 ? 0 : this->timer.scenarios[scenarioCount - 1];
					} else if (this->selected == 2 + scenarioCount + 1) {
						// delete timer
						this->timers.erase(getThisIndex());
						pop();
					} else {
						// exit
						this->timers.write(getThisIndex(), this->timer);
						pop();
					}
				}
				break;
			case SELECT_SCENARIO:
				{
					int scenarioIndex = this->selected;
					pop();
					switch (this->state) {
					case EDIT_BUTTON:
					case ADD_BUTTON:
						if (scenarioIndex < scenarios.size()) {
							// set new scenario index to button
							this->button.scenarios[this->selected] = scenarioIndex;
							++this->selected;
						} else {
							// remove scenario from button
							for (int i = this->selected; i < Button::SCENARIO_COUNT - 1; ++i) {
								this->button.scenarios[i] = this->button.scenarios[i + 1];
							}
							this->button.scenarios[Button::SCENARIO_COUNT - 1] = 0xff;
						}
						break;
					case EDIT_TIMER:
					case ADD_TIMER:
						if (scenarioIndex < scenarios.size()) {
							// set new scenario index to timer
							this->timer.scenarios[this->selected - 2] = scenarioIndex;
							++this->selected;
						} else {
							// remove scenario from button
							for (int i = this->selected - 2; i < Timer::SCENARIO_COUNT - 1; ++i) {
								this->timer.scenarios[i] = this->timer.scenarios[i + 1];
							}
							this->timer.scenarios[Timer::SCENARIO_COUNT - 1] = 0xff;
						}
						break;
					default:
						;
					}
				}
				break;
			default:
				;
			}
		}

		// draw menu
		bitmap.clear();
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
		case TOAST:
			{
				const char *name = this->string;
				int y = 10;
				int len = tahoma_8pt.calcWidth(name, 1);
				bitmap.drawText((bitmap.WIDTH - len) >> 1, y, tahoma_8pt, name, 1);

				if (ticker.getValue() - this->toastTime > 3000)
					this->state = IDLE;
			}
			break;
		case MAIN:
			{
				menu(delta, 5);
				entry("Actors");
				entry("Scenarios");
				entry("Buttons");
				entry("Timers");
				entry("Exit");
			}
			break;
		case ACTORS:
			{
				int actorCount = actors.size();
				menu(delta, actorCount + 1);
				for (int i = 0; i < actorCount; ++i) {
					const Actor & actor = actors.get(i);
					this->string = actor.name;
					entry(this->string);
				}
				entry("Exit");
			}
			break;
		case SCENARIOS:
			{
				int scenarioCount = scenarios.size();

				menu(delta, scenarioCount + 2);
				for (int i = 0; i < scenarioCount; ++i) {
					const Scenario &scenario = scenarios.get(i);
					this->string = scenario.name;
					entry(this->string);
				}
				entry("Add Scenario");
				entry("Exit");
			}
			break;
		case EDIT_SCENARIO:
		case ADD_SCENARIO:
			{
				const int actorCount = actors.size();
				
				menu(delta, 1 + actorCount + 2);
				this->string = "Scenario: ", this->scenario.name;
				entry(this->string);
				line();
				
				// actors
				for (int i = 0; i < actorCount; ++i) {
					const Actor &actor = actors.get(i);
					this->string = actor.name, ": ";
					int8_t value = this->scenario.actorValues[i];
					
					// edit actor value
					if (isSelected() && this->edit == 1) {
						if (value == 100)
							value = 1;
						else if (value >= 1 && value <= 99)
							value += 1;
						
						value = clamp(value + delta, -1, actor.type == Actor::Type::SWITCH ? 1 : 100);
						
						if (value == 1)
							value = 100;
						else if (value >= 2 && value <= 100)
							value -= 1;
							
						this->scenario.actorValues[i] = value;
					}
					
					// display actor value
					int begin = this->string.length();
					if (value == -1) {
						this->string += " - ";
					} else if (value == 0) {
						if (actor.type == Actor::Type::SWITCH)
							this->string += "off";
						else
							this->string += "down";
					} else if (value == 100) {
						if (actor.type == Actor::Type::SWITCH)
							this->string += "on";
						else
							this->string += "up";
					} else {
						this->string += int(value);
					}
					int end = this->string.length();

					entry(this->string, begin, end);
				}
				line();
				entry("Delete Scenario");
				entry("Exit");
			}
			break;
		case BUTTONS:
			{
				int buttonCount = buttons.size();
				
				menu(delta, buttonCount + 2);
				for (int i = 0; i < buttonCount; ++i) {
					const Button &button = buttons.get(i);
					
					this->string = hex(button.id), ':', hex(button.state);
					entry(this->string);
				}
				entry("Add Button");
				entry("Exit");
			}
			break;
		case EDIT_BUTTON:
		case ADD_BUTTON:
			{
				int scenarioCount = this->button.getScenarioCount();

				menu(delta, scenarioCount + 3);
				this->string = "Button: ", hex(this->button.id), ':', hex(this->button.state);
				label(this->string);
				
				// scenarios
				line();
				for (int i = 0; i < scenarioCount; ++i) {
					this->string = scenarios.get(button.scenarios[i]).name;
					entry(this->string);
				}
				entry("Add Scenario");
				line();
				
				entry("Delete Button");
				entry("Exit");
			}
			break;
		case TIMERS:
			{
				int timerCount = timers.size();
				
				menu(delta, timerCount + 2);
				for (int i = 0; i < timerCount; ++i) {
					const Timer &timer = timers.get(i);
					int minutes = (timer.time & Clock::MINUTES_MASK) >> Clock::MINUTES_SHIFT;
					int hours = (timer.time & Clock::HOURS_MASK) >> Clock::HOURS_SHIFT;
					int weekdays = (timer.time & Timer::WEEKDAYS_MASK) >> Timer::WEEKDAYS_SHIFT;
					this->string = bcd(hours) , ':' , bcd(minutes, 2);
					this->string += "    ";
					entryWeekdays(this->string, weekdays);
				}
				entry("Add Timer");
				entry("Exit");
			}
			break;
		case EDIT_TIMER:
		case ADD_TIMER:
			{
				int scenarioCount = this->timer.getScenarioCount();

				menu(delta, 2 + scenarioCount + 3);
				
				// time
				int hours = (this->timer.time & Clock::HOURS_MASK) >> Clock::HOURS_SHIFT;
				int minutes = (this->timer.time & Clock::MINUTES_MASK) >> Clock::MINUTES_SHIFT;
				if (isSelected() && this->edit > 0) {
					// edit time
					if (this->edit == 1)
						hours = Clock::addTime(hours, delta, 0x23);
					else
						minutes = Clock::addTime(minutes, delta, 0x59);
					this->timer.time = (this->timer.time & ~(Clock::HOURS_MASK | Clock::MINUTES_MASK))
						| (hours << Clock::HOURS_SHIFT) | (minutes << Clock::MINUTES_SHIFT);
				}
				
				this->string = "Time: ";
				int begin1 = this->string.length();
				this->string += bcd(hours);
				int end1 = this->string.length();
				this->string += ':';
				int begin2 = this->string.length();
				this->string += bcd(minutes, 2);
				int end2 = this->string.length();
				entry(this->string, this->edit == 1 ? begin1 : begin2, this->edit == 1 ? end1 : end2);
				
				// weekdays
				int weekdays = this->timer.time >> Timer::WEEKDAYS_SHIFT;
				if (isSelected() && this->edit > 0) {
					// edit weekday
					this->edit = max(this->edit + delta, 0);
					if (this->edit > 7)
						this->edit = 0;
				}
				this->string = "Days: ";
				entryWeekdays(this->string, weekdays, this->edit - 1);

				// scenarios
				line();
				for (int i = 0; i < scenarioCount; ++i) {
					this->string = scenarios.get(timer.scenarios[i]).name;
					entry(this->string);
				}
				entry("Add Scenario");
				line();
				
				entry("Delete Timer");
				entry("Exit");
			}
			break;
		case SELECT_SCENARIO:
			{
				int scenarioCount = scenarios.size();
				menu(delta, scenarioCount + 1);
				for (int i = 0; i < scenarioCount; ++i) {
					entry(scenarios.get(i).name);
				}
				entry("Remove Scenario");
			}
			break;
		}
	}
	
	void onButton(uint32_t id, uint8_t state) {
		switch (this->state) {
		case BUTTONS:
			{
				int buttonCount = buttons.size();
				for (int i = 0; i < buttonCount; ++i) {
					const Button &button = buttons.get(i);
					if (button.id == id && button.state == state) {
						this->selected = i;
						this->button = buttons.get(i);
						push(EDIT_BUTTON);
						break;
					}
				}
			}
			break;
		case ADD_BUTTON:
			{
				this->button.id = id;
				this->button.state = state;

				// check if button already exists to prevent duplicates
				for (int i = 0; i < buttons.size(); ++i) {
					const Button &button = buttons.get(i);
					if (button.id == id && button.state == state) {
						// overwrite with existing button
						this->button = button;
						setThisIndex(i);
						break;
					}
				}
			}
			break;
		default:
			;
		}
	}
	
	String<32> &toast() {
		this->toastTime = ticker.getValue();
		this->state = TOAST;
		return this->string;
	}


	// connection to EnOcean controller
	//EnOceanProtocol protocol;
	
	// 128x64 display
	Bitmap<128, 64> bitmap;
	Display display;

	// digital potentiometer
	Poti poti;

	// clock for time of day and weekday
	Clock clock;
	uint32_t lastTime = 0;

	// actor values
	ActorValues actorValues;


	// temperature
	// -----------
	
	Temperature temperature;
	uint16_t targetTemperature = 12 << 1;


	// flash storage
	// -------------
	
	const Button *findButton(uint32_t id, uint8_t state) {
		for (int i = 0; i < buttons.size(); ++i) {
			const Button &button = buttons.get(i);
			if (button.id == id && button.state == state)
				return &button;
		}
		return nullptr;
	}

	const Scenario *findScenario(uint8_t id) {
		for (int i = 0; i < scenarios.size(); ++i) {
			const Scenario &scenario = scenarios.get(i);
			if (scenario.id == id)
				return &scenario;
		}
		return nullptr;
	}

	Storage storage;
	Array<Actor, Scenario::ACTOR_COUNT> actors;
	Array<Scenario, SCENARIO_COUNT> scenarios;
	Array<Button, BUTTON_COUNT> buttons;
	Array<Timer, TIMER_COUNT> timers;


	// menu
	// ----

	void menu(int delta, int entryCount) {
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
	
	void label(const char* s) {
		int x = 10;
		int y = this->entryY + 2 - this->yOffset;
		bitmap.drawText(x, y, tahoma_8pt, s, 1);
		this->entryY += tahoma_8pt.height + 4;
	}

	void line() {
		int x = 10;
		int y = this->entryY + 2 - this->yOffset;
		bitmap.fillRectangle(x, y, 108, 1);
		this->entryY += 1 + 4;
	}

	void entry(const char* s, int editBegin = 0, int editEnd = 0) {
		int x = 10;
		int y = this->entryY + 2 - this->yOffset;//this->entryIndex * lineHeight + 2 - this->yOffset;
		bitmap.drawText(x, y, tahoma_8pt, s, 1);
		if (this->entryIndex == this->selected) {
			bitmap.drawText(0, y, tahoma_8pt, ">", 0);
			if (this->edit > 0) {
				int start = tahoma_8pt.calcWidth(s, editBegin, 1) + 1;
				int width = tahoma_8pt.calcWidth(s + editBegin, editEnd - editBegin, 1);
				bitmap.fillRectangle(x + start, y + tahoma_8pt.height, width, 1);
			}
			this->lastSelected = this->selected;
			this->selectedY = this->entryY;
		}

		++this->entryIndex;
		this->entryY += tahoma_8pt.height + 4;
	}

	void entryWeekdays(const char* s, int weekdays, int index = 0) {
		int x = 10;
		int y = this->entryY + 2 - this->yOffset;
		int x2 = bitmap.drawText(x, y, tahoma_8pt, s, 1) + 1;
		int start;
		int width;
		for (int i = 0; i < 7; ++i) {
			bool enabled = weekdays & (1 << i);
			int x3 = bitmap.drawText(x2 + 1, y, tahoma_8pt, weekdaysShort[i], 1) + 1;
			if (enabled)
				bitmap.fillRectangle(x2, y, x3 - x2, tahoma_8pt.height - 1, Mode::FLIP);
			if (i == index) {
				start = x2;
				width = x3 - x2;
			}
			x2 = x3 + 3;
		}
		if (this->entryIndex == this->selected) {
			bitmap.drawText(0, y, tahoma_8pt, ">", 1);
			if (this->edit > 0)
				bitmap.fillRectangle(start, y + tahoma_8pt.height, width, 1);
			this->lastSelected = this->selected;
			this->selectedY = this->entryY;
		}

		++this->entryIndex;
		this->entryY += tahoma_8pt.height + 4;
	}
	

	bool isSelected() {
		return this->entryIndex == this->selected;
	}
	
	void push(State state) {
		this->stack[this->stackIndex++] = {this->state, this->selected, this->selectedY, this->yOffset};
		this->state = state;
		this->selected = 0;
		this->selectedY = 0;
		this->lastSelected = -1;
		this->yOffset = 0;
	}
	
	void pop() {
		--this->stackIndex;
		this->state = this->stack[this->stackIndex].state;
		this->selected = this->stack[this->stackIndex].selected;
		this->selectedY = this->stack[this->stackIndex].selectedY;
		this->yOffset = this->stack[this->stackIndex].yOffset;
	}
	
	int getThisIndex() {
		return this->stack[this->stackIndex - 1].selected;
	}

	void setThisIndex(int index) {
		this->stack[this->stackIndex - 1].selected = index;
	}

	// menu state
	State state = IDLE;

	// index of selected element
	int selected = 0;
	int lastSelected = 0;
	int selectedY = 0;
	
	// edit value of selected element
	int edit = 0;
	
	// menu display state
	int yOffset = 0;
	int entryIndex;
	int entryY;
	
	// menu stack
	struct StackEntry {State state; int selected; int selectedY; int yOffset;};
	int stackIndex = 0;
	StackEntry stack[6];
	
	// toast data
	int toastTime;
	
	// temporary string
	String<32> string;
	
	// temporary objects
	Scenario scenario;
	Button button;
	Timer timer;
};



// emulator
// --------
static void errorCallback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

static void mouseCallback(GLFWwindow* window, int button, int action, int mods) {
 /*   if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if(GLFW_PRESS == action)
            lbutton_down = true;
        else if(GLFW_RELEASE == action)
            lbutton_down = false;
    }

    if(lbutton_down) {
         // do your drag here
    }*/
}

class LayoutManager {
public:

	void add(Widget * widget) {
		this->widgets.push_back(widget);
	}
	
	void doMouse(GLFWwindow * window) {
		// get window size
		int windowWidth, windowHeight;
		glfwGetWindowSize(window, &windowWidth, &windowHeight);

		double x;
        double y;
        glfwGetCursorPos(window, &x, &y);
		bool leftDown = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
		//std::cout << "x " << x << " y " << y << " down " << leftDown << std::endl;
		if (leftDown) {
			bool first = this->activeWidget == nullptr;
			float windowX = x / windowWidth;
			float windowY = 1.0f - y / windowHeight;
			if (first) {
				// search widget under mouse
				for (Widget * widget : this->widgets) {
					if (widget->contains(windowX, windowY))
						this->activeWidget = widget;
				}
			}
			if (this->activeWidget != nullptr) {
				float localX = (windowX - this->activeWidget->x1) / (this->activeWidget->x2 - this->activeWidget->x1);
				float localY = (windowY - this->activeWidget->y1) / (this->activeWidget->y2 - this->activeWidget->y1);
				this->activeWidget->touch(first, localX, localY);
			}
		} else {
			if (this->activeWidget != nullptr)
				this->activeWidget->release();
			this->activeWidget = nullptr;
		}
	}
	
	void draw() {
		for (Widget *widget : this->widgets) {
			widget->draw();
		}
	}
	
	std::vector<Widget *> widgets;
	Widget* activeWidget = nullptr;
};


Actor actorsData[] = {
	{"Light1", 0, Actor::Type::SWITCH, 0, 0},
	{"Light2", 0, Actor::Type::SWITCH, 0, 1},
	{"Light3", 0, Actor::Type::SWITCH, 0, 2},
	{"Light4", 0, Actor::Type::SWITCH, 0, 3},
	{"Blind1", 0, Actor::Type::BLIND, 2000, 4},
	{"Blind2", 0, Actor::Type::BLIND, 2000, 6},
	{"Blind3", 0, Actor::Type::BLIND, 2000, 8},
	{"Blind4", 0, Actor::Type::BLIND, 2000, 10},
};

Scenario scenariosData[] = {
	{0, "Light1 On", {100, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{1, "Light1 Off", {0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{2, "Light2 On", {0xff, 100, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{3, "Light2 Dim", {0xff, 50, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{4, "Light2 Off", {0xff, 0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{5, "Blind1 Up", {0xff, 0xff, 0xff, 0xff, 100, 0xff, 0xff, 0xff}},
	{6, "Blind1 Down", {0xff, 0xff, 0xff, 0xff, 0, 0xff, 0xff, 0xff}},
	{7, "Blind2 Up", {0xff, 0xff, 0xff, 0xff, 0xff, 100, 0xff, 0xff}},
	{8, "Blind2 Mid", {0xff, 0xff, 0xff, 0xff, 0xff, 50, 0xff, 0xff}},
	{9, "Blind2 Down", {0xff, 0xff, 0xff, 0xff, 0xff, 0, 0xff, 0xff}},
	{10, "Light4 On", {0xff, 0xff, 0xff, 100, 0xff, 0xff, 0xff, 0xff}},
	{11, "Light4 Off", {0xff, 0xff, 0xff, 0, 0xff, 0xff, 0xff, 0xff}},
};

Button buttonsData[] = {
	{0xfef3ac9b, 0x10, {0, 1, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}}, // bottom left
	{0xfef3ac9b, 0x30, {2, 3, 4, 0xff, 0xff, 0xff, 0xff, 0xff}}, // top left
	{0xfef3ac9b, 0x50, {5, 6, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}}, // bottom right
	{0xfef3ac9b, 0x70, {7, 8, 9, 0xff, 0xff, 0xff, 0xff, 0xff}}, // top right
};


Timer timersData[] = {
	{Clock::time(22, 41) | Timer::SUNDAY, {10, 11, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{Clock::time(10, 0) | Timer::MONDAY, {10, 11, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}}
};

/**
 * Pass path to EnOcean device as argument
 * MacOS: /dev/tty.usbserial-FT3PMLOR
 * Linux: /dev/ttyUSB0 (add yourself to the dialout group: sudo usermod -a -G dialout $USER)
 */
int main(int argc, const char **argv) {
	if (argc <= 1) {
		std::cout << "usage: emulator <device>" << std::endl;
	#if __APPLE__
		std::cout << "example: emulator /dev/tty.usbserial-FT3PMLOR" << std::endl;
	#elif __linux__
		std::cout << "example: emulator /dev/ttyUSB0" << std::endl;
	#endif
		return 1;
	}
	
	// erase flash
	for (int i = 0; i < Flash::PAGE_COUNT; ++i) {
		Flash::erase(i);
	}

	// read flash contents from file
	std::ifstream is("flash.bin", std::ios::binary);
	//is.read((char*)Flash::data, sizeof(Flash::data));
	is.close();

	// get device from command line
	std::string device = argv[1];
	
	// init GLFW
	glfwSetErrorCallback(errorCallback);
	if (!glfwInit())
		exit(EXIT_FAILURE);
	
	// create GLFW window and OpenGL 3.3 Core context
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	GLFWwindow *window = glfwCreateWindow(640, 480, "RoomControl", NULL, NULL);
	if (!window) {
		glfwTerminate();
		exit(EXIT_FAILURE);
	}
	glfwSetKeyCallback(window, keyCallback);
	glfwSetMouseButtonCallback(window, mouseCallback);
	
	// make OpenGL context current
	glfwMakeContextCurrent(window);
	
	// load OpenGL functions
	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
	
	// no v-sync
	glfwSwapInterval(0);

	System system(device);

	// default initialize arrays if empty
	if (system.actors.size() == 0) {
		system.actors.assign(actorsData);
		system.scenarios.assign(scenariosData);
		system.buttons.assign(buttonsData);
		system.timers.assign(timersData);
	}

	LayoutManager layoutManager;
	float y = 0.9f;

	// display
	layoutManager.add(&system.display);
	system.display.setRect(0.3f, y - 0.2f, 0.4f, 0.2f);

	y -= 0.3f;

	// potis
	layoutManager.add(&system.poti);
	system.poti.setRect(0.2f, y - 0.25f, 0.25f, 0.25f);
	//Poti poti2;
	//layoutManager.add(&poti2);
	//poti2.setRect(0.55f, y - 0.25f, 0.25f, 0.25f);

	// temperature
	layoutManager.add(&system.temperature);
	system.temperature.setRect(0.85f, y - 0.25f, 0.1, 0.25f);

	y -= 0.3f;

	// lights
	Light lights[4];
	for (int i = 0; i < 4; ++i) {
		layoutManager.add(&lights[i]);
		const float step = 1.0f / 8.0f;
		const float size = 0.1f;
		lights[i].setRect((step - size) * 0.5f + i * step, y - size, size, size);
	}

	// blinds
	Blind blinds[4];
	for (int i = 0; i < 4; ++i) {
		layoutManager.add(&blinds[i]);
		const float step = 1.0f / 8.0f;
		const float size = 0.1f;
		blinds[i].setRect((step - size) * 0.5f + (i + 4) * step, y - size * 2, size, size * 2);
	}
	
	// main loop
	int frameCount = 0;
	auto start = std::chrono::steady_clock::now();
	while (!glfwWindowShouldClose(window)) {
		auto frameStart = std::chrono::steady_clock::now();

		// get frame buffer size
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);

		// mouse
		layoutManager.doMouse(window);
		
		system.loop();
		
		// transfer actor values to emulator
		for (int i = 0; i < 4; ++i) {
			lights[i].setValue(system.actorValues.get(i));
			blinds[i].setValue(system.actorValues.get(i + 4));
		}

		// draw emulator on screen
		glViewport(0, 0, width, height);
		glClear(GL_COLOR_BUFFER_BIT);
		layoutManager.draw();
		
		glfwSwapBuffers(window);
		glfwPollEvents();

		auto now = std::chrono::steady_clock::now();
		std::this_thread::sleep_for(std::chrono::milliseconds(9) - (now - frameStart));
		
		// show frames per second
		++frameCount;
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);
		if (duration.count() > 1000) {
			//std::cout << frameCount * 1000 / duration.count() << "fps" << std::endl;
			frameCount = 0;
			start = std::chrono::steady_clock::now();
		}
	}

	// write flash
	std::ofstream os("flash.bin", std::ios::binary);
	os.write((char*)Flash::data, sizeof(Flash::data));
	os.close();

	// cleanup
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
