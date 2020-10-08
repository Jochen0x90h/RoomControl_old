#include "MqttSnBroker.hpp"

// min for qos (quality of service)
constexpr int8_t min(int8_t x, int8_t y) {return x < y ? x : y;}


MqttSnBroker::MqttSnBroker(UpLink::Parameters const &upParameters, DownLink::Parameters const &downParameters)
	: MqttSnClient(upParameters), DownLink(downParameters)
{
	// init clients
	for (int i = 0; i < MAX_CLIENT_COUNT; ++i) {
		ClientInfo &client = this->clients[i];
		client.clientId = 0;
		client.index = i;
	}
	
	// init topics
	for (int i = 0; i < MAX_TOPIC_COUNT; ++i) {
		TopicInfo &topic = this->topics[i];
		topic.hash = 0;
	}

	memset(this->sendBuffer, 0, SEND_BUFFER_SIZE);
}


MqttSnBroker::TopicResult MqttSnBroker::registerTopic(String topicName) {
	// get or add topic by name
	uint16_t topicId = getTopicId(topicName);
	Result result = Result::OK;
	if (topicId != 0) {
		TopicInfo &topic = getTopic(topicId);
		
		// check if we need to register the topic at the gateway
		if (topic.gatewayTopicId == 0)
			result = MqttSnClient::registerTopic(topicName).result;
	}
	return {topicId, result};
}

MqttSnBroker::Result MqttSnBroker::publish(uint16_t topicId, uint8_t const *data, int length, int8_t qos, bool retain) {
	if (isClientBusy() || isBrokerBusy())
		return Result::BUSY;

	TopicInfo *topic = findTopic(topicId);
	if (topic == nullptr || length > MAX_MESSAGE_LENGTH - 7)
		return Result::INVALID_PARAMETER;

	// publish to all clients and local client
	sendPublish(topicId, topic, data, length, qos, retain);

	// publish to gateway
	if (topic->gatewayTopicId != 0)
		MqttSnClient::publish(topic->gatewayTopicId, data, length, qos, retain);

	return Result::OK;
}

MqttSnBroker::TopicResult MqttSnBroker::subscribeTopic(String topicFilter, int8_t qos) {
	// get or add topic by name
	uint16_t topicId = getTopicId(topicFilter);
	Result result = Result::OK;
	if (topicId != 0) {
		TopicInfo &topic = getTopic(topicId);
		
		// check if we need to subscribe topic at the gateway
		if (qos > topic.getMaxQos())
			result = MqttSnClient::subscribeTopic(topicFilter, qos).result;

		if (result == Result::OK) {
			//todo
			// check if there is a retained message for this topic
			
			// set subscription quality of service
			topic.setQos(LOCAL_CLIENT_INDEX, qos);
		}
	}
	return {topicId, result};
}


// MqttSnClient user callbacks
// ---------------------------

void MqttSnBroker::onRegistered(uint16_t msgId, String topicName, uint16_t topicId) {
	uint16_t localTopicId = getTopicId(topicName);
	std::cout << topicName << " -> " << localTopicId << " " << topicId << std::endl;
	if (localTopicId != 0) {
		TopicInfo &topic = getTopic(localTopicId);
		topic.gatewayTopicId = topicId;
	}
}

void MqttSnBroker::onSubscribed(uint16_t msgId, String topicName, uint16_t topicId, int8_t qos) {
	uint16_t localTopicId = getTopicId(topicName);
	std::cout << topicName << " -> " << localTopicId << " " << topicId << std::endl;
	if (localTopicId != 0) {
		TopicInfo &topic = getTopic(localTopicId);
		topic.gatewayTopicId = topicId;
		topic.gatewayQos = qos;
	}
}

