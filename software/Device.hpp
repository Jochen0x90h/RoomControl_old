#pragma once

#include "String.hpp"
#include "Array.hpp"
#include "Action.hpp"


struct Device {
	enum class Type : uint8_t {
		/**
		 * Generic switch for light or power plug
		 */
		SWITCH,

		/**
		 * Light, same as switch, only for visualization
		 */
		LIGHT,

		/**
		 * Dimmer
		 */
		DIMMER,
		
		/**
		 * Window blind or roller shutter
		 */
		BLIND,

		/**
		 * Handle such as window or terrace door handle with multiple states
		 */
		HANDLE,
	};

	struct State {
		const char *name;
		uint8_t state;
	};
	
	struct Transition {
		uint8_t fromState;
		uint8_t toState;
	};

	// generic states
	static const uint8_t OFF = 0;
	static const uint8_t ON = 100;
	static const uint8_t CLOSE = 0;
	static const uint8_t CLOSED = 0;
	static const uint8_t OPEN = 100;
	static const uint8_t LOCKED = 101;

	// dimmer/blind states
	static const uint8_t LIGHTEN = 102;
	static const uint8_t DARKEN = 103;
	static const uint8_t DIM = 104;
	static const uint8_t STOP = 105;
	static const uint8_t RAISE = LIGHTEN;
	static const uint8_t LOWER = DARKEN;
	static const uint8_t MOVE = DIM;

	// window handle states
	static const uint8_t TILT = 50;
	static const uint8_t SHUT = 102; // closed or locked
	static const uint8_t UNSHUT = 103; // open or tilt
	static const uint8_t UNLOCKED = 104; // closed, open or tilt

	// heating states
	static const uint8_t NIGHT = 50;
	static const uint8_t ACTIVE = 101; // currently heating

	// placeholder for a value from 0 - 100
	static const uint8_t VALUE = 0xff;


	// unique id of the device, used by events, timers, scenarios and conditions
	uint8_t id;
	
	// type of actor
	Type type;

	// name
	char name[16];
	
	// configuration values
	uint16_t value1;
	uint16_t value2;

	// output, either local relay or enocean node id (input node id for handle)
	uint32_t output;

	/**
	 * Get all states of the device
	 */
	Array<State> getStates() const;

	/**
	 * Get all states that can be set by an action on a button or timer event
	 */
	Array<State> getActionStates() const;

	/**
	 * Get the name of the given state
	 */
	String getStateName(uint8_t state) const;

	/**
	 * Get possible state transitions of the device
	 */
	Array<Transition> getTransitions() const;

	// get transition speed for dimmer or blind
	int getSpeed() const {return this->value1;}

	// get delay time in milliseconds
	int getDelay() const {return this->value1;}
	
	// get timeout time in milliseconds
	int getTimeout() const {return this->value2;}

	// actions that execute on this device if conditions apply
	// note: must be last in struct!
	Actions actions;

	int byteSize() const {return offsetof(Device, actions) + this->actions.byteSize();}
};
