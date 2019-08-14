#include "Display.hpp"
#include "Poti.hpp"
#include "Light.hpp"
#include "Blind.hpp"
#include "glad/glad.h"
#include <GLFW/glfw3.h> // http://www.glfw.org/docs/latest/quick_guide.html
#include <boost/asio/io_context.hpp>
#include <boost/asio/serial_port.hpp>
#include <boost/asio/write.hpp>
#include <iostream>
#include <vector>
#include <chrono>
#include <thread>

// fonts
#include "tahoma_8pt.hpp"


namespace asio = boost::asio;
using spb = asio::serial_port_base;
using boost::system::error_code;


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

Bitmap<128, 64> bitmap;

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
			if (this->activeWidget != nullptr)
				this->activeWidget->release();
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

class Serial {
public:
	using Device = const std::string&;
	
	Serial(Device device) : tty(context) {
		this->tty.open(device);

		// set 115200 8N1
		this->tty.set_option(spb::baud_rate(57600));
		this->tty.set_option(spb::character_size(8));
		this->tty.set_option(spb::parity(spb::parity::none));
		this->tty.set_option(spb::stop_bits(spb::stop_bits::one));
		this->tty.set_option(spb::flow_control(spb::flow_control::none));
	}

	void send(const uint8_t *data, int length) {
		this->sentCount = 0;
		asio::async_write(
			this->tty,
			asio::buffer(data, length),
			[this] (error_code error, size_t writtenCount) {
				if (error) {
					// error
					this->sentCount = -1;
				} else {
					// ok
					this->sentCount = writtenCount;
				}
			});
	}
	
	// return number of bytes sent, -1 on error
	int getSentCount() {
		return this->sentCount;
	}
	
	void receive(uint8_t *data, int length) {
		this->receivedCount = 0;
		this->tty.async_read_some(
			asio::buffer(data, length),
			[this] (error_code error, size_t readCount) {
				if (error) {
					// error
					this->receivedCount = -1;
				} else {
					// ok
					this->receivedCount = readCount;
				}
			});
	}
	
	// return number of bytes received, -1 on error
	int getReceivedCount() {
		return this->receivedCount;
	}
	
	void update() {
		this->context.poll();
		if (this->context.stopped())
			this->context.reset();
	}

protected:

	asio::io_context context;
	asio::serial_port tty;
	
	int sentCount = 0;
	int receivedCount = 0;
};


class Timer {
public:
	
	Timer() {
		this->start = std::chrono::steady_clock::now();
	}

	/**
	 * timer value in milliseconds
	 */
	uint32_t getValue() {
		return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - this->start).count();
	}
	
protected:
	std::chrono::steady_clock::time_point start;
};
Timer timer;


inline int abs(int x) {return x > 0 ? x : -x;}

class Actors {
public:
	// number of actors
	static const int ACTOR_COUNT = 8;

	Actors() {
		for (int i = 0; i < ACTOR_COUNT; ++i) {
			this->speed[i] = 65536;
			this->targetValues[i] = 0;
			this->currentValues[i] = 0;
			this->running[i] = false;
		}
	}

	void setSpeed(int actorIndex, int speed) {
		this->speed[actorIndex] = speed;
	}

	/**
	 * Set scenario to actors with a dim level in the range 0 - 100 for each actor
	 * @param scenario array of dim levels, one for each actor
	 */
	void setValues(const uint8_t *values) {
		for (int i = 0; i < ACTOR_COUNT; ++i) {
			if (values[i] <= 100) {
				this->targetValues[i] = values[i] << 16;
				this->running[i] = true;
			}
		}
	}

	/**
	 * Get the current value of an actor
	 */
	uint8_t getCurrentValue(int i) {
		return this->currentValues[i] >> 16;
	}

	/**
	 * Stop any moving actor for which the value is valid and return true if at least one actor was stopped
	 */
	bool stop(const uint8_t *values) {
		bool stopped = false;
		for (int i = 0; i < ACTOR_COUNT; ++i) {
			if (values[i] <= 100) {
				if (this->running[i]) {
					this->running[i] = false;
					stopped = true;
				}
			}
		}
		return stopped;
	}

	int getSimilarity(const uint8_t *values) {
		int similarity = 0;
		for (int i = 0; i < ACTOR_COUNT; ++i) {
			if (values[i] <= 100) {
				similarity += abs((this->targetValues[i] >> 16) - values[i]);
			}
		}
		return similarity;
	}

