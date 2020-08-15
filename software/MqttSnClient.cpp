#include "MqttSnClient.hpp"
#include <cassert>

/*
static MqttSnClient::Udp6Endpoint const broadcast = {
	//{0xff, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01},
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01},
	0,
	MqttSnClient::GATEWAY_PORT
};
*/

MqttSnClient::MqttSnClient(UpLink::Parameters const &upParameters)
	: UpLink(upParameters), SystemTimer()
{
	// enable receiving of packets from gateway (on other side of up-link)
	upReceive(this->receiveMessage, MAX_MESSAGE_LENGTH);

	memset(this->sendBuffer, 0, SEND_BUFFER_SIZE);
}

MqttSnClient::~MqttSnClient() {

}

/*
MqttSnClient::Result MqttSnClient::searchGateway() {
	// allocate message
	Message m = allocateSendMessage(3, mqttsn::SEARCHGW);
	if (m.info == nullptr)
		return Result::BUSY;

	// serialize message
	m.data[2] = 1; // radius
	
	this->state = State::SEARCHING_GATEWAY;

	// before sending, wait for a jitter time to prevent flooding of gateway when multiple clients are switched on.
	// Therefore the random value must be different for each client (either hardware or client id dependent)
	//todo: use random time
	setSystemTimeout1(getSystemTime() + 100ms);
		
	return Result::OK;
}
*/

MqttSnClient::Result MqttSnClient::connect(/*Udp6Endpoint const &gateway, uint8_t gatewayId,*/ String clientName,
	bool cleanSession, bool willFlag)
{
	//if (gatewayId == 0)
	//	return Result::INVALID_PARAMETER;
    if (clientName.empty() || clientName.length > MAX_MESSAGE_LENGTH - 6)
		return Result::INVALID_PARAMETER;
	if (!canConnect())
		return Result::INVALID_STATE;

	//this->gateway = gateway;
	//this->gatewayId = gatewayId;

	// allocate message (always succeeds because message queue is empty)
	Message m = addSendMessage(6 + clientName.length, mqttsn::CONNECT);

	// set message id so that tetries are attempted
	m.info->msgId = 1;

	// serialize message
	m.data[2] = 0; // flags
	m.data[3] = 0x01; // protocol name/version
	mqttsn::setUShort(m.data + 4, toSeconds(KEEP_ALIVE_INTERVAL));
	memcpy(m.data + 6, clientName.data, clientName.length);

	// send message (isSendUpBusy() is guaranteed to be false)
	sendCurrentMessage();

	// update state
	this->state = State::CONNECTING;

	return Result::OK;
}

MqttSnClient::Result MqttSnClient::disconnect() {
    if (!isConnected() && !isAwake())
		return Result::INVALID_STATE;
	
	// allocate message
	Message m = addSendMessage(2, mqttsn::DISCONNECT);
	if (m.info == nullptr)
		return Result::BUSY;

	// send message
	if (!isUpSendBusy())
		sendCurrentMessage();

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

	// allocate message
	Message m = addSendMessage(2, mqttsn::PINGREQ);
	if (m.info == nullptr)
		return Result::BUSY;

	// send message
	if (!isUpSendBusy())
		sendCurrentMessage();

	return Result::OK;
}

MqttSnClient::MessageResult MqttSnClient::registerTopic(String topicName) {
    if (topicName.empty() || topicName.length > MAX_MESSAGE_LENGTH - 6)
		return {Result::INVALID_PARAMETER, 0};
    if (!isConnected())
		return {Result::INVALID_STATE, 0};
	
	// allocate message
	Message m = addSendMessage(6 + topicName.length, mqttsn::REGISTER);
	if (m.info == nullptr)
		return {Result::BUSY, 0};

	// generate and set message id
	uint16_t msgId = getNextMsgId();
	m.info->msgId = msgId;

	// serialize message
	mqttsn::setUShort(m.data + 2, 0);
	mqttsn::setUShort(m.data + 4, msgId);
	memcpy(m.data + 6, topicName.data, topicName.length);

	// send message
	if (!isUpSendBusy())
		sendCurrentMessage();

	return {Result::OK, m.info->msgId};
}