mqttsn::ReturnCode MqttSnBroker::onPublish(uint16_t topicId, uint8_t const *data, int length, int8_t qos, bool retain) {
	// check if we are able to forward to our clients
	if (isBrokerBusy())
		return mqttsn::ReturnCode::REJECTED_CONGESTED;

	uint16_t localTopicId = findTopicId(topicId);
	TopicInfo *topic = findTopic(localTopicId);
	if (topic == nullptr)
		return mqttsn::ReturnCode::REJECTED_INVALID_TOPIC_ID;

	// publish to all clients and local client
	sendPublish(topicId, topic, data, length, qos, retain);
}

// transport
// ---------

void MqttSnBroker::onDownReceived(uint16_t clientId, int length) {
	// check if length and message type are present
	if (length < 2) {
		onError(ERROR_MESSAGE);
		return;
	}
	uint8_t const *data = this->receiveMessage;

	// get message length
	int messageLength = data[0];
		
	// get message type
	mqttsn::MessageType messageType = mqttsn::MessageType(data[1]);

	// check if message has valid length and fits into buffer
	if (messageLength < 2 || messageLength > length) {
		onError(ERROR_MESSAGE, messageType);
		return;
	}
	
	// overwrite length to prevent using trailing data
	length = messageLength;

	switch (messageType) {
	case mqttsn::CONNECT:
		// a new client connects
		if (length < 7) {
			onError(ERROR_MESSAGE, messageType);
		} else {
			// create reply message
			Message m = addSendMessage(3, mqttsn::CONNACK);
			if (m.info != nullptr) {
				m.info->clients[0] = clientId;
				
				// add new client
				ClientInfo *client = findClient(0);
				if (client != nullptr) {
					uint8_t flags = data[2];
					uint8_t protocolId = data[3];
					uint16_t duration = mqttsn::getUShort(data + 4);
					String clientName(data + 6, length - 6);
					client->name << clientName;

					m.data[2] = uint8_t(mqttsn::ReturnCode::ACCEPTED);
				} else {
					// error: too many clients
					m.data[2] = uint8_t(mqttsn::ReturnCode::REJECTED_CONGESTED);
				}

				// send message
				if (!isDownSendBusy())
					sendCurrentMessage();
			} else {
				// error: send message queue full
			}
		}
		break;
	case mqttsn::DISCONNECT:
		// a client disconnects or wants to go asleep
		{
			// create reply message
			Message m = addSendMessage(2, mqttsn::DISCONNECT);
			if (m.info != nullptr) {
				m.info->clients[0] = clientId;

				// find client
				ClientInfo *client = findClient(clientId);
				if (client != nullptr) {
					uint16_t duration = 0;
					if (length >= 4)
						duration = mqttsn::getUShort(data + 2);

					client->disconnect();
					
					// clear subscriptions of this client from all topics
					for (int i = 0; i < this->topicCount; ++i) {
						this->topics[i].clearQos(client->index);
					}
				}
				
				// send reply message
				if (!isDownSendBusy())
					sendCurrentMessage();
			} else {
				// error: send message queue full
			}
		}
		break;
	case mqttsn::REGISTER:
		// client wants to register a topic so that it can publish to it
		if (length < 7) {
			onError(ERROR_MESSAGE, messageType);
		} else {
			// get client
			ClientInfo *client = findClient(clientId);
			if (client != nullptr) {
				// deserialize message
				uint16_t msgId = mqttsn::getUShort(data + 4);
				String topicName(data + 6, length - 6);

				// create reply message
				Message m = addSendMessage(7, mqttsn::REGACK);
				if (m.info != nullptr) {
					m.info->clients[0] = clientId;
					mqttsn::setUShort(m.data + 4, msgId);

					// get or add topic by name
					uint16_t topicId = getTopicId(topicName);
					if (topicId != 0) {
						TopicInfo &topic = getTopic(topicId);
						
						// check if we need to register the topic at gateway
						Result result = Result::OK;
						if (topic.gatewayTopicId == 0)
							result = MqttSnClient::registerTopic(topicName).result;
						
						if (result == Result::OK) {
							// serialize reply message
							mqttsn::setUShort(m.data + 2, topicId);
							m.data[6] = uint8_t(mqttsn::ReturnCode::ACCEPTED);
						} else {
							// error: could not register at gateway
							mqttsn::setUShort(m.data + 2, topicId);
							m.data[6] = uint8_t(mqttsn::ReturnCode::REJECTED_CONGESTED);
						}
					} else {
						// error: could not allocate a topic id
						mqttsn::setUShort(m.data + 2, 0);
						m.data[6] = uint8_t(mqttsn::ReturnCode::REJECTED_CONGESTED);
					}

					// send reply message
					if (!isDownSendBusy())
						sendCurrentMessage();
				} else {
					// error: send message queue full
				}
			} else {
				// error: client is not connected, send DISCONNECT
				sendDisconnect(clientId);
			}
		}
		break;
	case mqttsn::PUBLISH:
		// client wants to publish a message on a topic
		if (length < 7) {
			onError(ERROR_MESSAGE, messageType);
		} else {
			// get client
			ClientInfo *client = findClient(clientId);
			if (client != nullptr) {
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

				// create reply message
				Message m = addSendMessage(7, mqttsn::PUBACK);
				if (m.info != nullptr) {
					m.info->clients[0] = clientId;
					mqttsn::setUShort(m.data + 2, topicId);
					mqttsn::setUShort(m.data + 4, msgId);

					if (topicType == mqttsn::TopicType::NORMAL) {
						// find topic by id
						TopicInfo *topic = findTopic(topicId);
						if (topic != nullptr) {
							
							if (!isClientBusy() && !isBrokerBusy()) {
								// publish to clients except sender and local client
								sendPublish(topicId, topic, data, length, qos, retain, clientId);

								// publish to gateway
								if (topic->gatewayTopicId != 0)
									MqttSnClient::publish(topic->gatewayTopicId, data, length, min(qos, topic->gatewayQos), retain);

								// serialize reply message
								m.data[6] = uint8_t(mqttsn::ReturnCode::ACCEPTED);
							} else {
								// error: uplink or downlink congested
								m.data[6] = uint8_t(mqttsn::ReturnCode::REJECTED_CONGESTED);
							}
						} else {
							// error: topic not found
							m.data[6] = uint8_t(mqttsn::ReturnCode::REJECTED_INVALID_TOPIC_ID);
						}
					} else {
						// error: topic type not supported
						m.data[6] = uint8_t(mqttsn::ReturnCode::NOT_SUPPORTED);
					}

					// send reply message
					if (!isDownSendBusy())
						sendCurrentMessage();
				} else {
					// error: send message queue full
				}
			} else {
				// error: client is not connected, send DISCONNECT
				sendDisconnect(clientId);
			}
		}
		break;
	case mqttsn::PUBACK:
		// sent by the client in response to a PUBLISH message (if qos was greater zero)
		if (length < 7) {
			onError(ERROR_MESSAGE, messageType);
		} else {
			// get client
			ClientInfo *client = findClient(clientId);
			if (client != nullptr) {

				// deserialize message
				uint16_t topicId = mqttsn::getUShort(data + 2);
				uint16_t msgId = mqttsn::getUShort(data + 4);
				mqttsn::ReturnCode returnCode = mqttsn::ReturnCode(data[6]);
				
				// mark this client as done and remove PUBLISH message from queue if all are done
				removeSentMessage(msgId, client->index);

				switch (returnCode) {
				case mqttsn::ReturnCode::ACCEPTED:
					break;
				case mqttsn::ReturnCode::REJECTED_CONGESTED:
					onError(ERROR_CONGESTED, messageType);
					break;
				default:
					;
				}
			} else {
				// error: client is not connected, send DISCONNECT
				sendDisconnect(clientId);
			}
		}
		break;
	case mqttsn::SUBSCRIBE:
		// client wants to subscribe to a topic or topic wildcard
		if (length < 6) {
			onError(ERROR_MESSAGE, messageType);
		} else {
			// get client
			ClientInfo *client = findClient(clientId);
			if (client != nullptr) {
				// deserialize message
				uint8_t flags = data[2];
				bool dup = mqttsn::getDup(flags);
				int8_t qos = mqttsn::getQos(flags);
				mqttsn::TopicType topicType = mqttsn::getTopicType(flags);
				uint16_t msgId = mqttsn::getUShort(data + 3);

				// create reply message
				Message m = addSendMessage(8, mqttsn::SUBACK);
				if (m.info != nullptr) {
					m.info->clients[0] = clientId;
					m.data[2] = mqttsn::makeQos(qos); // granted qos
					mqttsn::setUShort(m.data + 5, msgId);

					if (topicType == mqttsn::TopicType::NORMAL) {
						String topicName(data + 5, length - 5);

						// get or add topic by name
						uint16_t topicId = getTopicId(topicName);
						if (topicId != 0) {
							TopicInfo &topic = getTopic(topicId);
							
							// check if we need to subscribe topic at the gateway
							Result result = Result::OK;
							if (qos > topic.getMaxQos())
								result = MqttSnClient::subscribeTopic(topicName, qos).result;
							
							if (result == Result::OK) {
								//todo
								// check if there is a retained message for this topic
								
								// add subscription of client to flags
								topic.setQos(client->index, qos);
								
								// serialize reply message
								mqttsn::setUShort(m.data + 3, topicId);
								m.data[7] = uint8_t(mqttsn::ReturnCode::ACCEPTED);
							} else {
								// error: could not subscribe at the gateway
								mqttsn::setUShort(m.data + 3, topicId);
								m.data[7] = uint8_t(mqttsn::ReturnCode::REJECTED_CONGESTED);
							}
						} else {
							// error: could not allocate a topic id
							mqttsn::setUShort(m.data + 3, 0);
							m.data[7] = uint8_t(mqttsn::ReturnCode::REJECTED_CONGESTED);
						}
					} else {
						// error: topic type not supported
						mqttsn::setUShort(m.data + 3, 0);
						m.data[7] = uint8_t(mqttsn::ReturnCode::NOT_SUPPORTED);
					}
					
					// send reply message
					if (!isDownSendBusy())
						sendCurrentMessage();
				} else {
					// error: send message queue full
				}
			} else {
				// error: client is not connected, send DISCONNECT
				sendDisconnect(clientId);
			}
		}
		break;
	case mqttsn::UNSUBSCRIBE:
		// client wants to unsubscribe from a topic or topic wildcard
		if (length < 6) {
			onError(ERROR_MESSAGE, messageType);
		} else {
			// get client
			ClientInfo *client = findClient(clientId);
			if (client != nullptr) {
				// deserialize message
				uint8_t flags = data[2];
				mqttsn::TopicType topicType = mqttsn::getTopicType(flags);
				uint16_t msgId = mqttsn::getUShort(data + 3);

				// create reply message
				Message m = addSendMessage(4, mqttsn::UNSUBACK);
				if (m.info != nullptr) {
					m.info->clients[0] = clientId;
					mqttsn::setUShort(m.data + 2, msgId);

					if (topicType == mqttsn::TopicType::NORMAL) {
						String topicName(data + 5, length - 5);

						//todo: check if topicFilter is a topic name or wildcard

						// get topic from topic name
						uint16_t topicId = getTopicId(topicName);
						if (topicId != 0) {
							TopicInfo &topic = getTopic(topicId);

							// remove subscription of client from flags
							int8_t qos = topic.clearQos(client->index);
						
							// check if maximum qos was reduced
							int8_t maxQos = topic.getMaxQos();
							if (maxQos < qos) {
								if (maxQos == -1) {
									// all subscriptions removed: unsubscribe at the gateway
									MqttSnClient::unsubscribeTopic(topicName);
									topic.gatewayTopicId = 0;
									topic.gatewayQos = -1;
								} else {
									// subscribe at the gateway with reduced qos
									MqttSnClient::subscribeTopic(topicName, qos);
								}
							}
						} else {
							// error: topic was new but could not be allocated which we can ignore
						}
					} else {
						// error: topic type not supported
					}

					// send reply message
					if (!isDownSendBusy())
						sendCurrentMessage();
				} else {
					// error: send message queue full
				}
			} else {
				// error: client is not connected
				sendDisconnect(clientId);
			}
		}
		break;
	case mqttsn::PINGREQ:
		// client sends a ping to keep connection alive
		{
			// get client
			ClientInfo *client = findClient(clientId);
			if (client != nullptr) {
				//todo: reset connection timeout
			
				// create reply message
				Message m = addSendMessage(2, mqttsn::PINGRESP);
				if (m.info != nullptr) {
					m.info->clients[0] = clientId;

					// send reply message
					if (!isDownSendBusy())
						sendCurrentMessage();
				} else {
					// error: send message queue full
				}
			} else {
				// error: client is not connected
				sendDisconnect(clientId);
			}
		}
		break;
	default:
		// error: unsupported message type
		onError(ERROR_BROKER_UNSUPPORTED, messageType);
	}

	// re-enable receiving of packets
	downReceive(this->receiveMessage, MAX_MESSAGE_LENGTH);
}

