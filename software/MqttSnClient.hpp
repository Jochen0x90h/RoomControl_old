#pragma once

#include "Udp6.hpp"
#include "SystemTimer.hpp"
#include "String.hpp"
#include "MQTTSNPacket.h"


/**
 * MQTT-SN client inspired by C implementation by Nordic Semiconductor
 * http://www.mqtt.org/new/wp-content/uploads/2009/06/MQTT-SN_spec_v1.2.pdf
 */
class MqttSnClient : public Udp6, public SystemTimer {
public:

	// Port the MQTT-SN client binds to
	static constexpr int CLIENT_PORT = 47194;

	// Port of the gateway
	static constexpr int GATEWAY_PORT = 47193;

	// Default retransmission time
	static constexpr SystemDuration RETRANSMISSION_INTERVAL = 8s;

	// The MQTT broker disconnects us if we don't send anything in one and a half times the keep alive interval
	static constexpr SystemDuration KEEP_ALIVE_INTERVAL = 60s;

	// Default time in seconds for which MQTT-SN Client is considered alive by the MQTT-SN Gateway
	//static constexpr int DEFAULT_ALIVE_DURATION = 60;

	// Default time in seconds for which MQTT-SN Client is considered asleep by the MQTT-SN Gateway
	//static constexpr int DEFAULT_SLEEP_DURATION = 30;

	// Maximum length of Client ID according to the protocol spec in bytes
	static constexpr int CLIENT_ID_MAX_LENGTH = 23;

	// Maximum length of a packet
	static constexpr int PACKET_MAX_LENGTH = 64;

	// Maximum length of will feature buffers. For internal use only
	static constexpr int WILL_TOPIC_MAX_LENGTH = 32;
	static constexpr int WILL_MSG_MAX_LENGTH = 32;

	enum class Result : uint8_t
	{
		OK,
		INVALID_PARAMETER,
		INVALID_STATE,
		BUSY
	};

	enum class State : uint8_t
	{
		STOPPED,
		SEARCHING_GATEWAY,
		CONNECTING,
		CONNECTED,
		DISCONNECTING,
		ASLEEP,
		AWAKE,
		WAITING_FOR_SLEEP
	};
	
	struct Topic
	{
		const char *topicName;
		uint16_t topicId;
	};
	

	MqttSnClient();

	~MqttSnClient() override;

	/**
	 * Get state of the client
	 */
	State getState() {return this->state;}
	bool isStopped() {return this->state == State::STOPPED;}
	bool isSearchingGateway() {return this->state == State::SEARCHING_GATEWAY;}
	bool isConnecting() {return this->state == State::CONNECTING;}
	bool isConnected() {return this->state == State::CONNECTED;}
	bool isAsleep() {return this->state == State::ASLEEP;}
	bool isAwake() {return this->state == State::AWAKE;}
	//bool isDisconnected() {return this->state == State::SEARCH_GATEWAY;}
	bool canConnect() {return isSearchingGateway() || isAsleep() || isAwake();}

	/**
	 * Returns true if transport is busy sending a packet. Guaranteed to be false in the "on..." user callbacks
	 */
	bool isBusy() {return this->busy;}


	// interface to gateway
	// --------------------

	/**
	 * Start searching for a gateway. When a gateway is found, onGatewayFound() gets called
	 */
	Result searchGateway();
	
	/**
	 * Connect to a gateway, e.g. in onGatewayFound() when an advertisement of a gateway has arrived
	 * @param gateway gateway ip address, scope and port
	 * @param gatewayId id of gateway
	 * @param clientId unique client ID
	 * @param cleanSession clean session flag
	 * @param willFlag if set, the gateway will request the will topic and will message
	 */
	Result connect(Udp6Endpoint const &gateway, uint8_t gatewayId, String clientId, bool cleanSession = false,
		bool willFlag = false);
	
	/**
	 * Disconnect from gateway
	 */
	Result disconnect();
	
	//Result sleep();

	/**
	 * Send a ping to the gateway
	 */
	Result ping();

	/**
	 * Register a topic at the gateway. On success, onRegistered() gets called with the topicId to use in publish()
	 */
	Result registerTopic(String topic);

