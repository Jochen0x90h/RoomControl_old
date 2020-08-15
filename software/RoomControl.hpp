#pragma once

#include "MqttSnBroker.hpp"
#include "Clock.hpp"
#include "Display.hpp"
#include "Poti.hpp"
#include "Bitmap.hpp"
#include "StringBuffer.hpp"
#include <iostream>

inline std::ostream &operator << (std::ostream &s, String const &str) {
	s << std::string(str.data, str.length);
	return s;
}

/**
 * Main room control class that inherits platform dependent (hardware or emulator) components
 */
class RoomControl : public MqttSnBroker, public Clock, public Display, public Poti {
public:

	RoomControl(UpLink::Parameters const &upParameters, DownLink::Parameters downParameters)
		: MqttSnBroker(upParameters, downParameters) {}

	~RoomControl() override;


// UpLink
// ------
	
	void onUpConnected() override {
		connect("MyClient");
	}


// MqttSnClient
// ------------

	uint16_t foo;

	void onConnected() override {
		std::cout << "onConnected" << std::endl;

		// register a topic name to obtain a topic id
		this->foo = registerTopic("foo").topicId;
	}
	
	void onDisconnected() override {
	
	}
	
	void onSleep() override {
	
	}
	
	void onWakeup() override {
	
	}
	
	void onError(int error, mqttsn::MessageType messageType) override {
	
	}
/*
	void onRegistered(uint16_t msgId, String topicName, uint16_t topicId) override {
		MqttSnBroker::onRegistered(msgId, topicName, topicId);
		
		std::cout << "onRegistered " << topicName << ' ' << topicId << std::endl;
		this->stateId = topicId;

		// subscribe to command topic
		subscribeTopic("cmd");
	}
*/

// MqttSnBroker
// ------------

	void onPublished(uint16_t topicId, uint8_t const *data, int length, int8_t qos, bool retain) override {
		std::string s((char const*)data, length);
		std::cout << "onPublished " << topicId << " data " << s << " retain " << retain << " qos " << int(qos) << std::endl;
	}
/*
	void onSubscribed(uint16_t msgId, String topicName, uint16_t topicId, int8_t qos) override {
		MqttSnBroker::onSubscribed(msgId, topicName, topicId, qos);
		
		std::cout << "onSubscribed " << topicName << ' ' << topicId << ' ' << int(qos) << std::endl;
		this->commandId = topicId;

		// publish a message on the state topic
		publish(this->stateId, "foo", 1);
	}
*/

	
// SystemTimer
// -----------
		
	void onSystemTimeout3(SystemTime time) override {
	
	}


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
};
