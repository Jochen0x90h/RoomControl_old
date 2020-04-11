#include "MqttSnClient.hpp"
#include "MQTTSNPacket.h"
#include <cassert>


static MqttSnClient::Udp6Endpoint const broadcast = {
	//{0xff, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01},
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01},
	0,
	MqttSnClient::GATEWAY_PORT
};


MqttSnClient::MqttSnClient()
	: Udp6(CLIENT_PORT), SystemTimer()
{
	// enable receiving of packets
	receiveUdp6(this->sender, this->receivePacket, sizeof(this->receivePacket));
}

MqttSnClient::~MqttSnClient() {

}

MqttSnClient::Result MqttSnClient::searchGateway() {
	uint8_t radius = 1;
	this->sendLength = MQTTSNSerialize_searchgw(this->sendPacket, PACKET_MAX_LENGTH, radius);
	if (this->sendLength == 0)
		return Result::INVALID_PARAMETER;

	this->state = State::SEARCHING_GATEWAY;

	// before sending, wait for a jitter time to prevent flooding of gateway when multiple clients are switched on.
	// Therefore the random value must be different for each client (either hardware or client id dependent)
	//todo: use random time
	setSystemTimeout(getSystemTime() + 100ms);
		
	return Result::OK;
}

MqttSnClient::Result MqttSnClient::connect(Udp6Endpoint const &gateway, uint8_t gatewayId, String clientId,
	bool cleanSession, bool willFlag)
{
	// check client state and options
	if (gatewayId == 0 || clientId.empty())
		return Result::INVALID_PARAMETER;
	if (!canConnect())
		return Result::INVALID_STATE;
	if (isBusy())
		return Result::BUSY;

	this->gateway = gateway;
	this->gatewayId = gatewayId;

	// connect options
	MQTTSNPacket_connectData o = MQTTSNPacket_connectData_initializer;
	o.clientID.lenstring.data = const_cast<char*>(clientId.data);
	o.clientID.lenstring.len = clientId.length;
	o.duration = toSeconds(KEEP_ALIVE_INTERVAL);
	o.cleansession = cleanSession;
	o.willFlag = willFlag;

	// serialize connect packet
	this->sendLength = MQTTSNSerialize_connect(this->sendPacket, PACKET_MAX_LENGTH, &o);
	if (this->sendLength == 0)
		return Result::INVALID_PARAMETER;

	// stop timer to prevent onTimeout() being called while we are busy (before onSent())
	//stop();

	// send packet (retransmission timeout is set in onSent())
	this->busy = true;
	sendUdp6(this->gateway, this->sendPacket, this->sendLength);

	// update state
	this->state = State::CONNECTING;

	return Result::OK;
}

MqttSnClient::Result MqttSnClient::disconnect() {
    if (!isConnected() && !isAwake())
		return Result::INVALID_STATE;
	if (isBusy())
		return Result::BUSY;

	// serialize disconnect packet
	this->sendLength = MQTTSNSerialize_disconnect(this->sendPacket, PACKET_MAX_LENGTH, -1);
	if (this->sendLength == 0)
		return Result::INVALID_PARAMETER;

	// send packet (retransmission timeout is set in onSent())
	this->busy = true;
	sendUdp6(this->gateway, this->sendPacket, this->sendLength);

	// update state
	if (isConnected())
		this->state = State::DISCONNECTING;
	else
		this->state = State::STOPPED;

	return Result::OK;
}

MqttSnClient::Result MqttSnClient::ping() {
    if (!isConnected())
		return Result::INVALID_STATE;
	if (isBusy())
		return Result::BUSY;

	// serialize packet
	this->sendLength = MQTTSNSerialize_pingreq(this->sendPacket, PACKET_MAX_LENGTH, {});

	// send packet (retransmission timeout is set in onSent())
	this->busy = true;
	sendUdp6(this->gateway, this->sendPacket, this->sendLength);

	return Result::OK;
}

MqttSnClient::Result MqttSnClient::registerTopic(String topic) {
    if (topic.empty())
		return Result::INVALID_PARAMETER;
    if (!isConnected())
		return Result::INVALID_STATE;
	if (isBusy())
		return Result::BUSY;

	// topic
	MQTTSNString t{nullptr, {topic.length, const_cast<char*>(topic.data)}};

	// serialize packet
	this->sendLength = MQTTSNSerialize_register(this->sendPacket, PACKET_MAX_LENGTH, 0, nextPacketId(), &t);

	// send packet (retransmission timeout is set in onSent())
	this->busy = true;
	sendUdp6(this->gateway, this->sendPacket, this->sendLength);

	return Result::OK;
}

