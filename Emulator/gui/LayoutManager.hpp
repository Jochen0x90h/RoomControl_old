#pragma once

#include "Widget.hpp"
#include "glad/glad.h"
#include <GLFW/glfw3.h> // http://www.glfw.org/docs/latest/quick_guide.html
#include <vector>
#include <algorithm>


class LayoutManager {
public:

	void add(Widget *widget) {
		this->widgets.push_back(widget);
	}
	
	void remove(Widget *widget) {
		auto it = std::find(this->widgets.begin(), this->widgets.end(), widget);
		this->widgets.erase(it);
		if (widget == this->activeWidget)
			this->activeWidget = nullptr;
	}
	
	void doMouse(GLFWwindow *window) {
		// get window size
		int windowWidth, windowHeight;
		glfwGetWindowSize(window, &windowWidth, &windowHeight);

		double x;
        double y;
        glfwGetCursorPos(window, &x, &y);
		bool leftDown = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
		//std::cout << "x " << x << " y " << y << " down " << leftDown << std::endl;
		if (leftDown) {
			bool first = this->activeWidget == nullptr;
			float windowX = x / windowWidth;
			float windowY = y / windowHeight;
			if (first) {
				// search widget under mouse
				for (Widget * widget : this->widgets) {
					if (widget->contains(windowX, windowY))
						this->activeWidget = widget;
				}
			}
			if (this->activeWidget != nullptr) {
				float localX = (windowX - this->activeWidget->x1) / (this->activeWidget->x2 - this->activeWidget->x1);
				float localY = (windowY - this->activeWidget->y1) / (this->activeWidget->y2 - this->activeWidget->y1);
				this->activeWidget->touch(first, localX, localY);
			}
		} else {
			if (this->activeWidget != nullptr)
				this->activeWidget->release();
			this->activeWidget = nullptr;
		}
	}
	
	void draw() {
		for (Widget *widget : this->widgets) {
			widget->draw();
		}
	}
	
	std::vector<Widget*> widgets;
	Widget *activeWidget = nullptr;
};
