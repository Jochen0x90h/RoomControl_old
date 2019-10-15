#include "Serial.hpp"


/**
 * Protocol of Enocean TCM310 or USB300 module
 */
class EnOceanProtocol {
public:
	// Packet type
	enum PacketType {
		NONE = 0,
		
		// ERP1 radio telegram (raw data)
		RADIO_ERP1 = 1,
		
		//
		RESPONSE = 2,
		
		//
		RADIO_SUB_TEL = 3,
		
		// An EVENT is primarily a confirmation for processes and procedures, incl. specific data content
		EVENT = 4,
		
		// Common commands
		COMMON_COMMAND = 5,
		
		//
		SMART_ACK_COMMAND = 6,
		
		//
		REMOTE_MAN_COMMAND = 7,
		
		//
		RADIO_MESSAGE = 9,
		
		//
		RADIO_ERP2 = 10,
		
		//
		RADIO_802_15_4 = 16,
		
		//
		COMMAND_2_4 = 17
	};

	// Limits
	enum {
		MAX_DATA_LENGTH = 256,
		MAX_OPTIONAL_LENGTH = 16
	};
	
	EnOceanProtocol(Serial::Device device) : serial(device) {
		// start receiving data
		receive();
	}

	void update();

protected:

	void receive();

	virtual void onPacket(uint8_t packetType, const uint8_t *data, int length,
		const uint8_t *optionalData, int optionalLength) = 0;
	
	/// Calculate checksum of EnOcean frame
	uint8_t calcChecksum(const uint8_t *data, int length);

	
	Serial serial;
	
	// recieve buffer
	int rxPosition = 0;
	uint8_t rxBuffer[6 + MAX_DATA_LENGTH + MAX_OPTIONAL_LENGTH + 1];
};
