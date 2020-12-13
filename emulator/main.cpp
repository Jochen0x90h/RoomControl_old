
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

struct DeviceData {
	uint32_t deviceId;
	String name;
	
	struct Component {
		RoomControl::Device::Component::Type type;
		uint8_t endpointIndex;
		uint8_t elementIndex;
		SystemDuration duration;
	};
	
	int componentCount;
	Component components[8];
};

// matches to emulated devices in Bus.cpp
constexpr DeviceData deviceData[] = {
	{0x00000001, "switch1", 7, {
		{RoomControl::Device::Component::ROCKER, 0, 0},
		{RoomControl::Device::Component::BUTTON, 1, 0},
		{RoomControl::Device::Component::DELAY_BUTTON, 1, 0, 2s},
		{RoomControl::Device::Component::SWITCH, 1, 1},
		{RoomControl::Device::Component::RELAY, 2, 0},
		{RoomControl::Device::Component::TIME_RELAY, 3, 0, 3s},
		{RoomControl::Device::Component::RELAY, 4, 0}
	}},
	{0x00000002, "switch2", 6, {
		{RoomControl::Device::Component::ROCKER, 0, 0},
		{RoomControl::Device::Component::BUTTON, 1, 0},
		{RoomControl::Device::Component::HOLD_ROCKER, 2, 0, 2s},
		{RoomControl::Device::Component::HOLD_BUTTON, 3, 0, 2s},
		{RoomControl::Device::Component::BLIND, 4, 0, 6500ms},
		{RoomControl::Device::Component::BLIND, 5, 0, 6500ms}}},
	{0x00000003, "tempsensor", 1, {
		{RoomControl::Device::Component::CELSIUS, 0, 0}}},
};

constexpr String routeData[][2] = {
	{"room/switch1/rk0", "room/switch1/rl0"},
	{"room/switch1/bt0", "room/switch1/rl1"},
	{"room/switch1/bt1", "room/switch1/rl2"},
	{"room/switch1/sw0", "room/switch1/rl2"},
	{"room/switch2/rk0", "room/switch2/bl0"},
	{"room/switch2/rk1", "room/switch2/bl0"},
	{"room/switch2/bt0", "room/switch2/bl1"},
	{"room/switch2/bt1", "room/switch2/bl1"}
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
 * Emulator main, start without parameters
 */
int main(int argc, const char **argv) {

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


	void (RoomControl::*member)(uint8_t endpointId, uint8_t const *data, int length);
	member = &RoomControl::busSend;
	std::cout << sizeof(member) << std::endl;
	//(*this.*member)(...);

	std::function<void (uint8_t, uint8_t const *, int)> func = [&roomControl] (uint8_t endpointId, uint8_t const *data, int length) {roomControl.busSend(endpointId, data, length);};
	std::cout << sizeof(func) << std::endl;

	// add test data
	for (auto d : deviceData) {
		RoomControl::Device device = {};
		RoomControl::DeviceState deviceState = {};
		device.deviceId = d.deviceId;
		device.setName(d.name);
		
		// add components
		RoomControl::ComponentEditor editor(device, deviceState);
		//int lastEndpointIndex = 0;
		//int componentIndex = 0;
		for (int i = 0; i < d.componentCount; ++i) {
			auto &componentData = d.components[i];
			auto type = componentData.type;
			auto &component = editor.insert(type);

			//auto endpointIndex = componentData.endpointIndex;
			/*if (endpointIndex != lastEndpointIndex) {
				lastEndpointIndex = endpointIndex;
				componentIndex = 0;
			}*/
			component.endpointIndex = componentData.endpointIndex;
			//component.componentIndex = componentIndex++;
			component.nameIndex = device.getNameIndex(type, i);
			component.elementIndex = componentData.elementIndex;
			
			if (component.is<RoomControl::Device::TimeComponent>()) {
				auto &timeComponent = component.cast<RoomControl::Device::TimeComponent>();
				timeComponent.duration = componentData.duration;
			}
			
			editor.next();
		}
		roomControl.devices.write(roomControl.devices.size(), &device);
	
		// debug print devices
		auto e = roomControl.devices[roomControl.devices.size() - 1];
		for (RoomControl::ComponentIterator it(*e.flash, *e.ram); !it.atEnd(); it.next()) {
			auto &component = it.getComponent();
			StringBuffer<8> b = component.getName();
			std::cout << b << std::endl;
		}
	}
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
		command.type = RoomControl::Command::SWITCH;
		data[0] = 1;
		command.setTopic(t.topic, data + 1, timer.end());
		roomControl.timers.write(roomControl.timers.size(), &timer);
	}
	roomControl.subscribeAll();

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
			//gui.temperatureSensor(id++);

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
