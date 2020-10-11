#pragma once

#include <boost/bimap.hpp>
#include "global.hpp"


/**
 * Down-link in a spanning tree of nodes used for wireless communication
 * Emulator implementation based on asio for linux/macOS
*/
class DownLink {
public:

	/**
	 * Constructor
	 * @param parameters platform dependent parameters
	 */
	DownLink()
		: emulatorSocket(global::context, global::downLocal)
	{}

	virtual ~DownLink();

	/**
	 * Setup for receiving data. When a message arrives, onReceivedDown() gets called
	 * @param data space for received message data
	 * @param length maximum message length
	 */
	void downReceive(uint8_t *data, int length);

	/**
	 * Called when a message was received
	 * @param clientId sender of the message
	 * @param length message length
	 */
	virtual void onDownReceived(uint16_t clientId, int length) = 0;

	/**
	 * Send a message to a receiver. When finished, onSentDown() gets called
	 * @param clientId receiver of the message
	 * @param data message data gets copied and can be discarded immediately after sendUp() returns
	 * @param length message length
	 */
	void downSend(uint16_t clientId, const uint8_t *data, int length);

	/**
	 * Called when send operation has finished
	 */
	virtual void onDownSent() = 0;

	/**
	 * Returns true when sending is in progress
	 */
	bool isDownSendBusy() {return this->sendBusy;}

private:
	
	asio::ip::udp::socket emulatorSocket;
	
	// list of downlinks
	uint16_t next = 1;
	boost::bimap<asio::ip::udp::endpoint, uint16_t> endpoints;

	asio::ip::udp::endpoint sender;
	uint8_t sendBuffer[256];
	bool sendBusy = false;
};
