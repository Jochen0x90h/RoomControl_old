#include "DeviceWidget.hpp"


DeviceWidget::~DeviceWidget()
{
	this->layoutManager.remove(this);
}