	void update() {
		uint32_t time = timer.getValue();
		
		// elapsed time in milliseconds
		uint32_t d = time - this->time;
		
		// adjust current value
		for (int i = 0; i < ACTOR_COUNT; ++i) {
			if (this->running[i]) {
				int step = d * this->speed[i];
				int targetValue = this->targetValues[i];
				int &currentValue = this->currentValues[i];
				if (targetValue > currentValue) {
					currentValue += step;
					if (currentValue >= targetValue) {
						currentValue = targetValue;
						this->running[i] = false;
					}
				} else {
					currentValue -= step;
					if (currentValue <= targetValue) {
						currentValue = targetValue;
						this->running[i] = false;
					}
				}
			}
		}
		
		this->time = time;
	}

protected:

	int speed[ACTOR_COUNT];

	// target states of actors
	int targetValues[ACTOR_COUNT];
	
	// current states of actors (simulated, not measured)
	int currentValues[ACTOR_COUNT];

	bool running[ACTOR_COUNT];
	
	// last time for determining a time step
	uint32_t time = 0;
};
Actors actors;


class Configuration {
public:

	// maximum number of scenarios per switch that can be cycled
	static const int SCENARIO_COUNT = 8;


	/**
	 * Find the scenarios for the node and state (switch)
	 */
	const uint8_t *findScenarios(uint32_t nodeId, uint8_t state) {
		static const uint8_t scenarios[][SCENARIO_COUNT] = {
			{0, 1, 0xff}, // lights
			{2, 3, 4, 0xff},
			{5, 6, 0xff}, // blinds
			{7, 8, 9, 0xff}
		};
		if (nodeId == 0xfef3ac9b) {
			if (state == 0x10) // bottom left
				return scenarios[0];
			if (state == 0x30) // top left
				return scenarios[1];
			if (state == 0x50) // bottom right
				return scenarios[2];
			if (state == 0x70) // top right
				return scenarios[3];
		}
		return nullptr;
	}

	struct Scenario {
		char name[16];
		uint8_t actorValues[Actors::ACTOR_COUNT];
	};
	

	/**
	 * Get the names for a scenario
	 */
	const char *getScenarioName(uint8_t scenario) {
		return scenarios[scenario].name;
	}

	/**
	 * Get the actor states for a scenario
	 */
	const uint8_t *getActorValues(int scenario) {
		return scenarios[scenario].actorValues;
	}
	
