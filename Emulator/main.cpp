
#include <iostream>
#include <fstream>
#include <vector>
#include <thread>
#include <iterator>

#include "Flash.hpp"
#include "System.hpp"

#include "gui/LayoutManager.hpp"
#include "gui/Display.hpp"
#include "gui/Poti.hpp"
#include "gui/Temperature.hpp"
#include "gui/Light.hpp"
#include "gui/Blind.hpp"


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


#define NO_ACTION_4 {0xff, 0xff}, {0xff, 0xff}, {0xff, 0xff}, {0xff, 0xff}
#define NO_ACTION_5 NO_ACTION_4, {0xff, 0xff}
#define NO_ACTION_6 NO_ACTION_5, {0xff, 0xff}
#define NO_ACTION_7 NO_ACTION_6, {0xff, 0xff}
#define NO_ACTION_8 NO_ACTION_7, {0xff, 0xff}
Event eventsData[] = {
	{{{{0, Device::ON}, {0, Device::OFF}, NO_ACTION_6}}, Event::Type::SWITCH, 0x10, 0xfef3ac9b}, // bottom left
	{{{{1, Device::OFF}, {0, Action::SCENARIO}, NO_ACTION_6}}, Event::Type::SWITCH, 0x30, 0xfef3ac9b}, // top left
	{{{{4, Device::STOP}, {4, Device::MOVE}, NO_ACTION_6}}, Event::Type::SWITCH, 0x50, 0xfef3ac9b}, // bottom right
	{{{{5, Device::CLOSED}, {5, Device::OPEN}, NO_ACTION_6}}, Event::Type::SWITCH, 0x70, 0xfef3ac9b}, // top right
/*
	{0xfef3ac9b, Event::Type::SWITCH, 0x10, {{0, Action::SET_TRANSITION_START + 0}, {0, Action::SET_TRANSITION_START + 1}, NO_ACTION_6}}, // bottom left
	{0xfef3ac9b, Event::Type::SWITCH, 0x30, {{1, Action::SET_TRANSITION_START + 0}, {1, Action::SET_TRANSITION_START + 1}, NO_ACTION_6}}, // top left
	{0xfef3ac9b, Event::Type::SWITCH, 0x50, {{4, 100}, NO_ACTION_7}}, // bottom right
	{0xfef3ac9b, Event::Type::SWITCH, 0x70, {{4, 0}, NO_ACTION_7}}, // top right
*/
	{{{{2, Device::ON}, NO_ACTION_7}}, Event::Type::TIMER, Event::SUNDAY, Clock::time(22, 41)},
	{{{{2, Device::ON}, NO_ACTION_7}}, Event::Type::TIMER, Event::THURSDAY, Clock::time(22, 41)},
	{{{{3, Device::ON}, NO_ACTION_7}}, Event::Type::TIMER, Event::MONDAY, Clock::time(10, 0)}
};

Scenario scenariosData[] = {
	{{{{0, Device::ON}, {1, Device::ON}, {2, Device::ON}, {3, Device::ON}, NO_ACTION_4}}, 0, "Lights On"},
};

Device devicesData[] = {
	{0, Device::Type::SWITCH, "Light1", 0, 0, {NO_ACTION_8}},
	{1, Device::Type::SWITCH, "Light2", 0, 1, {NO_ACTION_8}},
	{2, Device::Type::SWITCH, "Light3", 0, 2, {NO_ACTION_8}},
	{3, Device::Type::SWITCH, "Light4", 0, 3, {NO_ACTION_8}},
	{4, Device::Type::BLIND, "Blind1", 2000, 4, {NO_ACTION_8}},
	{5, Device::Type::BLIND, "Blind2", 2000, 6, {NO_ACTION_8}},
	{6, Device::Type::BLIND, "Blind3", 2000, 8, {NO_ACTION_8}},
	{7, Device::Type::BLIND, "Blind4", 2000, 10, {NO_ACTION_8}},
	{8, Device::Type::HANDLE, "Handle1", 0, 0xffffffff, {NO_ACTION_8}},
};

/**
 * Pass path to EnOcean device as argument
 * MacOS: /dev/tty.usbserial-FT3PMLOR
 * Linux: /dev/ttyUSB0 (add yourself to the dialout group: sudo usermod -a -G dialout $USER)
 */
