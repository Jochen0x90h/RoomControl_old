#pragma once

#include "Context.hpp"


/**
 * IPv6 UDP transport, implementation based on asio for linux/macOS
 */
class Udp6 {
public:

	Udp6(Context &context, uint16_t port);

	virtual ~Udp6();

	// IP address and port of an IPv6 endpoint
	struct Endpoint
	{
		Address address;
		uint16_t scope;
		uint16_t port;
	};

	/**
	 * Setup for receiving data. When a packet arrives, onReceive() gets called
	 * @param sender variable for sender of the packet
	 * @param data space for received packet data
	 * @param length maximum packet length
	 */
	void receive(Endpoint &sender, uint8_t *data, int length);

	/**
	 * Called when a packet arrived
	 * @param sender sender of the packet
	 * @param data packet data
	 * @param length packet length
	 */
	virtual void onReceive(const Endpoint &sender, const uint8_t *data, int length) = 0;

	/**
	 * Send a packet to a receiver. When finished, onSent() gets called
	 * @param receiver receiver of the packet
	 * @param data packet data
	 * @param length packet length
	 */
	void send(const Endpoint &receiver, const uint8_t *data, int length);

	/**
	 * Called when send operation has finished
	 */
	virtual void onSent(const uint8_t *data, int length) = 0;

protected:
	
	asio::ip::udp::socket socket;

	// cache for endpoint of sender when receiving data
	asio::ip::udp::endpoint sender;
};