MqttSnClient::Result MqttSnClient::publish(uint16_t topicId, const uint8_t *data, int length, int8_t qos, bool retain)
{
    if (topicId == 0 || length > MAX_MESSAGE_LENGTH - 7)
		return Result::INVALID_PARAMETER;
    if (!isConnected())
		return Result::INVALID_STATE;
	
	// allocate message
	Message m = addSendMessage(7 + length, mqttsn::PUBLISH);
	if (m.info == nullptr)
		return Result::BUSY;

	// generate and set message id
	uint16_t msgId = getNextMsgId();
	if (qos > 0)
		m.info->msgId = msgId;

	// serialize message
	m.data[2] = mqttsn::makeQos(qos) | mqttsn::makeRetain(retain) | mqttsn::makeTopicType(mqttsn::TopicType::NORMAL);
	mqttsn::setUShort(m.data + 3, topicId);
	mqttsn::setUShort(m.data + 5, msgId);
	memcpy(m.data + 7, data, length);

	// send message
	if (!isUpSendBusy())
		sendCurrentMessage();

	return Result::OK;
}

MqttSnClient::MessageResult MqttSnClient::subscribeTopic(String topicFilter, int8_t qos) {
	if (topicFilter.empty() || topicFilter.length > MAX_MESSAGE_LENGTH - 5)
		return {Result::INVALID_PARAMETER, 0};
	if (!isConnected())
		return {Result::INVALID_STATE, 0};
	
	// allocate message
	Message m = addSendMessage(5 + topicFilter.length, mqttsn::SUBSCRIBE);
	if (m.info == nullptr)
		return {Result::BUSY, 0};

	// generate and set message id
	uint16_t msgId = getNextMsgId();
	m.info->msgId = msgId;

	// serialize message
	m.data[2] = mqttsn::makeQos(qos) | mqttsn::makeTopicType(mqttsn::TopicType::NORMAL);
	mqttsn::setUShort(m.data + 3, msgId);
	memcpy(m.data + 5, topicFilter.data, topicFilter.length);

	// send message
	if (!isUpSendBusy())
		sendCurrentMessage();

	return {Result::OK, msgId};
}

MqttSnClient::Result MqttSnClient::unsubscribeTopic(String topicFilter) {
	if (topicFilter.empty() || topicFilter.length > MAX_MESSAGE_LENGTH - 5)
		return Result::INVALID_PARAMETER;
	if (!isConnected())
		return Result::INVALID_STATE;

	// allocate message
	Message m = addSendMessage(5 + topicFilter.length, mqttsn::UNSUBSCRIBE);
	if (m.info == nullptr)
		return Result::BUSY;

	// generate and set message id
	uint16_t msgId = getNextMsgId();
	m.info->msgId = msgId;

	// serialize message
	m.data[2] = mqttsn::makeTopicType(mqttsn::TopicType::NORMAL);
	mqttsn::setUShort(m.data + 3, msgId);
	memcpy(m.data + 5, topicFilter.data, topicFilter.length);

	// send message
	if (!isUpSendBusy())
		sendCurrentMessage();

	return Result::OK;
}


// transport
// ---------