int main(int argc, const char **argv) {
	if (argc <= 1) {
		std::cout << "usage: emulator <device>" << std::endl;
	#if __APPLE__
		std::cout << "example: emulator /dev/tty.usbserial-FT3PMLOR" << std::endl;
	#elif __linux__
		std::cout << "example: emulator /dev/ttyUSB0" << std::endl;
	#endif
		return 1;
	}
	
	std::cout << "sizeof(Event): " << sizeof(Event) << std::endl;
	std::cout << "sizeof(Scenario): " << sizeof(Scenario) << std::endl;
	std::cout << "sizeof(Device): " << sizeof(Device) << std::endl;

	// erase emulated flash
	memset(Flash::data, 0xff, sizeof(Flash::data));

	// read flash contents from file
	std::ifstream is("flash.bin", std::ios::binary);
	is.read((char*)Flash::data, sizeof(Flash::data));
	is.close();

	// get device from command line
	std::string device = argv[1];
	
	// init GLFW
	glfwSetErrorCallback(errorCallback);
	if (!glfwInit())
		exit(EXIT_FAILURE);
	
	// create GLFW window and OpenGL 3.3 Core context
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	GLFWwindow *window = glfwCreateWindow(640, 480, "RoomControl", NULL, NULL);
	if (!window) {
		glfwTerminate();
		exit(EXIT_FAILURE);
	}
	glfwSetKeyCallback(window, keyCallback);
	glfwSetMouseButtonCallback(window, mouseCallback);
	
	// make OpenGL context current
	glfwMakeContextCurrent(window);
	
	// load OpenGL functions
	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
	
	// no v-sync
	glfwSwapInterval(0);

	System system(device);
	Display display;

	// default initialize arrays if empty
	if (system.events.size() == 0) {
		system.events.assign(eventsData);
		system.scenarios.assign(scenariosData);
		system.devices.assign(devicesData);
		//system.outputs.assign(outputsData);

		for (int i = 0; i < system.devices.size(); ++i) {
			system.deviceStates[i].init(system.devices[i]);
		}
	}

	LayoutManager layoutManager;
	float y = 0.1f;

	// display
	layoutManager.add(&display);
	display.setRect(0.3f, y, 0.4f, 0.2f);

	y += 0.3f;

	// poti
	layoutManager.add(&system.poti);
	system.poti.setRect(0.2f, y, 0.25f, 0.25f);

	// motion detector
	layoutManager.add(&system.motionDetector);
	system.motionDetector.setRect(0.55f, y + 0.05, 0.15f, 0.15f);

	//Poti poti2;
	//layoutManager.add(&poti2);
	//poti2.setRect(0.55f, y, 0.25f, 0.25f);

	// temperature
	layoutManager.add(&system.temperature);
	system.temperature.setRect(0.85f, y, 0.1, 0.25f);

	y += 0.3f;

	// devices
	std::vector<std::unique_ptr<DeviceWidget>> devices;

	// main loop
	int frameCount = 0;
	auto start = std::chrono::steady_clock::now();
	while (!glfwWindowShouldClose(window)) {
		auto frameStart = std::chrono::steady_clock::now();

		// get frame buffer size
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);

		// mouse
		layoutManager.doMouse(window);
		
		// update system
		system.update();

		// update display
		display.update(system.bitmap);

		// check if device widgets still match system devices
		int deviceCount = system.devices.size();
		bool newDeviceWidgets = devices.size() != deviceCount;
		if (!newDeviceWidgets) {
			for (int i = 0; i < deviceCount; ++i) {
				auto type = system.devices[i].type;
				if (Light::isCompatible(type)) {
					newDeviceWidgets |= dynamic_cast<Light*>(devices[i].get()) == nullptr;
				} else if (Blind::isCompatible(type)) {
					newDeviceWidgets |= dynamic_cast<Blind*>(devices[i].get()) == nullptr;
				} else {
					newDeviceWidgets |= devices[i] != nullptr;
				}
			}
		}
		if (newDeviceWidgets) {
			// remove old widgets
			devices.resize(deviceCount);

			// create new widgets
			const float step = 1.0f / 8.0f;
			const float size = 0.1f;
			float x = (step - size) * 0.5f;
			for (int i = 0; i < deviceCount; ++i) {
				auto type = system.devices[i].type;
				if (Light::isCompatible(type)) {
					devices[i] = std::make_unique<Light>(layoutManager);
					devices[i]->setRect(x, y, size, size);
					x += step;
				} else if (Blind::isCompatible(type)) {
					devices[i] = std::make_unique<Blind>(layoutManager);
					devices[i]->setRect(x, y, size, size * 2);
					x += step;
				} else {
					devices[i] = nullptr;
				}
			}
		}
		
		// transfer device states to widgets
		for (int i = 0; i < deviceCount; ++i) {
			if (devices[i] != nullptr)
				devices[i]->setState(system.deviceStates[i]);
		}

		// draw emulator on screen
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

	// write flash
	std::ofstream os("flash.bin", std::ios::binary);
	os.write((char*)Flash::data, sizeof(Flash::data));
	os.close();

	// cleanup
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
