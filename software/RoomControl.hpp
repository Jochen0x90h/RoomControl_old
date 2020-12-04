#pragma once

#include "MqttSnBroker.hpp"
#include "Lin.hpp"
#include "Clock.hpp"
#include "Display.hpp"
#include "Poti.hpp"
#include "Bitmap.hpp"
#include "StringBuffer.hpp"
#include "Storage.hpp"
#include "StringSet.hpp"
#include "TopicBuffer.hpp"


/**
 * Main room control class that inherits platform dependent (hardware or emulator) components
 */
class RoomControl : public MqttSnBroker, public Lin, public Clock, public Display, public Poti {
public:
	// maximum number of mqtt routes
	static constexpr int MAX_ROUTE_COUNT = 32;
	
	// maximum number of timers
	static constexpr int MAX_TIMER_COUNT = 32;


	RoomControl();

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
		EDIT_LOCAL_DEVICE,
		
		// connections
		ROUTES,
		EDIT_ROUTE,
		ADD_ROUTE,

		// timers
		TIMERS,
		EDIT_TIMER,
		ADD_TIMER,

		// rules
		
		
		// helper
		SELECT_TOPIC,
	};
	
	
	void updateMenu(int delta, bool activated);

	// menu state
	State state = IDLE;

// Menu System
// -----------

	void menu(int delta, bool activated);

	void label(String s);

	void line();
	
	bool entry(String s, bool underline = false, int begin = 0, int end = 0);

	bool entryWeekdays(String s, int weekdays, bool underline = false, int index = 0);
	
	bool isSelectedEntry() {
		return this->selected == this->entryIndex;
	}

	/**
	 * Get edit state. Returns 0 if not in edit mode or not the entry being edited, otherwise returns the 1-based index
	 * of the field being edited
	 */
	int getEdit(int editCount = 1);

	/*bool isEditEntry(int entryIndex) {
		return this->selected == entryIndex && this->edit > 0;
	}*/

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
	bool stackHasChanged = false;

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


// Storage
// -------

	Storage storage;


// Room
// ----

	struct Room {
		// name of this room
		char name[16];

		/**
		 * Returns the size in bytes needed to store the control configuration in flash
		 * @return size in bytes
		 */
		int getFlashSize() const;

		/**
		 * Returns the size in bytes needed to store the contol state in ram
		 * @return size in bytes
		 */
		int getRamSize() const;
	};

	struct RoomState {
		// topic id for list of rooms in house (<prefix>)
		uint16_t houseTopicId;

		// topic id for list of devices in room (<prefix>/<room>)
		uint16_t roomTopicId;
	};

	Storage::Array<Room, RoomState> room;
	using RoomElement = Storage::Element<Room, RoomState>;


// Devices
// -------
	
	struct LocalDevice : public Lin::Device {
	
		/**
		 * Returns the size in bytes needed to store the device configuration in flash
		 * @return size in bytes
		 */
		int getFlashSize() const;

		/**
		 * Returns the size in bytes needed to store the device state in ram
		 * @return size in bytes
		 */
		int getRamSize() const;
		
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
		
		/**
		 * Assigns another device to this device by copying the number of bytes given by getFlashSize().
		 * The actual type must be the same or enough space following to accomodate all data
		 */
		void operator =(LocalDevice const &device);
	};
	
	struct DeviceState {
		// topic id for list of attributes in device (<prefix>/<room>/<device>)
		uint16_t deviceTopicId;
		
		// topic id for room command (<prefix>/<room>/<device>/cmd)
		//uint16_t deviceCommand;
	};
	
	struct Switch2Device : public LocalDevice {
		struct Relays {
			// relay/light: on duration, blind: duration from fully open to fully closed
			SystemDuration duration1;
			
			// blind: rocker long press duration
			SystemDuration duration2;
		};
		Relays r[2];

		// device name
		char name[16];
	};
	
	struct Switch2State : public DeviceState {
		// switches A and B
		struct Switches {
			// topic id's for publishing the state
			uint16_t stateId1;
			uint16_t stateId2;
		};

		// relays X and Y
		struct Relays {
			// topic id's for publishing the state
			uint16_t stateId1;
			uint16_t stateId2;

			// topic id's for receiving commands
			uint16_t commandId1;
			uint16_t commandId2;

			// relay/light: on timeout, blind: move timeout
			SystemTime timeout1;
			
			// blind: rocker long press timeout
			SystemTime timeout2;
			
			// blind: current position measured as time
			SystemDuration duration;
		};
				
		// current state of switches
		uint8_t switches;
		
		// current state of relays
		uint8_t relays;

		// track prssed state of relay commands to filter out double messages
		uint8_t pressed;

		// additional state for controlling the relays
		Relays r[2];

		// additional state for switches
		Switches s[2];
	};
	
	// subscribe devices to mqtt topics
	void subscribeDevices();
	
	// subscribe one device to mqtt topics
	void subscribeDevice(int index);

	// update time dependent state of devices (e.g. blind position)
	SystemTime updateDevices(SystemTime time);
	
	Storage::Array<LocalDevice, DeviceState> localDevices;
	using LocalDeviceElement = Storage::Element<LocalDevice, DeviceState>;
	
	// next time for reporting changing values
	SystemTime nextReportTime;
	
	// time of last update of changing values
	SystemTime lastUpdateTime;


