#pragma once

#include "MqttSnBroker.hpp"
#include "Lin.hpp"
#include "Clock.hpp"
#include "Display.hpp"
#include "Poti.hpp"
#include "Bitmap.hpp"
#include "StringBuffer.hpp"
#include "Storage.hpp"
#include <iostream>

inline std::ostream &operator << (std::ostream &s, String const &str) {
	s << std::string(str.data, str.length);
	return s;
}

/**
 * Main room control class that inherits platform dependent (hardware or emulator) components
 */
class RoomControl : public MqttSnBroker, public Lin, public Clock, public Display, public Poti {
public:

	RoomControl(UpLink::Parameters const &upParameters, DownLink::Parameters downParameters,
		Lin::Parameters linParameters)
		: MqttSnBroker(upParameters, downParameters), Lin(linParameters)
		, storage(0, FLASH_PAGE_COUNT, localDevices)
		
	{
		SystemTime time = getSystemTime();
		this->lastUpdateTime = time;
		this->nextReportTime = time;
		onSystemTimeout3(time);
	}

	~RoomControl() override;


// UpLink
// ------
	
	void onUpConnected() override;


// MqttSnClient
// ------------

	void onConnected() override;
	
	void onDisconnected() override;
	
	void onSleep() override;
	
	void onWakeup() override;
	
	void onError(int error, mqttsn::MessageType messageType) override;
	

// MqttSnBroker
// ------------

	void onPublished(uint16_t topicId, uint8_t const *data, int length, int8_t qos, bool retain) override;
	

// Lin
// ---

	void onLinReady() override;

	void onLinReceived(uint32_t deviceId, uint8_t const *data, int length) override;

	void onLinSent() override;

	
// SystemTimer
// -----------
		
	void onSystemTimeout3(SystemTime time) override;


// Clock
// -----

	void onSecondElapsed() override;


// Display
// -------

	void onDisplayReady() override;

	// display bitmap
	Bitmap<128, 64> bitmap;
	

// Poti
// ----

	void onPotiChanged(int delta, bool activated) override;


// Menu
// ----

	// menu state
	enum State {
		IDLE,
		MAIN,
		
		// local devices connected by lin bus
		LOCAL_DEVICES,
		
		
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
	
	void updateMenu(int delta, bool activated);

	// menu state
	State state = IDLE;

// Menu System
// -----------

	void menu(int delta, bool activated);

	void label(const char* s);

	void line();
	
	bool entry(String s, bool underline = false, int begin = 0, int end = 0);

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
	SystemTime toastTime;
	
	// temporary string buffer
	StringBuffer<32> buffer;


// Devices
// -------
	
	struct LocalDevice {
		Lin::Device device;
	
		/**
		 * Returns the size in bytes needed to store the device configuration in flash
		 * @return size in bytes
		 */
		int flashSize() const;

		/**
		 * Returns the size in bytes needed to store the device state in ram
		 * @return size in bytes
		 */
		int ramSize() const;
		
		/**
		 * Returns the name of the device
		 * @return device name
		 */
		String getName() const;
		
		/**
		 * Set the name of the device
		 * @paran name device name
		 */
		void setName(String name);
	};
	
	struct Switch2Device : public LocalDevice {
		struct Relays {
			SystemDuration duration1;
			SystemDuration duration2;
		};
		Relays r[2];

		// device name
		char name[16];
	};
	
	struct Switch2State {
		struct Relays {
			uint16_t topicId1;
			uint16_t topicId2;
			SystemTime timeout1;
			SystemTime timeout2;
			SystemDuration duration;
		};
		
		// current state of switches
		uint8_t switches;
		
		// current state of relays
		uint8_t relays;

		// additional state for controling the relays
		Relays r[2];


		// topic id for rocker a
		uint16_t a;
		
		// topic id for rocker b
		uint16_t b;
	};
	
	// subscribe devices to mqtt topics
	void subscribeDevices();
	
	// update time dependent state of devices (e.g. blind position)
	SystemTime updateDevices(SystemTime time);
	
	Storage storage;
	Storage::Array<LocalDevice, void> localDevices;
	using LocalDeviceElement = Storage::Element<LocalDevice, void>;
	union {
		LocalDevice localDevice;
		Switch2Device switch2Device;
	} temp;
	
	// next time for reporting changing values
	SystemTime nextReportTime;
	
	// time of last update of changing values
	SystemTime lastUpdateTime;
};