MqttSnClient::Result MqttSnClient::publish(uint16_t topicId, const uint8_t *data, uint16_t length, bool retained,
	int8_t qos)
{
    if (topicId == 0 || data == nullptr || length == 0)
		return Result::INVALID_PARAMETER;
    if (!isConnected())
		return Result::INVALID_STATE;
	if (isBusy())
		return Result::BUSY;

	uint8_t dup = 0;

	// topic
	MQTTSN_topicid topic;
	memset(&topic, 0, sizeof(MQTTSN_topicid));
	topic.type = MQTTSN_TOPIC_TYPE_NORMAL;
	topic.data.id = topicId;
	
	// serialize packet
	this->sendLength = MQTTSNSerialize_publish(this->sendPacket, PACKET_MAX_LENGTH, dup, qos, retained ? 1 : 0,
		nextPacketId(), topic, (uint8_t*)data, length);

	// send packet (retransmission timeout is set in onSent())
	this->busy = true;
	sendUdp6(this->gateway, this->sendPacket, this->sendLength);

	return Result::OK;
}

MqttSnClient::Result MqttSnClient::subscribeTopic(String topic) {
	if (topic.empty())
		return Result::INVALID_PARAMETER;
	if (!isConnected())
		return Result::INVALID_STATE;
	if (isBusy())
		return Result::BUSY;

	uint8_t qos = 1;
	uint8_t dup = 0;

	// topic
	MQTTSN_topicid t;
	memset(&t, 0, sizeof(MQTTSN_topicid));
	t.type = MQTTSN_TOPIC_TYPE_NORMAL;
	t.data.long_.name = const_cast<char*>(topic.data);
	t.data.long_.len = topic.length;

	// serialize packet
	this->sendLength = MQTTSNSerialize_subscribe(this->sendPacket, PACKET_MAX_LENGTH, dup, qos, nextPacketId(), &t);

	// send packet (retransmission timeout is set in onSent())
	this->busy = true;
	sendUdp6(this->gateway, this->sendPacket, this->sendLength);

	return Result::OK;
}

MqttSnClient::Result MqttSnClient::unsubscribeTopic(String topic) {
	if (topic.empty())
		return Result::INVALID_PARAMETER;
	if (!isConnected())
		return Result::INVALID_STATE;
	if (isBusy())
		return Result::BUSY;

	// topic
	MQTTSN_topicid t;
	memset(&t, 0, sizeof(MQTTSN_topicid));
	t.type = MQTTSN_TOPIC_TYPE_NORMAL;
	t.data.long_.name = const_cast<char*>(topic.data);
	t.data.long_.len  = topic.length;

	// serialize packet
	this->sendLength = MQTTSNSerialize_unsubscribe(this->sendPacket, PACKET_MAX_LENGTH, nextPacketId(), &t);
	
	// send packet (retransmission timeout is set in onSent())
	this->busy = true;
	sendUdp6(this->gateway, this->sendPacket, this->sendLength);

	return Result::OK;
}

MqttSnClient::Result MqttSnClient::regAck(uint16_t topicId, uint16_t packetId, uint8_t returnCode) {
	//todo
	return Result::OK;
}

void MqttSnClient::requestNext() {
	if (this->state != State::CONNECTED)
		return;
		
	if (!this->busy)
		setSystemTimeout(doNext(this->keepAliveTime));
	else
		this->timeoutPending = true;
}

void MqttSnClient::onReceivedUdp6(int length) {
	if (!this->busy)
		processPacket(this->sender, this->receivePacket, length);
	else
		this->receiveLength = length;
}

void MqttSnClient::onSentUdp6() {
	this->busy = false;

	State state = this->state;
	switch (state) {
	case State::STOPPED:
		break;
	case State::SEARCHING_GATEWAY:
	case State::CONNECTING:
		// set timer for retransmission if this connect request fails
		setSystemTimeout(getSystemTime() + RETRANSMISSION_INTERVAL);
		break;
	default:
		;
	}

	// process a pending received packet
	if (this->receiveLength > 0) {
		processPacket(this->sender, this->receivePacket, this->receiveLength);
		this->receiveLength = 0;
	}
	
	// process a pending timeout
	if (!this->busy && this->timeoutPending) {
		processTimeout(getSystemTime());
		this->timeoutPending = false;
	}
/*
	// check if nothing more to do
	if (!this->busy && state == State::CONNECTED) {
		// set timer for next event (user event or keep alive)
		set(doNext(this->keepAliveTime));
	}
*/
}

