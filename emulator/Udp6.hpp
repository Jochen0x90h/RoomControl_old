#pragma once

#include "global.hpp"


/**
 * IPv6 UDP transport, implementation based on asio for linux/macOS
 */
class Udp6 {
public:

	// IPv6 UDP address
	using Udp6Address = asio::ip::address_v6::bytes_type;

	// IP address and port of an IPv6 endpoint
	struct Udp6Endpoint
	{
		Udp6Address address;
		uint16_t scope;
		uint16_t port;
	};


	/**
	 * UDP/IPv6
	 * @param port port to bind to
	 */
	Udp6(uint16_t port)
		: emulatorSocket(global::context, asio::ip::udp::endpoint(asio::ip::udp::v6(), port))
	{}

	virtual ~Udp6();


	/**
	 * Setup for receiving data. When a packet arrives, onReceive() gets called
	 * @param sender variable for sender of the packet
	 * @param data space for received packet data
	 * @param length maximum packet length
	 */
	void receiveUdp6(Udp6Endpoint &sender, uint8_t *data, int length);

	/**
	 * Called when a packet was received
	 * @param length packet length
	 */
	virtual void onReceivedUdp6(int length) = 0;

	/**
	 * Send a packet to a receiver. When finished, onSent() gets called
	 * @param receiver receiver of the packet
	 * @param data packet data
	 * @param length packet length
	 */
	void sendUdp6(const Udp6Endpoint &receiver, const uint8_t *data, int length);

	/**
	 * Called when send operation has finished
	 */
	virtual void onSentUdp6() = 0;

protected:
	
	asio::ip::udp::socket emulatorSocket;

	// cache for endpoint of sender when receiving data
	asio::ip::udp::endpoint emulatorSender;
};