	/**
	 * Publish a message on a topic
	 * @param topicId id obtained using registerTopic()
	 * @param payload data
	 * @param payload data length
	 * @param retained retained mode
	 * @param qos quality of service: 0, 1, 2 or 3
	 */
	Result publish(uint16_t topicId, uint8_t const *data, uint16_t length, bool retained = false, int8_t qos = 1);

	Result publish(uint16_t topicId, String data, bool retained = false, int8_t qos = 1) {
		return publish(topicId, reinterpret_cast<uint8_t const*>(data.data), data.length, retained, qos);
	}

	/**
	 * Subscribe to a topic. On success, onSubscribed() is called with a topicId that is used in subsequent onPublish() calls
	 */
	Result subscribeTopic(String topic);

	/**
	 * Unsubscribe from a topic. On success, onUnsubscribed() is called
	 */
	Result unsubscribeTopic(String topic);

	//Result updateWillTopic();
	//Result updateWillMessage();
	
protected:

	Result regAck(uint16_t topicId, uint16_t packetId, uint8_t returnCode);

	
	// user callbacks (isBusy() is guaranteed to be false in a callback)
	// -----------------------------------------------------------------
		
	// Client has found a gateway
	virtual void onGatewayFound(Udp6Endpoint const &sender, uint8_t gatewayId) = 0;

	// Client has connected successfully
	virtual void onConnected() = 0;
	
	// Client was disconnected by the gateway
	virtual void onDisconnected() = 0;
	
	// Client is allowed to sleep
	virtual void onSleep() = 0;
	
	// Client should wake up
	virtual void onWakeup() = 0;	
			
	// Client has received a topic to register (wildcard topic only)
	virtual void onRegister(const char* topic, uint16_t topicId) = 0;

	// Client has registered a topic
	virtual void onRegistered(uint16_t packetId, uint16_t topicId) = 0;

	// Gateway publishes a message on a subscribed topic
	virtual void onPublish(uint16_t topicId, uint8_t const *data, int length, bool retained, int8_t qos) = 0;

	// Client has published a message successfully
	virtual void onPublished(uint16_t packetId, uint16_t topicId) = 0;

	// Client has subscribed successfully
	virtual void onSubscribed(uint16_t packetId, uint16_t topicId) = 0;

	// Client has unsubscribed successfully
	virtual void onUnsubscribed() = 0;
	
	// Client has updated the will topic successfully
	//virtual void onWillTopicUpdated() = 0;
	
	// Client has updated the will message successfully
	//virtual void onWillMessageUpdated() = 0;

	/**
	 * Invalid packet has been received
	 */
	virtual void onPacketError() = 0;

	/**
	 * Gateway rejected request due to congestion
	 * @param messageType type of message for which the error occurred
	 */
	virtual void onCongestedError(MQTTSN_msgTypes messageType) = 0;

	/**
	 * Message type is unsupported
	 * @param messageType type of message for which the error occurred
	 */
	virtual void onUnsupportedError(MQTTSN_msgTypes messageType) = 0;
	
public:

	// user activity
	// -------------

	/**
	 * When the user wants to do an action (e.g. publish()) the client may be busy. Therefore request that doNext() is called as soon as possible
	 */
	void requestNext();

	/**
	 * The user can do the next action (e.g. publish()) and indicate the time when more actions are intended
	 * @return time when doNext() should be called again
	 */
	virtual SystemTime doNext(SystemTime time) = 0;

protected:

	// transport
	// ---------

	void onReceivedUdp6(int length) override;

	void onSentUdp6() override;

	void onSystemTimeout(SystemTime time) override;
    

	// internal
	// --------
	
	void processPacket(const Udp6Endpoint &sender, const uint8_t *data, int length);
	void processTimeout(SystemTime time);
	
	uint16_t nextPacketId();

	// state
	State state = State::STOPPED;
		
	// gateway
	uint8_t gatewayId;
	Udp6Endpoint gateway;

	// id for next packet
	uint16_t packetId = 0;
	
	// cache for packet to send
	uint8_t sendPacket[PACKET_MAX_LENGTH];
	int sendLength;

	// transport is busy sending a packet
	bool busy = false;

	// cache for received packet
	Udp6Endpoint sender;
	uint8_t receivePacket[PACKET_MAX_LENGTH];
	int receiveLength = 0;
	
	// next time for keep alive message to the gateway
	SystemTime keepAliveTime;
	bool timeoutPending = false;
};
