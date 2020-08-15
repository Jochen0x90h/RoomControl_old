#include "UpLink.hpp"
#include <iostream>


UpLink::UpLink(Parameters parameters)
	: emulatorSocket(global::context, parameters.local), remote(parameters.remote)
{
	// post onConnectedUp() into event loop
	boost::asio::post(global::context, [this] {
		onUpConnected();
	});
}

UpLink::~UpLink() {
}

void UpLink::upReceive(uint8_t *data, int length) {
	std::cout << "receive" << std::endl;
	asio::ip::udp::endpoint sender;
	this->emulatorSocket.async_receive_from(boost::asio::buffer(data, length), sender,
		[this, &sender] (const boost::system::error_code& error, std::size_t receivedLength) {
			std::cout << "received " << receivedLength << " bytes from " << sender << std::endl;

			onUpReceived(int(receivedLength));
		}
	);
}

void UpLink::upSend(const uint8_t *data, int length) {
	assert(length <= sizeof(this->sendBuffer));
	memcpy(this->sendBuffer, data, length);
	this->sendBusy = true;
	this->emulatorSocket.async_send_to(boost::asio::buffer(this->sendBuffer, length), this->remote,
		[this] (const boost::system::error_code &error, std::size_t sentLength) {
			//std::cout << "sent " << sentLength << " bytes " << error.message() << std::endl;
			
			this->sendBusy = false;
			onUpSent();
		}
	);
}