void MqttSnClient::onSystemTimeout(SystemTime time) {
	if (!this->busy) {
		processTimeout(time);
	} else {
		this->timeoutPending = true;
	}
}

void MqttSnClient::processTimeout(SystemTime time) {
	switch (this->state) {
	case State::STOPPED:
		break;
	case State::SEARCHING_GATEWAY:
		// try again (CONNECT packet)
		sendUdp6(broadcast, this->sendPacket, this->sendLength);
		break;
	case State::CONNECTING:
		// try again (SEARCHGW packet)
		sendUdp6(this->gateway, this->sendPacket, this->sendLength);
		break;
	case State::CONNECTED:
		if (time >= this->keepAliveTime) {
			// send keep alive message to gateway
			this->keepAliveTime = getSystemTime() + RETRANSMISSION_INTERVAL;
			ping();
			
			// another timeout for doNext() after ping has been sent
			this->timeoutPending = true;
		} else {
			setSystemTimeout(doNext(this->keepAliveTime));
		}
	default:
		;
	}
}

void MqttSnClient::processPacket(Udp6Endpoint const &sender, uint8_t const *data, int length) {
	// check length of packet
	int packetLength;
	int typeOffset = MQTTSNPacket_decode(const_cast<uint8_t*>(data), length, &packetLength);
	if (typeOffset == MQTTSNPACKET_READ_ERROR || length < packetLength) {
		// error
		onPacketError();
		return;
	}
	
	// get message type
	MQTTSN_msgTypes messageType = MQTTSN_msgTypes(data[typeOffset]);
	
	// handle message depending on state
	switch (this->state) {
	case State::STOPPED:
		break;
	case State::SEARCHING_GATEWAY:
		switch (messageType) {
		case MQTTSN_ADVERTISE:
			// broadcasted periodically by a gateway to advertise its presence
			break;
		case MQTTSN_GWINFO:
			// sent by the gateway in response to our SEARCHGW request
			if (this->state == State::SEARCHING_GATEWAY) {
				uint8_t gatewayId = data[typeOffset + 1];
				onGatewayFound(sender, gatewayId);
			}
			break;
		default:
			// error
			onUnsupportedError(messageType);
		}
		break;
	case State::CONNECTING:
		switch (messageType) {
		case MQTTSN_CONNACK:
			// sent by the gateway in response to our CONNECT request
			{
				int returnCode;
				if (MQTTSNDeserialize_connack(&returnCode, const_cast<uint8_t*>(data), length) == 0) {
					onPacketError();
				} else {
					switch (returnCode)
					{
					case MQTTSN_RC_ACCEPTED:
						// set keep alive timer
						this->keepAliveTime = getSystemTime() + KEEP_ALIVE_INTERVAL;
						setSystemTimeout(this->keepAliveTime);

						// now we are connected to the gateway
						this->state = State::CONNECTED;
						onConnected();
						break;
					case MQTTSN_RC_REJECTED_CONGESTED:
						// the gateway rejected the connection request due to congestion
						onCongestedError(messageType);
						break;
					}
				}
			}
			break;
		default:
			// error
			onUnsupportedError(messageType);
		}
		break;
	default:
		// all other satates such as CONNECTED
		this->keepAliveTime = getSystemTime() + KEEP_ALIVE_INTERVAL;
		switch (messageType) {
		case MQTTSN_DISCONNECT:
			// sent by the gateway in response to our disconnect request
			switch (this->state) {
			case State::DISCONNECTING:
				stopSystemTimeout();
				this->state = State::STOPPED;
				onDisconnected();
				break;
			case State::WAITING_FOR_SLEEP:
				stopSystemTimeout();
				this->state = State::ASLEEP;
				onSleep();
				break;
			default:
				// error: received disconnect message in wrong state
				;
			}
			break;
		case MQTTSN_REGISTER:
			// sent by the gateway to inform us about the topic id it has assigned to a topic name
			{
				uint16_t topicId;
				uint16_t packetId;
				MQTTSNString topic;
				if (MQTTSNDeserialize_register(&topicId, &packetId, &topic, const_cast<uint8_t*>(data), length) == 0) {
					onPacketError();
				} else {
					regAck(topicId, packetId, MQTTSN_RC_ACCEPTED);
					
					onRegister(topic.cstring, topicId);
				}
			}
			break;
		case MQTTSN_REGACK:
			// sent by the gateway in response to a REGISTER message
			{
				uint16_t topicId;
				uint16_t packetId;
				uint8_t returnCode;
				if (MQTTSNDeserialize_regack(&topicId, &packetId, &returnCode, const_cast<uint8_t*>(data), length) == 0) {
					onPacketError();
				} else {
					switch (returnCode) {
					case MQTTSN_RC_ACCEPTED:
						onRegistered(packetId, topicId);
						break;
					case MQTTSN_RC_REJECTED_CONGESTED:
						onCongestedError(messageType);
						break;
					}
				}
			}
			break;
		case MQTTSN_PUBLISH:
			// sent by the gateway to publish a message on a topic we have subscribed
			{
				uint8_t dup;
				int qos;
				uint8_t retained;
				uint16_t packetId;
				MQTTSN_topicid topic;
				uint8_t *payload;
				int payloadLength;
				if (MQTTSNDeserialize_publish(&dup, &qos, &retained, &packetId, &topic, &payload, &payloadLength,
					const_cast<uint8_t*>(data), length) == 0)
				{
					onPacketError();
				} else {
					if (qos > 0) {
						// send puback
						//todo, maybe user has to do it
					}
					onPublish(topic.data.id, payload, payloadLength, retained != 0, qos);
				}
			}
			break;
		case MQTTSN_PUBACK:
			// sent by the gateway in response to a PUBLISH message
			{
				uint16_t topicId;
				uint16_t packetId;
				uint8_t returnCode;

				if (MQTTSNDeserialize_puback(&topicId, &packetId, &returnCode, const_cast<uint8_t*>(data), length) == 0) {
					onPacketError();
				} else {
					switch (returnCode) {
					case MQTTSN_RC_ACCEPTED:
						onPublished(packetId, topicId);
						break;
					case MQTTSN_RC_REJECTED_CONGESTED:
						onCongestedError(messageType);
						break;
					}
				}
			}
			break;
		case MQTTSN_SUBACK:
			// sent by the gateway in response to a SUBSCRIBE message
			{
				int qos;
				uint16_t topicId;
				uint16_t packetId;
				uint8_t returnCode;
				if (MQTTSNDeserialize_suback(&qos, &topicId, &packetId, &returnCode,
					const_cast<uint8_t*>(data), length) == 0)
				{
					onPacketError();
				} else {
					switch (returnCode) {
					case MQTTSN_RC_ACCEPTED:
						onSubscribed(packetId, topicId);
						break;
					case MQTTSN_RC_REJECTED_CONGESTED:
						onCongestedError(messageType);
						break;
					}
				}
			}
			break;
		case MQTTSN_UNSUBACK:
			// sent by the gateway in response to a UNSUBSCRIBE message
			{
				uint16_t packetId = 0;
				if (MQTTSNDeserialize_unsuback(&packetId, const_cast<uint8_t*>(data), length) == 0) {
					onPacketError();
				} else {
					onUnsubscribed();
				}
			}
			break;
		case MQTTSN_PINGRESP:
			// sent by the gateway in response to a PINGREQ message ("I am alive") and to indicate that no more buffered
			// data to a sleeping client
			if (MQTTSNDeserialize_pingresp(const_cast<uint8_t*>(data), length) == 0) {
				onPacketError();
			} else {
			}
			break;
	/*
		case MQTTSN_WILLTOPICREQ:
			break;
		case MQTTSN_WILLMSGREQ:
			break;
		case MQTTSN_WILLTOPICRESP:
			break;
		case MQTTSN_WILLMSGRESP:
			break;
	*/
		default:
			// error
			onUnsupportedError(messageType);
		}
	}
	
	// re-enable receiving of packets
	receiveUdp6(this->sender, this->receivePacket, sizeof(this->receivePacket));
}

uint16_t MqttSnClient::nextPacketId() {
	return this->packetId = this->packetId == 0xffff ? 1 : this->packetId + 1;
}
