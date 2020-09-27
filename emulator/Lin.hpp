#pragma once

#include "global.hpp"
#include "Array.hpp"
#include "Gui.hpp"


/**
 * Up-link in a spanning tree of nodes used for wireless communication
 * Emulator implementation uses virtual devices on user interface
 */
class Lin {
public:

	// platform dependent parameters
	struct Parameters {
		Gui &gui;
	};

	// LIN device
	struct Device {
		enum Type {
			/**
			 * 2 channel switch with actuators
			 * Flags for channel A/X: Bit 0: Switches, Bits 1-2: No relay, 1 relay, 2 relays, 2 interlocked relays for blind
			 * Flags for channel B:/Y Bit 4: Switches, Bits 5-6: No relay, 1 relay, 2 relays, 2 interlocked relays for blind
			 */
			SWITCH2 = 1
		};
		
		// 32 bit unique id
		uint32_t id;
		
		// device type
		Type type;
		
		// flags
		uint8_t flags;
	};

	/**
	 * Constructor
	 * @param parameters platform dependent parameters
	 */
	Lin(Parameters parameters);

	virtual ~Lin();

	/**
	 * Notify that the lin bus is ready
	 */
	virtual void onLinReady() = 0;

	/**
	 * Get list of devices
	 */
	Array<Device> getLinDevices();

	/**
	 * Called when a message was received
	 * @param deviceId sender sender of the message
	 * @param data message data
	 * @param length message length
	 */
	virtual void onLinReceived(uint32_t deviceId, uint8_t const *data, int length) = 0;

	/**
	 * Send a message to a lin device
	 * @param deviceId receiver of the message
	 * @param data message data
	 * @param length message length, maximum is 8
	 */
	void linSend(uint32_t deviceId, uint8_t const *data, int length);
	
	/**
	 * Called when send operation has finished
	 */
	virtual void onLinSent() = 0;

	/**
	 * Returns true when sending is in progress
	 */
	bool isLinSendBusy() {return this->sendBusy;}

	// only for emulator
	void doGui(int &id);
	
private:
	
	Gui &gui;
	
	uint8_t states[32];

	bool sendBusy = false;

	uint8_t receiveData[8];
};