void MqttSnClient::onUpReceived(int length) {
	// check if length and message type are present
	if (length < 2) {
		onError(ERROR_MESSAGE);
		return;
	}
	uint8_t const *data = this->receiveMessage;

	// get message length
	int messageLength = data[0];
	
	// check for extended length (currently not used)
	/*if (messageLength == 1) {
		if (length < 4) {
			onError(ERROR_PACKET);
			return;
		}
		
		// get message length and remove the 2 extra bytes so that offsets conform to spec
		messageLength = getUShort(data + 1) - 2;
		data += 2;
		length -= 2;
	}*/
	
	// get message type
	mqttsn::MessageType messageType = mqttsn::MessageType(data[1]);

	// check if message has valid length and is complete
	if (messageLength < 2 || messageLength > length) {
		onError(ERROR_MESSAGE, messageType);
		return;
	}
	
	// overwrite length to prevent using trailing data
	length = messageLength;
		
	// handle message depending on state
	switch (this->state) {
	case State::STOPPED:
		break;
/*
	case State::SEARCHING_GATEWAY:
		switch (messageType) {
		case MQTTSN_ADVERTISE:
			// broadcasted periodically by a gateway to advertise its presence
			break;
		case MQTTSN_GWINFO:
			// sent by the gateway in response to our SEARCHGW request
			{
				// remove SEARCHGW message
				removeSentMessage(1);

				uint8_t gatewayId = data[typeOffset + 1];
				onGatewayFound(sender, gatewayId);
			}
			break;
		default:
			// error
			onUnsupportedError(messageType);
		}
		break;
		*/
	case State::CONNECTING:
		switch (messageType) {
		case mqttsn::CONNACK:
			// sent by the gateway in response to our CONNECT request
			if (length < 3) {
				onError(ERROR_MESSAGE, messageType);
			} else {
				// deserialize message
				mqttsn::ReturnCode returnCode = mqttsn::ReturnCode(data[2]);

				// remove CONNECT message
				removeSentMessage(1, mqttsn::CONNECT);

				switch (returnCode)
				{
				case mqttsn::ReturnCode::ACCEPTED:
					// set timer for idle ping to keep the connection alive
					setSystemTimeout1(getSystemTime() + KEEP_ALIVE_INTERVAL);

					// now we are connected to the gateway
					this->state = State::CONNECTED;
					onConnected();
					break;
				case mqttsn::ReturnCode::REJECTED_CONGESTED:
					// the gateway rejected the connection request due to congestion
					onError(ERROR_CONGESTED, messageType);
					break;
				default:
					;
				}
			}
			break;
		default:
			// error
			onError(ERROR_UNSUPPORTED, messageType);
		}
		break;
	default:
		// all other states such as CONNECTED
		switch (messageType) {
		case mqttsn::DISCONNECT:
			switch (this->state) {
			case State::WAITING_FOR_SLEEP:
				// sent by the gateway in response to a sleep request
				stopSystemTimeout1();
				this->state = State::ASLEEP;
				onSleep();
				break;
			default:
				// sent by the gateway in response to a disconnect request
				// or the gateway has dropped our connection for some reason
				stopSystemTimeout1();
				this->state = State::STOPPED;
				onDisconnected();
			}
			break;
		/*case mqttsn::REGISTER:
			// sent by the gateway to inform us about the topic id it has assigned to a topic name
			if (length < 7) {
				onError(ERROR_PACKET, messageType);
			} else {
				// deserialize message
				uint16_t topicId = mqttsn::getUShort(data + 2);
				uint16_t msgId = mqttsn::getUShort(data + 4);
				String topicName(data + 6, length - 6);

				// reply to gateway with regack
				Message m = addSendMessage(7, mqttsn::REGACK);
				if (m.info != nullptr) {
					// serialize message
					mqttsn::setUShort(m.data + 2, topicId);
					mqttsn::setUShort(m.data + 4, msgId);
					m.data[6] = uint8_t(mqttsn::ReturnCode::ACCEPTED);

					// send message
					if (!isSendUpBusy())
						sendCurrentMessage();
				} else {
					// error: send message queue full
				}
				
				// user callback
				onRegister(topicName, topicId);
			}
			break;*/
		case mqttsn::REGACK:
			// sent by the gateway in response to a REGISTER message
			if (length < 7) {
				onError(ERROR_MESSAGE, messageType);
			} else {
				// deserialize message
				uint16_t topicId = mqttsn::getUShort(data + 2);
				uint16_t msgId = mqttsn::getUShort(data + 4);
				mqttsn::ReturnCode returnCode = mqttsn::ReturnCode(data[6]);

				// remove REGISTER message
				uint8_t *data = removeSentMessage(msgId, mqttsn::REGISTER);
				if (data != nullptr) {
					switch (returnCode) {
					case mqttsn::ReturnCode::ACCEPTED:
						{
							int length = data[0];
							String topicName(data + 6, length - 6);
							onRegistered(msgId, topicName, topicId);
						}
						break;
					case mqttsn::ReturnCode::REJECTED_CONGESTED:
						onError(ERROR_CONGESTED, messageType);
						break;
					default:
						;
					}
				}
			}
			break;
		case mqttsn::PUBLISH:
			// sent by the gateway to publish a message on a topic we have subscribed
			if (length < 7) {
				onError(ERROR_MESSAGE, messageType);
			} else {
				// deserialize message
				uint8_t flags = data[2];
				bool dup = mqttsn::getDup(flags);
				int8_t qos = mqttsn::getQos(flags);
				bool retain = mqttsn::getRetain(flags);
				mqttsn::TopicType topicType = mqttsn::getTopicType(flags);
				uint16_t topicId = mqttsn::getUShort(data + 3);
				uint16_t msgId = mqttsn::getUShort(data + 5);
				uint8_t const *payload = data + 7;
				int payloadLength = length - 7;

				if (qos <= 0 || !isBusy()) {
					// user callback
					mqttsn::ReturnCode returnCode = mqttsn::ReturnCode::REJECTED_INVALID_TOPIC_ID;
					if (topicType == mqttsn::TopicType::NORMAL)
						returnCode = onPublish(topicId, payload, payloadLength, qos, retain);

					if (qos > 0) {
						// reply to gateway with puback
						Message m = addSendMessage(7, mqttsn::PUBACK);

						// serialize message
						mqttsn::setUShort(m.data + 2, topicId);
						mqttsn::setUShort(m.data + 4, msgId);
						m.data[6] = uint8_t(returnCode);

						// send message
						if (!isUpSendBusy())
							sendCurrentMessage();
					}
				} else {
					// error: send message queue full
				}
			}
			break;
		case mqttsn::PUBACK:
			// sent by the gateway in response to a PUBLISH message (if qos was greater zero)
			if (length < 7) {
				onError(ERROR_MESSAGE, messageType);
			} else {
				// deserialize message
				uint16_t topicId = mqttsn::getUShort(data + 2);
				uint16_t msgId = mqttsn::getUShort(data + 4);
				mqttsn::ReturnCode returnCode = mqttsn::ReturnCode(data[6]);
				
				// remove PUBLISH message from queue
				if (removeSentMessage(msgId, mqttsn::PUBLISH) != nullptr) {
					switch (returnCode) {
					case mqttsn::ReturnCode::ACCEPTED:
						break;
					case mqttsn::ReturnCode::REJECTED_CONGESTED:
						onError(ERROR_CONGESTED, messageType);
						break;
					default:
						;
					}
				}
			}
			break;
		case mqttsn::SUBACK:
			// sent by the gateway in response to a SUBSCRIBE message
			if (length < 8) {
				onError(ERROR_MESSAGE, messageType);
			} else {
				// deserialize message
				uint8_t flags = data[2];
				int8_t qos = mqttsn::getQos(flags);
				uint16_t topicId = mqttsn::getUShort(data + 3);
				uint16_t msgId = mqttsn::getUShort(data + 5);
				mqttsn::ReturnCode returnCode = mqttsn::ReturnCode(data[7]);

				// remove SUBSCRIBE message from queue
				uint8_t *data = removeSentMessage(msgId, mqttsn::SUBSCRIBE);
				if (data != nullptr) {
					switch (returnCode) {
					case mqttsn::ReturnCode::ACCEPTED:
						{
							int length = data[0];
							String topicName(data + 5, length - 5);
							onSubscribed(msgId, topicName, qos, topicId);
						}
						break;
					case mqttsn::ReturnCode::REJECTED_CONGESTED:
						onError(ERROR_CONGESTED, messageType);
						break;
					default:
						;
					}
				}
			}
			break;
		case mqttsn::UNSUBACK:
			// sent by the gateway in response to a UNSUBSCRIBE message
			if (length < 4) {
				onError(ERROR_MESSAGE, messageType);
			} else {
				// deserialize message
				uint16_t msgId = mqttsn::getUShort(data + 2);

				// remove UNSUBSCRIBE message from queue
				removeSentMessage(msgId, mqttsn::UNSUBSCRIBE);
			}
			break;
		case mqttsn::PINGRESP:
			// sent by the gateway in response to a PINGREQ message ("I am alive") and to indicate that it has no more
			// buffered data to a sleeping client
			break;
	/*
		case mqttsn::WILLTOPICREQ:
			break;
		case mqttsn::WILLMSGREQ:
			break;
		case mqttsn::WILLTOPICRESP:
			break;
		case mqttsn::WILLMSGRESP:
			break;
	*/
		default:
			// error: unsupported message type
			onError(ERROR_UNSUPPORTED, messageType);
		}
	}
	
	// re-enable receiving of packets
	upReceive(this->receiveMessage, MAX_MESSAGE_LENGTH);
}

