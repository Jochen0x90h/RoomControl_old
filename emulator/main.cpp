
#include <iostream>
#include <fstream>
#include <vector>
#include <thread>
#include <iterator>

#include "Flash.hpp"
#include "RoomControl.hpp"

#include "Gui.hpp"


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

/*
Event eventData[] = {
	{0xfef3ac9b, 0x10, {2, {{0, Device::ON}, {0, Device::OFF}}}}, // bottom left
	{0xfef3ac9b, 0x30, {2, {{1, Device::OFF}, {0, Action::SCENARIO}}}}, // top left
	{0xfef3ac9b, 0x50, {2, {{4, Device::STOP}, {4, Device::MOVE}}}}, // bottom right
	{0xfef3ac9b, 0x70, {2, {{5, Device::CLOSED}, {5, Device::OPEN}}}}, // top right
};

Timer timerData[] = {
	{Clock::time(22, 41), Timer::SUNDAY, {1, {{2, Device::ON}}}},
	{Clock::time(10, 0), Timer::MONDAY, {1, {{3, Device::ON}}}},
};

Scenario scenarioData[] = {
	{0, "Lights On", {4, {{0, Device::ON}, {1, Device::ON}, {2, Device::ON}, {3, Device::ON}}}},
};

Device deviceData[] = {
	{0, Device::Type::SWITCH, "Light1", 500, 5000, 0}, // delay 0.5s, timeout for on state 3s
	{1, Device::Type::SWITCH, "Light2", 0, 0, 1},
	{2, Device::Type::SWITCH, "Light3", 0, 0, 2},
	{3, Device::Type::SWITCH, "Light4", 0, 0, 3},
	{4, Device::Type::BLIND, "Blind1", 2000, 0, 4},
	{5, Device::Type::BLIND, "Blind2", 2000, 0, 6},
	{6, Device::Type::BLIND, "Blind3", 2000, 0, 8},
	{7, Device::Type::BLIND, "Blind4", 2000, 0, 10},
	{8, Device::Type::HANDLE, "Handle1", 0, 0, 0},
};
*/

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
/*
	std::cout << "sizeof(Event): " << sizeof(Event) << " byteSize(): " << eventData[0].byteSize() << std::endl;
	std::cout << "sizeof(Timer): " << sizeof(Timer) << " byteSize(): " << timerData[0].byteSize() << std::endl;
	std::cout << "sizeof(Scenario): " << sizeof(Scenario) << " byteSize(): " << scenarioData[0].byteSize() << std::endl;
	std::cout << "sizeof(Device): " << sizeof(Device) << " byteSize(): " << deviceData[0].byteSize() << std::endl;
*/
	// erase emulated flash
	memset(const_cast<uint8_t*>(Flash::getAddress(0)), 0xff, Flash::PAGE_COUNT * Flash::PAGE_SIZE);

	// read flash contents from file
	std::ifstream is("flash.bin", std::ios::binary);
	//is.read(reinterpret_cast<char*>(const_cast<uint8_t*>(Flash::getAddress(0))), Flash::PAGE_COUNT * Flash::PAGE_SIZE);
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
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	//glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GL_TRUE);
	GLFWwindow *window = glfwCreateWindow(800, 800, "RoomControl", NULL, NULL);
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
	
	// v-sync
	glfwSwapInterval(1);

	// the room control application
	RoomControl roomControl;
		
	// emulator user interface
	Gui gui;

	// main loop
	int frameCount = 0;
	auto start = std::chrono::steady_clock::now();
	while (!glfwWindowShouldClose(window)) {
		auto frameStart = std::chrono::steady_clock::now();

		// process events
		glfwPollEvents();

		// process emulated system events
		global::context.poll();
		if (global::context.stopped())
			global::context.reset();

		// mouse
		gui.doMouse(window);

		// set viewport
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		glViewport(0, 0, width, height);
		
		// clear screen
		glClear(GL_COLOR_BUFFER_BIT);

		// gui
		{
			int id = 0;
		
			// display
			gui.display(roomControl.emulatorDisplayBitmap);
		
			// poti
			auto poti = gui.poti(id++);
			roomControl.onPotiChanged(poti.first, poti.second);
			
			gui.newLine();
			
			// double rocker switch
			gui.doubleRocker(id++);

			gui.temperatureSensor(id++);

			gui.light(true, 100);
			
			gui.blind(50);
		}
		
		// swap render buffer to screen
		glfwSwapBuffers(window);
		
		
		// show frames per second
		auto now = std::chrono::steady_clock::now();
		++frameCount;
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);
		if (duration.count() > 1000) {
			//std::cout << frameCount * 1000 / duration.count() << "fps" << std::endl;
			frameCount = 0;
			start = std::chrono::steady_clock::now();
		}

	}
/*
	// default initialize arrays if empty
	if (system.events.size() == 0) {
		system.events.assign(eventData);
		system.timers.assign(timerData);
		system.scenarios.assign(scenarioData);
		system.devices.assign(deviceData);

		for (int i = 0; i < system.devices.size(); ++i) {
			system.deviceStates[i].init(system.devices[i]);
		}
	}
*/

/*
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
*/

	// write flash
	std::ofstream os("flash.bin", std::ios::binary);
	os.write(reinterpret_cast<const char*>(Flash::getAddress(0)), Flash::PAGE_COUNT * Flash::PAGE_SIZE);
	os.close();

	// cleanup
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
