#pragma once

#include "EnOceanProtocol.hpp"
#include "Bitmap.hpp"
#include "Display.hpp"
#include "Poti.hpp"
#include "MotionDetector.hpp"
#include "Temperature.hpp"
#include "Storage.hpp"
#include "Clock.hpp"
#include "DeviceState.hpp"
#include "Scenario.hpp"
#include "Event.hpp"
#include "String.hpp"
#include "Ticker.hpp"


static const int EVENT_COUNT = 32;
static const int SCENARIO_COUNT = 32;
static const int DEVICE_COUNT = 32;


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

		HEATING
	};

	State getState() {return this->state;}

	void updateMenu();
	
	void onSwitch(uint32_t input, uint8_t state);
	
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
	Storage::Array<Event, EVENT_COUNT> events;
	uint32_t eventStates[(EVENT_COUNT + 31) / 32];

	// scenarios
	Storage::Array<Scenario, SCENARIO_COUNT> scenarios;

	// devices
	Storage::Array<Device, DEVICE_COUNT> devices;
	DeviceState deviceStates[DEVICE_COUNT];

	// 128x64 display
	Bitmap<128, 64> bitmap;


	// actions
	// -------
	
	Action doFirstAction(const Event &event);
	void doFirstActionAndToast(const Event &event);

	bool doAllActions(const Action *actions, int count);
	
	// set temp string with action (device/state, device/transition or scenario)
	void setAction(Action action);

	void listActionsSaveCancel();

	// set temp string with constraint (device/state)
	void setConstraint(Action action);

	// ids
	// ---
	
	template <typename T, int C>
	static uint8_t newId(Storage::Array<T, C> &array) {
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

	template <typename T, int C>
	static void deleteId(Storage::Array<T, C> &array, uint8_t id) {
		for (int index = 0; index < array.size(); ++index) {
			T element = array[index];
			
			// delete all actions with given id
			int actionCount = element.getActionCount();
			int j = 0;
			for (int i = 0; i < actionCount; ++i) {
				if (element.actions[i].id != id) {
					element.actions[j] = element.actions[i];
					++j;
				}
			}
			if (j < actionCount) {
				for (; j < T::ACTION_COUNT; ++j) {
					element.actions[j] = {0xff, 0xff};
				}
				array.write(index, element);
			}
		}
	}
	

	// events
	// --------
	
	int getEventCount(Event::Type type);

	const Event &getEvent(Event::Type type, int index);


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

	void updateSelection(int delta);

	void menu();
	void menu(int delta) {updateSelection(delta); menu();}

	void label(const char* s);

	void line();
	
	void entry(const char* s, bool underline = false, int begin = 0, int end = 0);

	void entryWeekdays(const char* s, int weekdays, bool underline = false, int index = 0);
	
	bool isSelectedEntry() {
		return this->entryIndex == this->selected;
	}

	bool isEditEntry() {
		return this->entryIndex == this->selected && this->edit > 0;
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

	// index of selected element
	int selected = 0;
	int lastSelected = 0;
	int selectedY = 0;
	
	// edit value of selected element
	int edit = 0;
	
	// menu display state
	int yOffset = 0;
	int entryIndex = 0xffff;
	int entryY;
	
	// menu stack
	struct StackEntry {State state; int selected; int selectedY; int yOffset;};
	int stackIndex = 0;
	StackEntry stack[6];
	
	// toast data
	uint32_t toastTime;
	
	// temporary string buffer
	StringBuffer<32> buffer;
	
	// temporary objects
	union {
		Actions actions;
		Event event;
		Scenario scenario;
	} u;
	Device device;
};
