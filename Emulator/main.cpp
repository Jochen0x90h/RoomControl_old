#include "Display.hpp"
#include "Poti.hpp"
#include "glad/glad.h"
#include <GLFW/glfw3.h> // http://www.glfw.org/docs/latest/quick_guide.html
#include <iostream>
#include <vector>
#include <chrono>
#include <thread>

// fonts
#include "tahoma_8pt.hpp"


static void errorCallback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

static void mouseCallback(GLFWwindow* window, int button, int action, int mods) {
 /*   if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if(GLFW_PRESS == action)
            lbutton_down = true;
        else if(GLFW_RELEASE == action)
            lbutton_down = false;
    }

    if(lbutton_down) {
         // do your drag here
    }*/
}


class LayoutManager {
public:

	void add(Widget * widget) {
		this->widgets.push_back(widget);
	}
	
	void doMouse(GLFWwindow * window) {
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
			float windowY = 1.0f - y / windowHeight;
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
			this->activeWidget = nullptr;
		}
	}
	
	void draw() {
		for (Widget * widget : this->widgets) {
			widget->draw();
		}
	}
	
	std::vector<Widget *> widgets;
	Widget* activeWidget = nullptr;
};

int main(void) {
	GLFWwindow *window;
	
	// init
	glfwSetErrorCallback(errorCallback);
	if (!glfwInit())
		exit(EXIT_FAILURE);
	
	// create window and OpenGL context
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	window = glfwCreateWindow(640, 480, "LedControl", NULL, NULL);
	if (!window) {
		glfwTerminate();
		exit(EXIT_FAILURE);
	}
	glfwSetKeyCallback(window, keyCallback);
	glfwSetMouseButtonCallback(window, mouseCallback);
	
	glfwMakeContextCurrent(window);
	gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
	
	// no v-sync
	glfwSwapInterval(0);

	LayoutManager layoutManager;
	
	float y = 0.9f;

	// display
	Display display;
	layoutManager.add(&display);
	display.x1 = 0.3;
	display.y1 = y - 0.2f;
	display.x2 = 0.7f;
	display.y2 = y;

	y -= 0.3f;

	Poti poti1;
	layoutManager.add(&poti1);
	poti1.x1 = 0.2;
	poti1.y1 = y - 0.25f;
	poti1.x2 = 0.45f;
	poti1.y2 = y;

	Poti poti2;
	layoutManager.add(&poti2);
	poti2.x1 = 0.55;
	poti2.y1 = y - 0.25f;
	poti2.x2 = 0.8f;
	poti2.y2 = y;

	
	// loop
	int frameCount = 0;
	auto start = std::chrono::steady_clock::now();
	while (!glfwWindowShouldClose(window)) {
		auto frameStart = std::chrono::steady_clock::now();

		// get frame buffer size
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);

		// mouse
		layoutManager.doMouse(window);
		

		// display
		{
			display.bitmap.clear();

			const char *name = "foo";

			// effect name
			int y = 10;
			int len = tahoma_8pt.calcWidth(name);
			display.bitmap.drawText((display.bitmap.WIDTH - len) >> 1, y, tahoma_8pt, name, Mode::SET);
			
			display.update();
		}
		
		// draw on screen
		glViewport(0, 0, width, height);
		glClear(GL_COLOR_BUFFER_BIT);
		layoutManager.draw();
		
		glfwSwapBuffers(window);
		glfwPollEvents();

		auto now = std::chrono::steady_clock::now();
		std::this_thread::sleep_for(std::chrono::milliseconds(9) - (now - frameStart));
		
		// show frames per second
		++frameCount;
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);
		if (duration.count() > 1000) {
			//std::cout << frameCount * 1000 / duration.count() << "fps" << std::endl;
			frameCount = 0;
			start = std::chrono::steady_clock::now();
		}
	}
	
	// cleanup
	glfwDestroyWindow(window);
	glfwTerminate();
	exit(EXIT_SUCCESS);
}