void MqttSnBroker::onDownSent() {
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

void MqttSnBroker::onSystemTimeout2(SystemTime time) {
	if (this->sendMessagesTail < this->sendMessagesCurrent) {
		// resend a message that was not acknowledged or mark as pending
		if (!(this->resendPending = isDownSendBusy()))
			resend();
	}
}

bool MqttSnBroker::TopicInfo::isRemoteSubscribed() {
	for (int i = 0; i < (MAX_CLIENT_COUNT + 15) / 16; ++i) {
		uint32_t clientQos = this->clientQosArray[i];
		
		if (i == MAX_CLIENT_COUNT / 16) {
			clientQos |= 0xffffffff << (MAX_CLIENT_COUNT & 15) * 2;
		}

		if (clientQos != 0xffffffff)
			return true;
	}
	return false;
}

int8_t MqttSnBroker::TopicInfo::getMaxQos() {
	uint32_t a = 0xffffffff;
	uint32_t b = 0xffffffff;
	for (uint32_t clientQos : this->clientQosArray) {
		uint32_t c = clientQos;
		uint32_t d = clientQos >> 1;
		
		uint32_t x = (a & c) | ((a | c) & ~b & ~d);
		uint32_t y = (b & d) | (~a & b) | (~c & d);
		
		a = x;
		b = y;
	}
	
	for (int shift = 16; shift >= 2; shift >>= 1) {
		uint32_t c = a >> shift;
		uint32_t d = b >> shift;
		
		uint32_t x = (a & c) | ((a | c) & ~b & ~d);
		uint32_t y = (b & d) | (~a & b) | (~c & d);
		
		a = x;
		b = y;
	}
	return ((((a & 1) | ((b & 1) << 1)) + 1) & 3) - 1;
}

void MqttSnBroker::TopicInfo::setQos(int clientIndex, int8_t qos) {
	uint32_t &clientQos = this->clientQosArray[clientIndex >> 4];
	int shift = (clientIndex << 1) & 31;
	clientQos = (clientQos & ~(3 << shift)) | ((qos & 3) << shift);
}

int8_t MqttSnBroker::TopicInfo::clearQos(int clientIndex) {
	uint32_t &clientQos = this->clientQosArray[clientIndex >> 4];
	int shift = (clientIndex << 1) & 31;
	int8_t qos = (((clientQos >> shift) + 1) & 3) - 1;
	clientQos |= 3 << shift;
	return qos;
}

MqttSnBroker::ClientInfo *MqttSnBroker::findClient(uint16_t clientId) {
	// client at index 0 is local client
	for (int i = 1; i < MAX_CLIENT_COUNT; ++i) {
		ClientInfo &client = this->clients[i];
		if (client.clientId == clientId)
			return &client;
	}
	return nullptr;
}

uint16_t MqttSnBroker::getTopicId(String name) {
	// calc djb2 hash of name
	// http://www.cse.yorku.ca/~oz/hash.html
	uint32_t hash = 5381;
	for (char c : name) {
		hash = (hash << 5) + hash + uint8_t(c); // hash * 33 + c
    }
	
	// search for topic
	int empty = -1;
	for (int i = 0; i < this->topicCount; ++i) {
		TopicInfo *topic = &this->topics[i];
		if (topic->hash == hash)
			return i + 1;
		if (topic->hash == 0)
			empty = i;
	}
			
	// if neither found the topic by hash nor an empty topic, add a new topic
	if (empty == -1) {
		if (this->topicCount >= MAX_TOPIC_COUNT)
			return 0;
		empty = this->topicCount++;
	}
	
	// initialize topic
	TopicInfo *topic = &this->topics[empty];
	topic->hash = hash;
	topic->retainedOffset = this->retainedSize;
	topic->retainedAllocated = 0;
	topic->retainedLength = 0;
	for (uint32_t &clientQos : topic->clientQosArray)
		clientQos = 0xffffffff;
	topic->gatewayTopicId = 0;
	topic->gatewayQos = -1;
	
	return empty + 1;
}

MqttSnBroker::TopicInfo *MqttSnBroker::findTopic(uint16_t topicId) {
	if (topicId < 1 || topicId > this->topicCount)
		return nullptr;
	TopicInfo *topic = &this->topics[topicId - 1];
	return topic->hash != 0 ? topic : nullptr;
}

uint16_t MqttSnBroker::findTopicId(uint16_t gatewayTopicId) {
	for (int i = 0; i < this->topicCount; ++i) {
		TopicInfo *topic = &this->topics[i];
		if (topic->hash != 0 && topic->gatewayTopicId == gatewayTopicId)
			return i + 1;
	}
	return 0;
}

uint16_t MqttSnBroker::insertRetained(int offset, int length) {
	// make space in the memory for retained messages
	for (int i = offset; i < this->retainedSize; ++i) {
		this->retained[i + length] = this->retained[i];
	}
	this->retainedSize += length;
	
	// adjust offsets of all retained messages starting at or behind offset
	for (int i = 0; i < this->topicCount; ++i) {
		TopicInfo &topic = this->topics[i];
		if (topic.retainedOffset >= offset)
			topic.retainedOffset += length;
	}
	
	return uint16_t(offset);
}

uint16_t MqttSnBroker::eraseRetained(int offset, int length) {
	// erase region in the memory for retained messages
	for (int i = this->retainedSize; i >= offset; --i) {
		this->retained[i + length] = this->retained[i];
	}

	// adjust offsets of all retained messages starting at or behind offset
	for (int i = 0; i < this->topicCount; ++i) {
		TopicInfo &topic = this->topics[i];
		if (topic.retainedOffset >= offset)
			topic.retainedOffset -= length;
	}

	return uint16_t(this->retainedSize);
}

MqttSnBroker::Message MqttSnBroker::addSendMessage(int length, mqttsn::MessageType type) {
	if (this->busy)
		return {};

	// garbage collect old messages
	if (this->sendMessagesHead + 1 + 1 > MAX_SEND_COUNT
		|| this->sendBufferHead + length + MAX_MESSAGE_LENGTH > SEND_BUFFER_SIZE )
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

	// clear client flags
	for (uint32_t &client : info->clients) {
		client = 0;
	}

	// clear message id
	info->msgId = 0;

	// set busy flag if no new message will fit
	this->busy = this->sendMessagesHead >= MAX_SEND_COUNT || this->sendBufferFill + MAX_MESSAGE_LENGTH > SEND_BUFFER_SIZE;

	return {info, data};
}

void MqttSnBroker::sendCurrentMessage() {
	int current = this->sendMessagesCurrent;

	// get message
	MessageInfo *info = &this->sendMessages[current];
	uint8_t *data = this->sendBuffer + info->offset;
	int length = data[0];
	
	// get flags of PUBLISH message
	uint8_t flags = data[2];

	// get client id
	uint16_t clientId;
	int clientIndex;
	if (data[1] != mqttsn::PUBLISH) {
		// send message to just one client
		clientId = info->clients[0];
		clientIndex = MAX_CLIENT_COUNT;
	} else {
		// send message to all clients whose flag is set
		clientIndex = this->sendMessagesClientIndex;
		
		// skip empty flags (guaranteed that one flag is set)
		while (((info->clients[clientIndex >> 5] >> (clientIndex & 31)) & 1) == 0) {
			++clientIndex;
		}
		
		// get client id
		clientId = this->clients[clientIndex].clientId;
		
		// get topic
		uint16_t topicId = mqttsn::getUShort(data + 3);
		TopicInfo &topic = getTopic(topicId);
		
		// set actual qos to message
		int8_t clientQos = topic.getQos(clientIndex);
		int8_t qos = mqttsn::getQos(flags);
		data[2] = (flags & ~mqttsn::makeQos(3)) | mqttsn::makeQos(min(qos, clientQos));
		
		// go to next set flag
		while (clientIndex < MAX_CLIENT_COUNT) {
			++clientIndex;
			if (((info->clients[clientIndex >> 5] >> (clientIndex & 31)) & 1) != 0)
				break;
		}
	}
		
	// send to client
	downSend(clientId, data, length);

	// restore flags in PUBLISH message
	data[2] = flags;

	// set sent time
	info->sentTime = getSystemTime();

	if (current == this->sendMessagesTail) {
		if (info->msgId == 0) {
			// remove message from tail because it's not needed any more such as PINGREQ
			if (clientIndex == MAX_CLIENT_COUNT)
				this->sendMessagesTail = current + 1;

			// set timer for idle ping to keep the connection alive
			setSystemTimeout2(info->sentTime + KEEP_ALIVE_INTERVAL);
		} else {
			// start retransmission timeout
			if (this->sendMessagesClientIndex == 0)
				setSystemTimeout2(info->sentTime + RETRANSMISSION_INTERVAL);
		}
	}

	// make next message current
	if (clientIndex == MAX_CLIENT_COUNT) {
		this->sendMessagesCurrent = current + 1;
		clientIndex = 0;
	}
	
	this->sendMessagesClientIndex = clientIndex;
}

void MqttSnBroker::resend() {
	// get message
	MessageInfo *info = &this->sendMessages[this->sendMessagesTail];
	uint8_t *data = this->sendBuffer + info->offset;
	int length = data[0];

	// get flags of PUBLISH message
	uint8_t flags = data[2];

	// get client id
	uint16_t clientId;
	int clientIndex;
	if (data[1] != mqttsn::PUBLISH) {
		// send message to just one client
		clientId = info->clients[0];
		clientIndex = MAX_CLIENT_COUNT;
	} else {
		// send message to all clients whose flag is set
		clientIndex = this->sendMessagesClientIndex;
		
		// skip empty flags (guaranteed that one flag is set)
		while (((info->clients[clientIndex >> 5] >> (clientIndex & 31)) & 1) == 0) {
			++clientIndex;
		}
		
		// get client id
		clientId = this->clients[clientIndex].clientId;
		
		// get topic
		uint16_t topicId = mqttsn::getUShort(data + 3);
		TopicInfo &topic = getTopic(topicId);
		
		// set actual qos to message
		int8_t clientQos = topic.getQos(clientIndex);
		int8_t qos = mqttsn::getQos(flags);
		data[2] = (flags & ~mqttsn::makeQos(3)) | mqttsn::makeQos(min(qos, clientQos));
	}

	// send to client
	downSend(clientId, data, length);

	// restore flags in PUBLISH message
	data[2] = flags;

	// start timeout, either for retransmission or idle ping
	setSystemTimeout2(getSystemTime() + RETRANSMISSION_INTERVAL);
}

void MqttSnBroker::removeSentMessage(uint16_t msgId, uint16_t clientIndex) {
	int tail = this->sendMessagesTail;
	
	// mark message with msgId as obsolete
	for (int i = tail; i < this->sendMessagesCurrent; ++i) {
		MessageInfo *info = &this->sendMessages[i];
		
		if (info->msgId == msgId) {
			uint32_t allClients = 0;

			if (clientIndex < MAX_CLIENT_COUNT) {
				// clear client flag
				info->clients[clientIndex >> 5] &= ~(1 << (clientIndex & 31));
				
				// get all client flags
				for (uint32_t clients : info->clients)
					allClients |= clients;
			}
			
			// remove message if all clients have acknowledged
			if (allClients == 0) {
				info->msgId = 0;
				int length = this->sendBuffer[info->offset];
				this->sendBufferFill -= length;
				this->busy = this->sendBufferFill + MAX_MESSAGE_LENGTH > SEND_BUFFER_SIZE;
			}
			break;
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
}

void MqttSnBroker::sendPublish(uint16_t topicId, TopicInfo *topic, uint8_t const *data, int length, int8_t qos,
	bool retain, uint16_t excludeClientId)
{
	// handle retained message
	if (retain) {
		if (length > 0) {
			// store retained message
			if (length > topic->retainedAllocated) {
				// enlarge space for retained message
				topic->retainedOffset = insertRetained(topic->retainedOffset,
					length - topic->retainedAllocated);
				topic->retainedAllocated = length;
			}
			memcpy(&this->retained[topic->retainedOffset], data, length);
			topic->retainedLength = length;
		} else {
			// delete retained message
			topic->retainedOffset = eraseRetained(topic->retainedOffset,
				topic->retainedAllocated);
			topic->retainedAllocated = 0;
			topic->retainedLength = 0;
		}
	}
	
	// check if a remote client has subscribed the topic
	if (topic->isRemoteSubscribed()) {
		// allocate message
		Message m = addSendMessage(7 + length, mqttsn::PUBLISH);

		// generate and set message id
		uint16_t msgId = getNextMsgId();
		m.info->msgId = msgId;

		// publish to all clients that have subscribed to the topic
		for (ClientInfo &client : this->clients) {
			uint16_t clientId = client.clientId;
			if (clientId != 0 && clientId != excludeClientId) {
				int clientQos = topic->getQos(client.index);
				if (clientQos >= 0) {
					m.info->clients[client.index >> 5] |= 1 << (client.index & 31);
				}
			}
		}

		// serialize message
		m.data[2] = mqttsn::makeQos(qos) | mqttsn::makeRetain(retain) | mqttsn::makeTopicType(mqttsn::TopicType::NORMAL);
		mqttsn::setUShort(m.data + 3, topicId);
		mqttsn::setUShort(m.data + 5, msgId);
		memcpy(m.data + 7, data, length);
		
		// send message
		if (!isDownSendBusy())
			sendCurrentMessage();
	}

	// publish to local client if it has subscribed to the topic
	int8_t localQos = topic->getQos(LOCAL_CLIENT_INDEX);
	if (localQos >= 0)
		onPublished(topicId, data, length, min(qos, localQos), retain);
}

void MqttSnBroker::sendDisconnect(uint16_t clientId) {
	// create reply message
	Message m = addSendMessage(2, mqttsn::DISCONNECT);
	if (m.info != nullptr) {
		m.info->clients[0] = clientId;
		
		// send message
		if (!isDownSendBusy())
			sendCurrentMessage();
	} else {
		// error: send message queue full
	}
}