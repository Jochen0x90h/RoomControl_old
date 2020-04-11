#pragma once

#include "MqttSnClient.hpp"
#include "Clock.hpp"
#include "Display.hpp"
#include "Poti.hpp"
#include "Bitmap.hpp"
#include "StringBuffer.hpp"
#include <iostream>


class RoomControl : public MqttSnClient, public Clock, public Display, public Poti {
public:

	RoomControl() {}

	~RoomControl() override;
	
// MqttSnClient
// ------------

	void onGatewayFound(const Udp6Endpoint &system, uint8_t gatewayId) override {
		std::cout << "onGatewayFound" << std::endl;
		Udp6Endpoint gw;
		gw.address = sender.address;
		gw.scope = sender.scope;
		gw.port = GATEWAY_PORT;
		connect(gw, gatewayId, "MyClient");
	}

	void onConnected() override {
		std::cout << "onConnected" << std::endl;

		// register a topic name to obtain a topic id
		registerTopic("state");
	}
	
	void onDisconnected() override {
	
	}
	
	void onSleep() override {
	
	}
	
	void onWakeup() override {
	
	}
			
	void onRegister(const char* topic, uint16_t topicId) override {
	
	}

	void onRegistered(uint16_t packetId, uint16_t topicId) override {
		std::cout << "onRegistered " << packetId << ' ' << topicId << std::endl;
		this->stateId = topicId;

		// subscribe to command topic
		subscribeTopic("cmd");
	}

	void onPublish(uint16_t topicId, uint8_t const *data, int length, bool retained, int8_t qos) override {
		std::string s((char const*)data, length);
		std::cout << "onPublish " << topicId << " data " << s << " retained " << retained << " qos " << int(qos) << std::endl;
	}

	void onPublished(uint16_t packetId, uint16_t topicId) override {
		std::cout << "onPublished " << packetId << ' ' << topicId << std::endl;
	}

	void onSubscribed(uint16_t packetId, uint16_t topicId) override {
		std::cout << "onSubscribed " << packetId << ' ' << topicId << std::endl;
		this->commandId = topicId;

		// publish a message on the state topic
		publish(this->stateId, "foo");
	}

	void onUnsubscribed() override {
	
	}
	
	void onPacketError() override {
	
	}

	void onCongestedError(MQTTSN_msgTypes messageType) override {
	
	}

	void onUnsupportedError(MQTTSN_msgTypes messageType) override {
	
	}

	SystemTime doNext(SystemTime time) override {
		std::cout << "doNext" << std::endl;
				
		return time;
	}


	int stateId;
	int commandId;


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
