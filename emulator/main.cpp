
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

constexpr String routeData[][2] = {
	{"room/00000001/a", "room/00000001/x"},
	{"room/00000001/b0", "room/00000001/y0"},
	{"room/00000001/b1", "room/00000001/y1"},
	{"room/00000002/a", "room/00000002/x"},
	{"room/00000002/b0", "room/00000002/y"}
};

struct TimerData {
	ClockTime time;
	String topic;
};
constexpr TimerData timerData[] = {
	{makeClockTime(1, 10, 00), "room/00000001/x"},
	{makeClockTime(2, 22, 41), "room/00000001/y0"}
};


/**
 * Pass path to EnOcean device as argument
 * MacOS: /dev/tty.usbserial-FT3PMLOR
 * Linux: /dev/ttyUSB0 (add yourself to the dialout group: sudo usermod -a -G dialout $USER)
 */
int main(int argc, const char **argv) {

/*	if (argc <= 1) {
		std::cout << "usage: emulator <device>" << std::endl;
	#if __APPLE__
		std::cout << "example: emulator /dev/tty.usbserial-FT3PMLOR" << std::endl;
	#elif __linux__
		std::cout << "example: emulator /dev/ttyUSB0" << std::endl;
	#endif
		return 1;
	}
*/
/*
	std::cout << "sizeof(Event): " << sizeof(Event) << " byteSize(): " << eventData[0].byteSize() << std::endl;
	std::cout << "sizeof(Timer): " << sizeof(Timer) << " byteSize(): " << timerData[0].byteSize() << std::endl;
	std::cout << "sizeof(Scenario): " << sizeof(Scenario) << " byteSize(): " << scenarioData[0].byteSize() << std::endl;
	std::cout << "sizeof(Device): " << sizeof(Device) << " byteSize(): " << deviceData[0].byteSize() << std::endl;
*/
	// erase emulated flash
	memset(const_cast<uint8_t*>(Flash::getAddress(0)), 0xff, FLASH_PAGE_COUNT * FLASH_PAGE_SIZE);

	// read flash contents from file
	std::ifstream is("flash.bin", std::ios::binary);
	//is.read(reinterpret_cast<char*>(const_cast<uint8_t*>(Flash::getAddress(0))), Flash::PAGE_COUNT * Flash::PAGE_SIZE);
	is.close();

	// get device from command line
	//std::string device = argv[1];
	
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

	// emulator user interface
	Gui gui;

	// set global variables
	boost::system::error_code ec;
	asio::ip::address localhost = asio::ip::address::from_string("::1", ec);
	global::local = asio::ip::udp::endpoint(asio::ip::udp::v6(), 1337);
	global::upLink = asio::ip::udp::endpoint(localhost, 47193);
	global::gui = &gui;

	// the room control application
	RoomControl roomControl;

	// add test data
	for (auto r : routeData) {
		RoomControl::Route route = {};
		route.setSrcTopic(r[0]);
		route.setDstTopic(r[1]);
		roomControl.routes.write(roomControl.routes.size(), &route);
	}
	for (auto t : timerData) {
		RoomControl::Timer timer = {};
		timer.time = t.time;
		timer.commandCount = 1;
		RoomControl::Command &command = timer.u.commands[0];
		uint8_t *data = timer.begin();
		command.type = RoomControl::Command::BINARY;
		data[0] = 1;
		command.setTopic(t.topic, data + 1, timer.end());
		roomControl.timers.write(roomControl.timers.size(), &timer);
	}

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
			
			// temperature sensor
			gui.temperatureSensor(id++);

			gui.newLine();
						
			roomControl.doGui(id);
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

	// write flash
	std::ofstream os("flash.bin", std::ios::binary);
	os.write(reinterpret_cast<const char*>(Flash::getAddress(0)), FLASH_PAGE_COUNT * FLASH_PAGE_SIZE);
	os.close();

	// cleanup
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