// Routes
// ------

	struct Route {
		Route &operator =(const Route &) = delete;

		/**
		 * Returns the size in bytes needed to store the device configuration in flash
		 * @return size in bytes
		 */
		int getFlashSize() const;

		/**
		 * Returns the size in bytes needed to store the device state in ram
		 * @return size in bytes
		 */
		int getRamSize() const;
	
		String getSrcTopic() const {return {buffer, this->srcTopicLength};}
		String getDstTopic() const {return {buffer + this->srcTopicLength, this->dstTopicLength};}

		void setSrcTopic(String topic);
		void setDstTopic(String topic);
	
		uint8_t srcTopicLength;
		uint8_t dstTopicLength;
		uint8_t buffer[TopicBuffer::MAX_TOPIC_LENGTH * 2];
	};
	
	struct RouteState {
		RouteState &operator =(const RouteState &) = delete;

		uint16_t srcTopicId;
		uint16_t dstTopicId;
	};

	// initialize route (register/subscribe mqtt topics)
	void initRoute(int index);

	void copy(Route &dstRoute, RouteState &dstRouteState, Route const &srcRoute, RouteState const &srcRouteState);

	Storage::Array<Route, RouteState> routes;


// Command
// -------

	/**
	 * Command to be executed e.g. on timer event.
	 * Contains an mqtt topic where to publish a value and the value itself, both are stored in an external data buffer
	 */
	struct Command {
		enum Type : uint8_t {
			// button: pressed or release (bt)
			BUTTON,
			
			// rocker: up, down or release (rc)
			ROCKER,
			
			// binary: on or off (bn)
			BINARY,
			
			// 8 bit value, 0-255 (v8)
			VALUE8,
			
			// 0-100% (r1)
			PERCENTAGE,
			
			// room temperature 8°C - 33.5°C (rt)
			TEMPERATURE,
			
			// color rgb (cr)
			COLOR_RGB,

			TYPE_COUNT,
		};
		
		/**
		 * Get the size of the value in bytes
		 */
		int getValueSize() const;
		
		/**
		 * Get the number of values (e.g. 3 for rgb color)
		 */
		int getValueCount() const;
		
		int getFlashSize() const {return this->topicLength + getValueSize();}
		
		void changeType(int delta, uint8_t *data, uint8_t *end);
		
		void setTopic(String topic, uint8_t *data, uint8_t *end);
		
		// type of value that is published on the topic
		Type type;

		// length of topic where value gets published
		uint8_t topicLength;
	};

	void publishValue(uint16_t topicId, Command::Type type, uint8_t const *value);


// Timers
// ------

	struct Timer {
		// weekday flags
		static constexpr uint8_t MONDAY = 0x01;
		static constexpr uint8_t TUESDAY = 0x02;
		static constexpr uint8_t WEDNESDAY = 0x04;
		static constexpr uint8_t THURSDAY = 0x08;
		static constexpr uint8_t FRIDAY = 0x10;
		static constexpr uint8_t SATURDAY = 0x20;
		static constexpr uint8_t SUNDAY = 0x40;

		static constexpr int MAX_COMMAND_COUNT = 16;
		static constexpr int MAX_VALUE_SIZE = 4;
		static constexpr int BUFFER_SIZE = (sizeof(Command) + MAX_VALUE_SIZE + TopicBuffer::MAX_TOPIC_LENGTH)
			* MAX_COMMAND_COUNT;

		Timer &operator =(const Timer &) = delete;

		/**
		 * Returns the size in bytes needed to store the device configuration in flash
		 * @return size in bytes
		 */
		int getFlashSize() const;

		/**
		 * Returns the size in bytes needed to store the device state in ram
		 * @return size in bytes
		 */
		int getRamSize() const;

		uint8_t *begin() {return this->u.buffer + sizeof(Command) * this->commandCount;}
		uint8_t const *begin() const {return this->u.buffer + sizeof(Command) * this->commandCount;}
		uint8_t *end() {return this->u.buffer + BUFFER_SIZE;}

		// timer time and weekday flags
		ClockTime time;
	
	
		uint8_t commandCount;
		union {
			// list of commands, overlaps with buffer
			Command commands[MAX_COMMAND_COUNT];
			
			// buffer for commands, topics and values
			uint8_t buffer[BUFFER_SIZE];
		} u;
	};
	
	struct TimerState {
		TimerState &operator =(const TimerState &) = delete;

		uint16_t topicIds[Timer::MAX_COMMAND_COUNT];
	};
	
	// initialize timer (register mqtt topics)
	void initTimer(int index);

	void copy(Timer &dstTimer, TimerState &dstTimerState, Timer const &srcTimer, TimerState const &srcTimerState);

	Storage::Array<Timer, TimerState> timers;
	using TimerElement = Storage::Element<Timer, TimerState>;


// Clock
// -----

void onSecondElapsed() override;


// Temp
// ----

	union {
		Room room;
		Lin::Device linDevice;
		LocalDevice localDevice;
		Switch2Device switch2Device;
		Route route;
		Timer timer;
	} temp;
	union {
		DeviceState localDevice;
		RouteState route;
		TimerState timer;
	} tempState;
	int tempIndex;
	

// Topic Selector
// --------------

	void enterTopicSelector(String topic, bool onlyCommands, int index);

	// selected topic including space for prefix
	TopicBuffer selectedTopic;
	uint16_t selectedTopicId = 0;
	uint16_t topicDepth;
	StringSet<64, 64 * 16> topicSet;
	bool onlyCommands;

};
