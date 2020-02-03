#pragma once

#include "LayoutManager.hpp"
#include <stdlib.h> // prevent second definition of abs() in util.hpp
#include "DeviceState.hpp"


class DeviceWidget : public Widget {
public:
	
	DeviceWidget(const char *fragmentShaderSource, LayoutManager &layoutManager)
		: Widget(fragmentShaderSource), layoutManager(layoutManager)
	{
		layoutManager.add(this);
	}

	virtual ~DeviceWidget();

	/**
	 * Check if this widget is compatible with the given device type
	 */
	//virtual bool isCompatible(Device::Type type) = 0;
	
	/**
	 * Set the device state
	 */
	virtual void setState(const DeviceState &deviceState) = 0;

protected:
	
	LayoutManager &layoutManager;
};
