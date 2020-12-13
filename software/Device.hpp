#pragma once


/**
 * Device id type
 */
using DeviceId = uint32_t;


/**
 * Type of device endpoint such as button, relay or temperature sensor
 */
 enum class EndpointType : uint8_t {
	// generic binary sensor with two states
	BINARY_SENSOR = 1,
	
	// button, returns to released state when not pressed
	BUTTON = 2,
	
	// switch with two stable states
	SWITCH = 3,
	
/*
	// two binary sensors
	BINARY_SENSOR_2 = 4,

	// two buttons
	BUTTON_2 = 5,

	// two switches
	SWITCH_2 = 6,
*/
	// rocker with two sides, returns to released state when not pressed
	ROCKER = 7,

/*
	// four binary sensors
	BINARY_SENSOR_4 = 8,

	// two buttons
	BUTTON_4 = 9,

	// four switches
	SWITCH_4 = 10,

	// two rockers
	ROCKER_2 = 11,
*/
	
	// generic relay with two states
	RELAY = 20,
	
	// light, only on/off
	LIGHT = 21,

/*
	// two relays
	RELAY_2 = 22,

	// two lights
	LIGHT_2 = 23,
*/
	// two interlocked relays for blind, only one can be on
	BLIND = 24,
	
/*
	// four relays
	RELAY_4 = 25,

	// four lights
	LIGHT_4 = 26,

	// two blinds
	BLIND_2 = 27,
*/

	// temperature sensor (16 bit value, 1/20 Kelvin)
	TEMPERATURE_SENSOR = 30,
	
	// air pressure sensor
	AIR_PRESSURE_SENSOR = 31,
	
	// air humidity sensor
	AIR_HUMIDITY_SENSOR = 32,
	
	// air voc (volatile organic compounds) content sensor
	AIR_VOC_SENSOR = 33,


	// brightness sensor
	BRIGHTNESS_SENSOR = 40,
	
	// motion detector
	MOTION_DETECTOR = 41,


	// active energy counter of an electicity meter
	ACTIVE_ENERGY_COUNTER = 50,
	
	// active power measured by an electricity meter
	ACTIVE_POWER_SENSOR = 51,
 };
 