void MqttSnClient::onUpSent() {
	if (this->resendPending) {
		// resend a message that was not acknowledged
		this->resendPending = false;
		resend();
	} else {
		// check if there are more messages to send in the queue
		if (this->sendMessagesHead != this->sendMessagesCurrent)
			sendCurrentMessage();
	}
}

void MqttSnClient::onSystemTimeout1(SystemTime time) {
	
	switch (this->state) {
	case State::STOPPED:
		break;
	/*case State::SEARCHING_GATEWAY:
		// try again (SEARCHGW message)
		resend();
		break;*/
	case State::CONNECTING:
		// try again (CONNECT message)
		resend();
		break;
	case State::CONNECTED:
		if (this->sendMessagesTail < this->sendMessagesCurrent) {
			// resend a message that was not acknowledged or mark as pending
			if (!(this->resendPending = isUpSendBusy()))
				resend();
		} else {
			// idle ping
			ping();
		}
		break;
	default:
		;
	}
}

// internal
// --------

MqttSnClient::Message MqttSnClient::addSendMessage(int length, mqttsn::MessageType type) {
	if (this->busy)
		return {};

	// garbage collect old messages
	if (this->sendMessagesHead + 1 + 1 > MAX_SEND_COUNT
		|| this->sendBufferHead + length + MAX_MESSAGE_LENGTH > SEND_BUFFER_SIZE)
	{
		int j = 0;
		uint16_t offset = 0;
		for (int i = this->sendMessagesTail; i < this->sendMessagesHead; ++i) {
			MessageInfo *info = &this->sendMessages[i];
			if (info->msgId != 0 || i >= this->sendMessagesCurrent) {
				uint8_t *data = this->sendBuffer + info->offset;
				int length = data[0];
				
				MessageInfo *info2 = &this->sendMessages[j++];
				info2->msgId = info->msgId;
				info2->offset = offset;
				memcpy(this->sendBuffer + offset, data, length);
				offset += length;
			}

		}
		this->sendMessagesTail = 0;
		this->sendMessagesCurrent = j - (this->sendMessagesHead - this->sendMessagesCurrent);
		this->sendMessagesHead = j;
		this->sendBufferHead = offset;
		this->sendBufferFill = offset;
	}

	// allocate message info
	MessageInfo *info = &this->sendMessages[this->sendMessagesHead++];

	// allocate data
	info->offset = this->sendBufferHead;
	this->sendBufferHead += length;
	this->sendBufferFill += length;
	uint8_t *data = this->sendBuffer + info->offset;

	// set message header
	data[0] = length;
	data[1] = type;

	// clear message id
	info->msgId = 0;

	// set busy flag if no new message will fit
	this->busy = this->sendMessagesHead >= MAX_SEND_COUNT || this->sendBufferFill + MAX_MESSAGE_LENGTH > SEND_BUFFER_SIZE;

	return {info, data};
}

