#include "MqttSnBroker.hpp"

#include <iostream>
#include <fstream>
#include <vector>
#include <thread>
#include <iterator>


class MyBroker : public MqttSnBroker {
public:

	MyBroker(UpLink::Parameters const &upParameters, DownLink::Parameters const &downParameters)
		: MqttSnBroker(upParameters, downParameters)
	{
	}


// UpLink
// ------
	
	void onUpConnected() override {
		connect("myClient");
	}

	
// MqttSnClient
// ------------

	void onConnected() override{
		std::cout << "connected to gateway" << std::endl;

		this->foo = registerTopic("foo").topicId;
		this->bar = registerTopic("bar").topicId;
		//setSystemTimeout3(getSystemTime() + 1s);
	
		subscribeTopic("foo", 1);
		subscribeTopic("bar", 1);
	}
	
	void onDisconnected() override{
	
	}
	
	void onSleep() override {
	
	}
	
	void onWakeup() override {
	
	}

	void onError(int error, mqttsn::MessageType messageType) override {
	
	}


// MqttSnBroker
// ------------
	
	void onPublished(uint16_t topicId, uint8_t const *data, int length, int8_t qos, bool retain) override {
		std::string s((char const*)data, length);
		std::cout << "published " << topicId << " " << s << std::endl;
	}


// SystemTimer
// -----------
	
	void onSystemTimeout3(SystemTime time) override {
		std::cout << "publish" << std::endl;

		publish(this->foo, "foo", 1);
		publish(this->bar, "bar", 1);

		setSystemTimeout3(getSystemTime() + 1s);		
	}


	// topics
	uint16_t foo;
	uint16_t bar;
};

/**
 * start mqtt broker (e.g. mosquitto)
 * configure OS, INCLUDE and LIB in makefile and compile: https://github.com/eclipse/paho.mqtt-sn.embedded-c/tree/master/MQTTSNGateway
 * start gateway
 * start testBroker
 * publish on command line: mqtt pub --topic foo --message testMessage
 * subscribe on command line: mqtt sub --topic foo --topic bar
 */
int main(int argc, const char **argv) {
	boost::system::error_code ec;
	asio::ip::address localhost = asio::ip::address::from_string("::1", ec);

	UpLink::Parameters upParameters;
	upParameters.local = asio::ip::udp::endpoint(asio::ip::udp::v6(), 1337);
	upParameters.remote = asio::ip::udp::endpoint(localhost, 47193);

	DownLink::Parameters downParameters;
	downParameters.local = asio::ip::udp::endpoint(asio::ip::udp::v6(), 1338);

	MyBroker broker(upParameters, downParameters);
	
	// run event loop
	global::context.run();

	return 0;
}
