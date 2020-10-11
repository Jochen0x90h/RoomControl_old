#include "UpLink.hpp"
#include <iostream>


UpLink::UpLink()
	: emulatorSocket(global::context, global::local)
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
	this->emulatorSocket.async_receive_from(boost::asio::buffer(data, length), this->sender,
		[this] (const boost::system::error_code& error, std::size_t receivedLength) {
			std::cout << "received " << receivedLength << " bytes from " << this->sender << std::endl;

			onUpReceived(int(receivedLength));
		}
	);
}

void UpLink::upSend(const uint8_t *data, int length) {
	assert(length <= sizeof(this->sendBuffer));
	memcpy(this->sendBuffer, data, length);
	this->sendBusy = true;
	this->emulatorSocket.async_send_to(boost::asio::buffer(this->sendBuffer, length), global::upLink,
		[this] (const boost::system::error_code &error, std::size_t sentLength) {
			//std::cout << "sent " << sentLength << " bytes " << error.message() << std::endl;
			
			this->sendBusy = false;
			onUpSent();
		}
	);
}