void MqttSnClient::sendCurrentMessage() {
	int current = this->sendMessagesCurrent;

	// get message
	MessageInfo *info = &this->sendMessages[current];
	uint8_t *data = this->sendBuffer + info->offset;
	int length = data[0];
	
	// send message to gateway
	upSend(data, length);

	// set sent time
	info->sentTime = getSystemTime();

	if (current == this->sendMessagesTail) {
		if (info->msgId == 0) {
			// remove message from tail because it's not needed any more such as PINGREQ
			this->sendMessagesTail = current + 1;
		
			// set timer for idle ping to keep the connection alive
			setSystemTimeout1(info->sentTime + KEEP_ALIVE_INTERVAL);
		} else {
			// start retransmission timeout
			setSystemTimeout1(info->sentTime + RETRANSMISSION_INTERVAL);
		}
	}

	// make next message current
	this->sendMessagesCurrent = current + 1;
}

void MqttSnClient::resend() {
	// get message
	MessageInfo *info = &this->sendMessages[this->sendMessagesTail];

	// get message data
	uint8_t *data = this->sendBuffer + info->offset;
	int length = data[0];
	
	// send to gateway
	upSend(data, length);

	// start retransmission timeout
	setSystemTimeout1(getSystemTime() + RETRANSMISSION_INTERVAL);
}

