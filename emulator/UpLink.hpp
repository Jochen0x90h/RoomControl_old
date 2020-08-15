#pragma once

#include "global.hpp"


/**
 * Up-link in a spanning tree of nodes used for wireless communication
 * Emulator implementation based on asio for linux/macOS
 */
class UpLink {
public:

	// platform dependent parameters
	struct Parameters {
		// local endpoint (e.g. asio::ip::udp::endpoint(asio::ip::udp::v6(), port))
		asio::ip::udp::endpoint local;

		// remote endpoint of uplink
		asio::ip::udp::endpoint remote;
	};

	/**
	 * Constructor
	 * @param parameters platform dependent parameters
	 */
	UpLink(Parameters parameters);

	virtual ~UpLink();

	/**
	 * Notify that a the up-link has been established
	 */
	virtual void onUpConnected() = 0;

	/**
	 * Setup for receiving data. When a message arrives, onReceivedUp() gets called
	 * @param data space for received message data
	 * @param length maximum message length
	 */
	void upReceive(uint8_t *data, int length);

	/**
	 * Called when a message was received
	 * @param length message length
	 */
	virtual void onUpReceived(int length) = 0;

	/**
	 * Send a message to a receiver. When finished, onSentUp() gets called
	 * @param data message data gets copied and can be discarded immediately after sendUp() returns
	 * @param length message length
	 */
	void upSend(const uint8_t *data, int length);

	/**
	 * Called when send operation has finished
	 */
	virtual void onUpSent() = 0;

	/**
	 * Returns true when sending is in progress
	 */
	bool isUpSendBusy() {return this->sendBusy;}
	
private:
	
	asio::ip::udp::socket emulatorSocket;
	asio::ip::udp::endpoint remote;
	uint8_t sendBuffer[256];
	bool sendBusy = false;
};
