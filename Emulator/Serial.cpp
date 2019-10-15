#include "Serial.hpp"
#include <boost/asio/write.hpp>

using spb = asio::serial_port_base;
using boost::system::error_code;


Serial::Serial(Device device) : tty(context) {
	this->tty.open(device);

	// set 115200 8N1
	this->tty.set_option(spb::baud_rate(57600));
	this->tty.set_option(spb::character_size(8));
	this->tty.set_option(spb::parity(spb::parity::none));
	this->tty.set_option(spb::stop_bits(spb::stop_bits::one));
	this->tty.set_option(spb::flow_control(spb::flow_control::none));
}

void Serial::send(const uint8_t *data, int length) {
	this->sentCount = 0;
	asio::async_write(
		this->tty,
		asio::buffer(data, length),
		[this] (error_code error, size_t writtenCount) {
			if (error) {
				// error
				this->sentCount = -1;
			} else {
				// ok
				this->sentCount = writtenCount;
			}
		});
}

void Serial::receive(uint8_t *data, int length) {
	this->receivedCount = 0;
	this->tty.async_read_some(
		asio::buffer(data, length),
		[this] (error_code error, size_t readCount) {
			if (error) {
				// error
				this->receivedCount = -1;
			} else {
				// ok
				this->receivedCount = readCount;
			}
		});
}