MqttSnClient::Message MqttSnClient::getSentMessage(uint16_t msgId) {
	// mark message with msgId as obsolete
	for (int i = this->sendMessagesTail; i < this->sendMessagesCurrent; ++i) {
		MessageInfo *info = &this->sendMessages[i];
		
		if (info->msgId == msgId)
			return {info, &this->sendBuffer[info->offset]};
	}
	return {};
}

uint8_t *MqttSnClient::removeSentMessage(uint16_t msgId, mqttsn::MessageType type) {
	uint8_t *data = nullptr;
	int tail = this->sendMessagesTail;
	
	// mark message with msgId as obsolete
	for (int i = tail; i < this->sendMessagesCurrent; ++i) {
		MessageInfo *info = &this->sendMessages[i];
		
		if (info->msgId == msgId) {
			uint8_t *d = this->sendBuffer + info->offset;
			if (d[1] == type) {
				data = d;
				info->msgId = 0;
				int length = d[0];
				this->sendBufferFill -= length;
				this->busy = this->sendBufferFill + MAX_MESSAGE_LENGTH > SEND_BUFFER_SIZE;
				break;
			}
		}
	}
	
	// remove acknowledged messages and messages that don't need acknowledge
	for (int i = tail; i < this->sendMessagesCurrent; ++i) {
		MessageInfo *info = &this->sendMessages[i];
	
		if (info->msgId == 0)
			++tail;
		else
			break;
	}
	this->sendMessagesTail = tail;
	
	SystemTime systemTime = getSystemTime();
	if (tail < this->sendMessagesCurrent) {
		// get next message
		MessageInfo *info = &this->sendMessages[tail];
		
		// check if we have to resend it immediately
		if (systemTime >= info->sentTime + RETRANSMISSION_INTERVAL) {
			// resend a message that was not acknowledged or mark as pending
			if (!(this->resendPending = isUpSendBusy()))
				resend();
		} else {
			// wait until next message has to be resent
			setSystemTimeout1(info->sentTime + RETRANSMISSION_INTERVAL);
		}
	} else {
		// set timer for idle ping to keep the connection alive
		setSystemTimeout1(systemTime + KEEP_ALIVE_INTERVAL);
	}
	
	return data;
}
