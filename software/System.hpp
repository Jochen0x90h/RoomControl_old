#pragma once

#include "EnOceanProtocol.hpp"
#include "Bitmap.hpp"
#include "Poti.hpp"
#include "MotionDetector.hpp"
#include "Temperature.hpp"
#include "Storage.hpp"
#include "Clock.hpp"
#include "Event.hpp"
#include "Timer.hpp"
#include "Scenario.hpp"
#include "DeviceState.hpp"
#include "String.hpp"
#include "Ticker.hpp"
#include "config.hpp"


class System {
public:

	System(Serial::Device device);
	
	void onPacket(uint8_t packetType, const uint8_t *data, int length, const uint8_t *optionalData, int optionalLength);
	
	void update();
	
	// menu state
	enum State {
		IDLE,
		MAIN,
		
		// events
		EVENTS,
		EDIT_EVENT,
		ADD_EVENT,

		// timers
		TIMERS,
		EDIT_TIMER,
		ADD_TIMER,

		// scenarios
		SCENARIOS,
		EDIT_SCENARIO,
		ADD_SCENARIO,
		
		// actions that can be triggered by buttons, timers and scenarios
		SELECT_ACTION,
		ADD_ACTION,
		
		// devices such as switch, light, blind and window handle
		DEVICES,
		EDIT_DEVICE,
		ADD_DEVICE,
		SELECT_CONDITION,
		ADD_CONDITION,
	};

	State getState() {return this->state;}

	void updateMenu();
	
	void onEvent(uint32_t input, uint8_t state);
	
	StringBuffer<32> &toast() {
		this->toastTime = this->ticker.getTicks();
		return this->buffer;
	}

	
	// digital potentiometer with switch
	Poti poti;

	// motion detector
	MotionDetector motionDetector;

	// wall clock time including weekday
	Clock clock;
	uint32_t lastTime = 0;

	// system ticks in milliseconds
	Ticker ticker;
	uint32_t lastTicks = 0;

	// temperature sensor
	Temperature temperature;
	uint16_t targetTemperature = 12 << 1;
 
	// connection to enocean module
	EnOceanProtocol protocol;
 
	// flash storage for events, scenarios and devices
	Storage storage;

	// events
	Storage::Array<Event> events;
	uint32_t eventStates[(EVENT_COUNT + 31) / 32];

	// timers
	Storage::Array<Timer> timers;

	// scenarios
	Storage::Array<Scenario> scenarios;

	// devices
	Storage::Array<Device> devices;
	DeviceState deviceStates[DEVICE_COUNT];

	// 128x64 display
	Bitmap<128, 64> bitmap;


	// actions
	// -------
		
	Action doFirstAction(const Actions &actions);
	void doFirstActionAndToast(const Actions &actions);

	bool doAllActions(const Actions &actions);
	
	// set temp string with action (device/state, device/transition or scenario)
	void setAction(Action action);

	void listActions();

	// set temp string with constraint (device/state)
	void setConstraint(Action action);

	// ids
	// ---
	
	template <typename T>
	static uint8_t newId(Storage::Array<T> &array) {
		bool found;
		uint8_t id = 0;
		int deviceCount = array.size();
		do {
			found = true;
			for (int i = 0; i < deviceCount; ++i) {
				if (array[i].id == id) {
					++id;
					found = false;
				}
			}
		} while (!found);
		return id;
	}

	template <typename T>
	static void deleteScenarioId(Storage::Array<T> &array, uint8_t id) {
		for (int index = 0; index < array.size(); ++index) {
			T element = array[index];
			
			// delete all actions with given scenario id
			if (element.actions.deleteId(id, Action::SCENARIO, Action::SCENARIO)) {
				array.write(index, element);
			}
		}
	}

	template <typename T>
	static void deleteDeviceId(Storage::Array<T> &array, uint8_t id) {
		for (int index = 0; index < array.size(); ++index) {
			T element = array[index];

			// delete all actions with given device id
			if (element.actions.deleteId(id, 0, Action::TRANSITION_END)) {
				array.write(index, element);
			}
		}
	}



	// scenarios
	// ---------

	//const Scenario *findScenario(uint8_t id);

	uint8_t newScenarioId();
	
	void deleteScenarioId(uint8_t id);
	
	// devices
	// -------

	bool isDeviceStateActive(uint8_t id, uint8_t state);

	void applyConditions(const Device &device, DeviceState &deviceState);

	void addDeviceStateToString(const Device &device, DeviceState &deviceState);
	
	uint8_t newDeviceId();
	
	void deleteDeviceId(uint8_t id);

	// menu
	// ----

	void menu(int delta, bool activated);

	void label(const char* s);

	void line();
	
	bool entry(const char* s, bool underline = false, int begin = 0, int end = 0);

	bool entryWeekdays(const char* s, int weekdays, bool underline = false, int index = 0);
	
	bool isSelectedEntry() {
		return this->selected == this->entryIndex;
	}

	bool isEdit(int editCount = 1);

	bool isEditEntry(int entryIndex) {
		return this->selected == entryIndex && this->edit > 0;
	}

	void push(State state);
	
	void pop();
	
	int getThisIndex() {
		return this->stack[this->stackIndex - 1].selected;
	}

	void setThisIndex(int index) {
		this->stack[this->stackIndex - 1].selected = index;
	}

	// menu state
	State state = IDLE;

	// index of selected menu entry
	int selected = 0;
	
	// y coordinate of selected menu entry
	int selectedY = 0;
		
	// starting y coodinate of display
	int yOffset = 0;
	
	// menu stack
	struct StackEntry {State state; int selected; int selectedY; int yOffset;};
	int stackIndex = 0;
	StackEntry stack[6];

	// true if selected menu entry was activated
	bool activated = false;

	// index of current menu entry
	int entryIndex = 0xffff;
	
	// y coordinate of current menu entry
	int entryY;

	// edit value of selected element
	int edit = 0;

	// toast data
	uint32_t toastTime;
	
	// temporary string buffer
	StringBuffer<32> buffer;
	
	// temporary objects
	union {
		Event event;
		Timer timer;
		Scenario scenario;
		Device device;
	} u;
	Actions *actions;
	uint8_t actionIndex;
};
