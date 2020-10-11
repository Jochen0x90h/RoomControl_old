#include "DownLink.hpp"
#include <iostream>


DownLink::~DownLink() {
}

//void DownLink::disconnectDown(uint16_t node) {
//	this->endpoints.right.erase(node);
//}

void DownLink::downReceive(uint8_t *data, int length) {
	std::cout << "down receive" << std::endl;
	this->emulatorSocket.async_receive_from(boost::asio::buffer(data, length), this->sender,
		[this] (const boost::system::error_code& error, std::size_t receivedLength) {
			std::cout << "received " << receivedLength << " bytes from " << this->sender << std::endl;

			auto p = this->endpoints.left.insert({sender, this->next});
			if (p.second)
				++this->next;

			onDownReceived(p.first->second, int(receivedLength));
		}
	);
}

void DownLink::downSend(uint16_t clientId, const uint8_t *data, int length) {
	assert(length <= sizeof(this->sendBuffer));
	memcpy(this->sendBuffer, data, length);
	this->sendBusy = true;
	asio::ip::udp::endpoint endpoint = this->endpoints.right.at(clientId);
	this->emulatorSocket.async_send_to(boost::asio::buffer(this->sendBuffer, length), endpoint,
		[this] (const boost::system::error_code &error, std::size_t sentLength) {
			//std::cout << "sent " << sentLength << " bytes " << error.message() << std::endl;
			
			this->sendBusy = false;
			onDownSent();
		}
	);
}
