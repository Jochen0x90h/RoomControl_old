#include "Udp6.hpp"
#include <iostream>


Udp6::~Udp6() {
}

void Udp6::receiveUdp6(Udp6Endpoint &sender, uint8_t *data, int length) {
	std::cout << "receive" << std::endl;
	this->emulatorSocket.async_receive_from(boost::asio::buffer(data, length), this->emulatorSender,
		[this, &sender] (const boost::system::error_code& error, std::size_t receivedLength) {
			std::cout << "received " << receivedLength << " bytes from " << this->emulatorSender << std::endl;

			sender.address = this->emulatorSender.address().to_v6().to_bytes();
			sender.scope = this->emulatorSender.address().to_v6().scope_id();
			sender.port = this->emulatorSender.port();
			onReceivedUdp6(int(receivedLength));
		}
	);
}

void Udp6::sendUdp6(const Udp6Endpoint &receiver, const uint8_t *data, int length) {
	asio::ip::udp::endpoint endpoint(asio::ip::address_v6(receiver.address, receiver.scope), receiver.port);
	this->emulatorSocket.async_send_to(boost::asio::buffer(data, length), endpoint,
		[this] (const boost::system::error_code &error, std::size_t sentLength) {
			//std::cout << "sent " << sentLength << " bytes " << error.message() << std::endl;
			
			onSentUdp6();
		}
	);
}
