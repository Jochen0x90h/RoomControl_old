#pragma once

#include <boost/asio/io_service.hpp>
#include <boost/asio/serial_port.hpp>


namespace asio = boost::asio;

class Serial {
public:
	using Device = const std::string&;
	
	Serial(Device device);

	void send(const uint8_t *data, int length);
	
	// return number of bytes sent, -1 on error
	int getSentCount() {
		return this->sentCount;
	}
	
	void receive(uint8_t *data, int length);
	
	// return number of bytes received, -1 on error
	int getReceivedCount() {
		return this->receivedCount;
	}
	
	void update() {
		this->context.poll();
		if (this->context.stopped())
			this->context.reset();
	}

protected:

	asio::io_service context;
	asio::serial_port tty;
	
	int sentCount = 0;
	int receivedCount = 0;
};
