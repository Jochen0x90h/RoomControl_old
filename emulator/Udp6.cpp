#include "Udp6.hpp"
#include <iostream>


Udp6::Udp6(Context &context, uint16_t port)
	: socket(context, asio::ip::udp::endpoint(asio::ip::udp::v6(), port))
{
}

Udp6::~Udp6() {
}

void Udp6::receive(Endpoint &sender, uint8_t *data, int length) {
	std::cout << "receive" << std::endl;
	this->socket.async_receive_from(boost::asio::buffer(data, length), this->sender,
		[this, &sender, data] (const boost::system::error_code& error, std::size_t size) {
			std::cout << "received " << size << " bytes from " << this->sender << std::endl;

			sender.address = this->sender.address().to_v6().to_bytes();
			sender.scope = this->sender.address().to_v6().scope_id();
			sender.port = this->sender.port();
			onReceive(sender, data, int(size));
		}
	);
}

void Udp6::send(const Endpoint &receiver, const uint8_t *data, int length) {
	asio::ip::udp::endpoint endpoint(asio::ip::address_v6(receiver.address, receiver.scope), receiver.port);
	this->socket.async_send_to(boost::asio::buffer(data, length), endpoint,
		[this, data, length] (const boost::system::error_code &error, std::size_t size) {
			//std::cout << "sent " << size << " bytes " << error.message() << std::endl;
			
			onSent(data, length);
		}
	);
}