	static const Scenario scenarios[10];
};
const Configuration::Scenario Configuration::scenarios[10] = {
	{"Light1 On", {100, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{"Light1 Off", {0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{"Light2 On", {0xff, 100, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{"Light2 Dim", {0xff, 50, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{"Light2 Off", {0xff, 0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{"Blind1 Up", {0xff, 0xff, 0xff, 0xff, 100, 0xff, 0xff, 0xff}},
	{"Blind1 Down", {0xff, 0xff, 0xff, 0xff, 0, 0xff, 0xff, 0xff}},
	{"Blind2 Up", {0xff, 0xff, 0xff, 0xff, 0xff, 100, 0xff, 0xff}},
	{"Blind2 Mid", {0xff, 0xff, 0xff, 0xff, 0xff, 50, 0xff, 0xff}},
	{"Blind2 Down", {0xff, 0xff, 0xff, 0xff, 0xff, 0, 0xff, 0xff}},
};
Configuration configuration;


// menu
class Menu {
public:

	enum State {
		IDLE,
		EVENT,
	};


	void show() {
		bitmap.clear();
		

		switch (this->state) {
		case IDLE:
			break;
		case EVENT:
			{
				const char *name = this->event;
				int y = 10;
				int len = tahoma_8pt.calcWidth(name);
				bitmap.drawText((bitmap.WIDTH - len) >> 1, y, tahoma_8pt, name, Mode::SET);
			
				if (timer.getValue() - this->eventTime > 1000)
					this->state = IDLE;
			}
			break;
		}
	}
	
	void setEvent(const char *event) {
		this->eventTime = timer.getValue();
		this->event = event;
		this->state = EVENT;
	}
	
protected:
	State state = IDLE;

	// event data
	int eventTime;
	const char *event;
};
Menu menu;


// EnOcean

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

	void onPacket(uint8_t packetType, const uint8_t *data, int length,
		const uint8_t *optionalData, int optionalLength);
	
	/// Calculate checksum of EnOcean frame
	uint8_t calcChecksum(const uint8_t *data, int length);

	
	Serial serial;
	
	// recieve buffer
	int rxPosition = 0;
	uint8_t rxBuffer[6 + MAX_DATA_LENGTH + MAX_OPTIONAL_LENGTH + 1];
};


void EnOceanProtocol::update() {
	this->serial.update();
	
	int receivedCount = serial.getReceivedCount();

	if (receivedCount > 0) {
		this->rxPosition += receivedCount;
		
		// check if header is complete
		while (this->rxPosition > 6) {
			// check if header is ok
			if (this->rxBuffer[0] == 0x55 && calcChecksum(this->rxBuffer + 1, 4) == this->rxBuffer[5]) {
				int dataLength = (this->rxBuffer[1] << 8) | this->rxBuffer[2];
				int optionalLength = this->rxBuffer[3];
				int length = dataLength + optionalLength;
				uint8_t packetType = this->rxBuffer[4];
			
				// check if complete frame has arrived
				if (this->rxPosition >= 6 + length + 1) {
					// check if frame is ok
					if (calcChecksum(this->rxBuffer + 6, length) == this->rxBuffer[6 + length]) {
						// cancel timeout and reset retry count
						//this->txTimer.cancel();
						//this->txRetryCount = 0;

						onPacket(packetType, this->rxBuffer + 6, dataLength,
							this->rxBuffer + 6 + dataLength, optionalLength);
						
						// remove frame from buffer
						std::move(this->rxBuffer + 6 + length + 1, this->rxBuffer + rxPosition,
							this->rxBuffer);
						this->rxPosition -= 6 + length + 1;
					} else {
						// error
						this->rxPosition = 0;
					}
				} else {
					// incomplete frame: continue receiving
					break;
				}
			} else {
				// error
				this->rxPosition = 0;
			}
		}

		// continue receiving
		receive();
	}
}

void EnOceanProtocol::receive() {
	this->serial.receive(this->rxBuffer + this->rxPosition, sizeof(this->rxBuffer) - this->rxPosition);
}

void EnOceanProtocol::onPacket(uint8_t packetType, const uint8_t *data, int length,
	const uint8_t *optionalData, int optionalLength)
{
	//std::cout << "onPacket " << length << " " << optionalLength << std::endl;

	if (packetType == RADIO_ERP1) {
		if (length >= 7 && data[0] == 0xf6) {
			
			uint32_t nodeId = (data[2] << 24) | (data[3] << 16) | (data[4] << 8) | data[5];
			int state = data[1];
			std::cout << "nodeId " << std::hex << nodeId << " state " << state << std::dec << std::endl;
			
			// find scenario
			const uint8_t *scenarios = configuration.findScenarios(nodeId, state);
			if (scenarios != nullptr) {
				// find current scenario by comparing similarity to actor target values
				int bestSimilarity = 0x7fffffff;
				int bestI;
				for (int i = 0; i < Configuration::SCENARIO_COUNT && scenarios[i] != 0xff; ++i) {
					uint8_t scenario = scenarios[i];
					const uint8_t *values = configuration.getActorValues(scenario);
					int similarity = actors.getSimilarity(values);
					if (similarity < bestSimilarity) {
						bestSimilarity = similarity;
						bestI = i;
					}
				}
				int currentScenario = scenarios[bestI];
				
				// check if actor (e.g. blind) is still moving
				if (actors.stop(configuration.getActorValues(currentScenario))) {
					// stopped
					menu.setEvent("Stopped");
				} else {
					// next scenario
					int nextI = bestI + 1;
					if (nextI >= Configuration::SCENARIO_COUNT || scenarios[nextI] == 0xff)
						nextI = 0;
					uint8_t nextScenario = scenarios[nextI];

					// set actor values for next scenario
					actors.setValues(configuration.getActorValues(nextScenario));
				
					// display scenario name
					const char *name = configuration.getScenarioName(nextScenario);
					menu.setEvent(name);
				}
			}
		}
		if (optionalLength >= 7) {
			uint32_t destinationId = (optionalData[1] << 24) | (optionalData[2] << 16) | (optionalData[3] << 8) | optionalData[4];
			int dBm = -optionalData[5];
			int security = optionalData[6];
			std::cout << "destinationId " << std::hex << destinationId << std::dec << " dBm " << dBm << " security " << security << std::endl;
		}
	}
	
}

static const uint8_t crc8Table[256] = {0x00, 0x07, 0x0e, 0x09, 0x1c, 0x1b, 0x12, 0x15, 0x38, 0x3f, 0x36, 0x31, 0x24, 0x23, 0x2a, 0x2d, 0x70, 0x77, 0x7e, 0x79, 0x6c, 0x6b, 0x62, 0x65, 0x48, 0x4f, 0x46, 0x41, 0x54, 0x53, 0x5a, 0x5d, 0xe0, 0xe7, 0xee, 0xe9, 0xfc, 0xfb, 0xf2, 0xf5, 0xd8, 0xdf, 0xd6, 0xd1, 0xc4, 0xc3, 0xca, 0xcd, 0x90, 0x97, 0x9e, 0x99, 0x8c, 0x8b, 0x82, 0x85, 0xa8, 0xaf, 0xa6, 0xa1, 0xb4, 0xb3, 0xba, 0xbd, 0xc7, 0xc0, 0xc9, 0xce, 0xdb, 0xdc, 0xd5, 0xd2, 0xff, 0xf8, 0xf1, 0xf6, 0xe3, 0xe4, 0xed, 0xea, 0xb7, 0xb0, 0xb9, 0xbe, 0xab, 0xac, 0xa5, 0xa2, 0x8f, 0x88, 0x81, 0x86, 0x93, 0x94, 0x9d, 0x9a, 0x27, 0x20, 0x29, 0x2e, 0x3b, 0x3c, 0x35, 0x32, 0x1f, 0x18, 0x11, 0x16, 0x03, 0x04, 0x0d, 0x0a, 0x57, 0x50, 0x59, 0x5e, 0x4b, 0x4c, 0x45, 0x42, 0x6f, 0x68, 0x61, 0x66, 0x73, 0x74, 0x7d, 0x7a, 0x89, 0x8e, 0x87, 0x80, 0x95, 0x92, 0x9b, 0x9c, 0xb1, 0xb6, 0xbf, 0xb8, 0xad, 0xaa, 0xa3, 0xa4, 0xf9, 0xfe, 0xf7, 0xf0, 0xe5, 0xe2, 0xeb, 0xec, 0xc1, 0xc6, 0xcf, 0xc8, 0xdd, 0xda, 0xd3, 0xd4, 0x69, 0x6e, 0x67, 0x60, 0x75, 0x72, 0x7b, 0x7c, 0x51, 0x56, 0x5f, 0x58, 0x4d, 0x4a, 0x43, 0x44, 0x19, 0x1e, 0x17, 0x10, 0x05, 0x02, 0x0b, 0x0c, 0x21, 0x26, 0x2f, 0x28, 0x3d, 0x3a, 0x33, 0x34, 0x4e, 0x49, 0x40, 0x47, 0x52, 0x55, 0x5c, 0x5b, 0x76, 0x71, 0x78, 0x7f, 0x6A, 0x6d, 0x64, 0x63, 0x3e, 0x39, 0x30, 0x37, 0x22, 0x25, 0x2c, 0x2b, 0x06, 0x01, 0x08, 0x0f, 0x1a, 0x1d, 0x14, 0x13, 0xae, 0xa9, 0xa0, 0xa7, 0xb2, 0xb5, 0xbc, 0xbb, 0x96, 0x91, 0x98, 0x9f, 0x8a, 0x8D, 0x84, 0x83, 0xde, 0xd9, 0xd0, 0xd7, 0xc2, 0xc5, 0xcc, 0xcb, 0xe6, 0xe1, 0xe8, 0xef, 0xfa, 0xfd, 0xf4, 0xf3};

uint8_t EnOceanProtocol::calcChecksum(const uint8_t *data, int length) {
	uint8_t crc = 0;
	for (int i = 0 ; i < length ; ++i)
		crc = crc8Table[crc ^ data[i]];
	return crc;
}


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
	display.setRect(0.3f, y - 0.2f, 0.4f, 0.2f);

	y -= 0.3f;

	// potis
	Poti poti1;
	layoutManager.add(&poti1);
	poti1.setRect(0.2f, y - 0.25f, 0.25f, 0.25f);
	Poti poti2;
	layoutManager.add(&poti2);
	poti2.setRect(0.55f, y - 0.25f, 0.25f, 0.25f);

	y -= 0.3f;

	// lights
	Light lights[4];
	for (int i = 0; i < 4; ++i) {
		layoutManager.add(&lights[i]);
		const float step = 1.0f / 8.0f;
		const float size = 0.1f;
		lights[i].setRect((step - size) * 0.5f + i * step, y - size, size, size);
	}

	// blinds
	Blind blinds[4];
	for (int i = 0; i < 4; ++i) {
		layoutManager.add(&blinds[i]);
		const float step = 1.0f / 8.0f;
		const float size = 0.1f;
		blinds[i].setRect((step - size) * 0.5f + (i + 4) * step, y - size * 2, size, size * 2);
		actors.setSpeed(i + 4, 2000);
	}

	// timer
	Timer timer;

	// connection to EnOcean controller
	EnOceanProtocol protocol("/dev/tty.usbserial-FT3PMLOR");
	
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
		/*{
			bitmap.clear();

			const char *name = "foo";

			// effect name
			int y = 10;
			int len = tahoma_8pt.calcWidth(name);
			bitmap.drawText((bitmap.WIDTH - len) >> 1, y, tahoma_8pt, name, Mode::SET);
		}*/


		// update
		protocol.update();
		actors.update();
		display.update(bitmap);
		menu.show();
		for (int i = 0; i < 4; ++i) {
			lights[i].setValue(actors.getCurrentValue(i));
			blinds[i].setValue(actors.getCurrentValue(i + 4));
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
