#include "Display.hpp"
#include "Poti.hpp"
#include "Light.hpp"
#include "Blind.hpp"
#include "glad/glad.h"
#include <GLFW/glfw3.h> // http://www.glfw.org/docs/latest/quick_guide.html
#include <boost/asio/io_service.hpp>
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


inline int min(int x, int y) {return x < y ? x : y;}
inline int max(int x, int y) {return x > y ? x : y;}
inline int abs(int x) {return x > 0 ? x : -x;}
inline int clamp(int x, int minval, int maxval) {return x < minval ? minval : (x > maxval ? maxval : x);}
inline int limit(int x, int size) {return x < 0 ? 0 : (x >= size ? size - 1 : x);}


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

static const char *hexTable = "0123456789ABCDEF";

template <int L>
class String {
public:
	String() {}
	
	template <typename T>
	String &operator = (const T &value) {
		this->index = 0;
		return operator , (value);
	}

	template <typename T>
	String &operator += (const T &value) {
		return operator , (value);
	}

	String &operator , (char ch) {
		if (this->index < L)
			this->buffer[this->index++] = ch;
		this->buffer[this->index] = 0;
		return *this;
	}

/*
	String &operator , (const char* s) {
		while (*s != 0 && this->index < L) {
			this->buffer[this->index++] = *(s++);
		}
		this->buffer[this->index] = 0;
		return *this;
	}
*/

	template <int N>
	String &operator , (const char (&array)[N]) {
		for (int i = 0; i < N && this->index < L; ++i) {
			char ch = array[i];
			if (ch == 0)
				break;
			this->buffer[this->index++] = ch;
		}
		this->buffer[this->index] = 0;
		return *this;
	}

	String &operator , (int dec) {
		char buffer[12];

		unsigned int value = dec < 0 ? -dec : dec;

		char * b = buffer + 11;
		*b = 0;
		do {
			--b;
			*b = '0' + value % 10;
			value /= 10;
		} while (value > 0);

		if (dec < 0) {
			--b;
			*b = '-';
		}

		// append the number
		while (*b != 0 && this->index < L) {
			this->buffer[this->index++] = *(b++);
		}
		this->buffer[this->index] = 0;
		return *this;
	}

	String &operator , (uint8_t hex) {
		if (this->index < L)
			this->buffer[this->index++] = hexTable[hex >> 4];
		if (this->index < L)
			this->buffer[this->index++] = hexTable[hex & 0xf];
		this->buffer[this->index] = 0;
		return *this;
	}

	String &operator , (uint32_t hex) {
		for (int i = 28; i >= 0 && this->index < L; i -= 4) {
			this->buffer[this->index++] = hexTable[(hex >> i) & 0xf];
		}
		this->buffer[this->index] = 0;
		return *this;
	}

	operator const char * () {
		return this->buffer;
	}
protected:
	char buffer[L + 1];
	int index = 0;
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

	asio::io_service context;
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





class Flash {
public:
	static const int PAGE_SIZE = 1024;
	static const int WRITE_ALIGN = 2;

	Flash(int pageCount) {
		this->data = new uint8_t[pageCount * PAGE_SIZE];
		std::fill(this->data, this->data + pageCount * PAGE_SIZE, 0xff);
	}

	void *getAddress(int pageIndex, int offset = 0) {
		return this->data + pageIndex * PAGE_SIZE + offset;
	}
	
	void erase(int pageIndex) {
		uint8_t *page = this->data + pageIndex * PAGE_SIZE;
		std::fill(page, page + PAGE_SIZE, 0xff);
	}
	
	void write(int pageIndex, int offset, const void * data, int length) {
		uint8_t *page = this->data + pageIndex * PAGE_SIZE;
		const uint8_t *d = (uint8_t*)data;
		std::copy(d, d + length, page + offset);
	}
	
protected:

	uint8_t *data;
};
Flash flash(16);


template <typename ELEMENT, int COUNT>
class Storage {
public:
	static const int PAGE_SHIFT = 8;
	static const int INDEX_MASK = 0xff;

	// index update that tracks changes to the index such as insert and erase
	struct Update {
		uint16_t index;
		uint16_t index2;
	};

	// index update size and number of index updates that fit into a page, taking alignment of flash into account
	static const int UPDATE_SIZE = (sizeof(Update) + Flash::WRITE_ALIGN - 1) & ~(Flash::WRITE_ALIGN - 1);
	static const int PAGE_UPDATE_COUNT = Flash::PAGE_SIZE / UPDATE_SIZE;

	// element size and number of elements that fit into a page, taking alignment of flash into account
	static const int ELEMENT_SIZE = (sizeof(ELEMENT) + Flash::WRITE_ALIGN - 1) & ~(Flash::WRITE_ALIGN - 1);
	static const int PAGE_ELEMENT_COUNT = Flash::PAGE_SIZE / ELEMENT_SIZE;

	// number of flash pages to store elements. Have at least 1.5 free pages
	static const int ELEMENT_PAGE_COUNT = (COUNT + (PAGE_ELEMENT_COUNT * 5) / 2) / PAGE_ELEMENT_COUNT;


	int init(int pageStart) {
		this->indexPageStart = pageStart;
		this->elementPageStart = pageStart + 2;
		
		// determine which index page is current
		uint16_t *indexPage1 = (uint16_t*)flash.getAddress(this->indexPageStart);
		uint16_t *indexPage2 = (uint16_t*)flash.getAddress(this->indexPageStart + 1);
		int last1 = -1;
		int last2 = -1;
		for (int i = 0; i < Flash::PAGE_SIZE / 2; ++i) {
			if (indexPage1[i] != 0xffff)
				last1 = i;
			if (indexPage2[i] != 0xffff)
				last2 = i;
		}
		if (last1 >= last2) {
			this->indexIndex = 0;
			if (last2 != -1) {
				// erase other page if it is not empty (just to be sure, should always be empty)
				flash.erase(this->indexPageStart + 1);
			}
		} else {
			this->indexIndex = 1 << PAGE_SHIFT;
			if (last1 != -1) {
				// erase other page if it is not empty (just to be sure, should always be empty)
				flash.erase(this->indexPageStart);
			}
		}

		// copy indices from beginning of page
		uint8_t *indexPage = (uint8_t*)flash.getAddress(this->indexPageStart + (this->indexIndex >> PAGE_SHIFT));
		uint16_t *elementIndices = (uint16_t*)indexPage;
		int ii = 0;
		while (ii < COUNT) {
			uint16_t elementIndex = this->elementIndices[ii] = elementIndices[ii];
			++ii;
			if (elementIndex == 0xffff)
				break;
		}
		
		// apply index updates
		int updateStart = (ii * 2 + UPDATE_SIZE - 1) / UPDATE_SIZE;
		for (int updateIndex = updateStart; updateIndex < PAGE_UPDATE_COUNT; ++updateIndex) {
			Update *update = (Update*)(indexPage + updateIndex * UPDATE_SIZE);
			if (update->index < COUNT) {
				// set element index
				this->elementIndices[update->index] = update->index2;
			} else if (update->index < COUNT * 2) {
				if (update->index2 < COUNT) {
					// move element
					
					//!
				} else {
					// delete element
					for (int i = update->index + 1; i < COUNT; ++i) {
						this->elementIndices[i - 1] = this->elementIndices[i];
					}
					this->elementIndices[COUNT - 1] = 0xffff;
				}
			} else if (update->index == COUNT * 2) {
				// move element page
				int	srcPageIndex = update->index2;
				int dstPageIndex = (srcPageIndex == 0 ? ELEMENT_PAGE_COUNT : srcPageIndex) - 1;
				int newIndex = 0;
				for (int i = 0; i < COUNT; ++i) {
					int pageIndex = this->elementIndices[i] >> PAGE_SHIFT;
					if (pageIndex == srcPageIndex) {
						this->elementIndices[i] = (dstPageIndex << PAGE_SHIFT) + newIndex;
						++newIndex;
					}
				}
				
			} else {
				// entry is invalid: end of updates
				this->indexIndex |= updateIndex;
				goto indexNotFull;
			}
		}
		moveIndex();
indexNotFull:
		
		// determine count
		int count;
		for (count = 0; count < COUNT && this->elementIndices[count] != 0xffff; ++count);
		this->count = count;
		

		// determine which element page is current
		for (int i = 0; i < ELEMENT_PAGE_COUNT; ++i) {
			if (isEmpty(i)) {
				// found empty page: use page before as current page
				this->elementIndex = ((i == 0 ? ELEMENT_PAGE_COUNT : i) - 1) << PAGE_SHIFT;
				break;
			}
		}

		// determine next free element index in current element page
		int last = getLastIndex(this->elementIndex >> PAGE_SHIFT);
		
		// check if element page is full
		if (last >= PAGE_ELEMENT_COUNT - 1)
			moveElements();
		else
			this->elementIndex |= last + 1;
	
		return 2 + ELEMENT_PAGE_COUNT;
	}
	
	int getCount() const {
		return this->count;
	}
	
	const ELEMENT &get(int index) const {
		uint16_t elementIndex = this->elementIndices[index];
		int pageIndex = this->elementPageStart + (elementIndex >> PAGE_SHIFT);
		void * entry = flash.getAddress(pageIndex, (elementIndex & INDEX_MASK) * ELEMENT_SIZE);
		return *(ELEMENT*)entry;
	}
	
	void write(int index, const ELEMENT &element) {
		if (index == this->count && index < COUNT)
			this->count = index + 1;
		if (index < this->count) {
			// write new entry at current free position
			{
				int indexPage = this->elementPageStart + (this->elementIndex >> PAGE_SHIFT);
				flash.write(indexPage, (this->elementIndex & INDEX_MASK) * ELEMENT_SIZE, &element, sizeof(ELEMENT));
			}

			// write index update
			{
				int indexPage = this->indexPageStart + (this->indexIndex >> PAGE_SHIFT);
				Update u{uint16_t(index), this->elementIndex};
				flash.write(indexPage, (this->indexIndex & INDEX_MASK) * UPDATE_SIZE, &u, sizeof(Update));
			}

			// increment index index and check if page is full
			if ((this->indexIndex & INDEX_MASK) >= PAGE_UPDATE_COUNT - 1) {
				moveIndex();
			} else {
				++this->indexIndex;
			}

			// set element index
			this->elementIndices[index] = this->elementIndex;

			// increment element index and check if page is full
			if ((this->elementIndex & INDEX_MASK) >= PAGE_ELEMENT_COUNT - 1) {
				moveElements();
			} else {
				++this->elementIndex;
			}
		}
	}
	
	void move(int index, int newIndex) {
	
	}
	
	void erase(int index) {
		if (index < this->count) {
			for (int i = index + 1; i < this->count; ++i) {
				this->elementIndices[i - 1] =  this->elementIndices[i];
			}
			--this->count;

			// write index update
			{
				int pageIndex = this->indexPageStart + (this->indexIndex >> PAGE_SHIFT);
				Update u{uint16_t(index), COUNT};
				flash.write(pageIndex, (this->indexIndex & INDEX_MASK) * UPDATE_SIZE, &u, sizeof(Update));
			}

			// increment index index and check if page is full
			if ((this->indexIndex & INDEX_MASK) >= PAGE_UPDATE_COUNT - 1) {
				moveIndex();
			} else {
				++this->indexIndex;
			}
		}
	}
	
protected:

	bool isEmpty(int elementPageIndex) {
		// check if page is referenced by an element
		for (int i = 0; i < this->count; ++i) {
			if ((this->elementIndices[i] >> PAGE_SHIFT) == elementPageIndex)
				return false;
		}
		
		// no: check if element page is actually empty (just to be sure, should always be empty)
		uint32_t *page = (uint32_t*)flash.getAddress(this->elementPageStart + elementPageIndex);
		for (int i = 0; i < Flash::PAGE_SIZE / 4; ++i) {
			if (page[i] != 0xffffffff) {
				flash.erase(this->elementPageStart + elementPageIndex);
				break;
			}
		}
		return true;
	}

	int getLastIndex(int elementPageIndex) {
		// check if page is referenced by an element
		int last = -1;
		for (int i = 0; i < this->count; ++i) {
			int elementIndex = this->elementIndices[i];
			if ((elementIndex >> PAGE_SHIFT) == elementPageIndex)
				last = max(last, elementIndex & INDEX_MASK);
		}
		return last;
	}

	void moveIndex() {
		int srcIndex = (this->indexIndex >> PAGE_SHIFT);
		int dstIndex = srcIndex ^ 1;
		
		// write element indices to destination page
		flash.write(this->indexPageStart + dstIndex, 0, this->elementIndices, COUNT * 2);
		
		// erase source page
		flash.erase(this->indexPageStart + srcIndex);
	
		// set current pate index
		int ii = (min(this->count + 1, COUNT) * 2 + UPDATE_SIZE - 1) / UPDATE_SIZE;
		this->indexIndex = (dstIndex << PAGE_SHIFT) | ii;
	}
	
	void moveElements() {
		int pageIndex = (this->elementIndex >> PAGE_SHIFT);
		int dstPageIndex = pageIndex == ELEMENT_PAGE_COUNT ? 0 : pageIndex + 1;
		int srcPageIndex = dstPageIndex == ELEMENT_PAGE_COUNT ? 0 : dstPageIndex + 1;
		
		int dstPage = this->elementPageStart + dstPageIndex;
		int srcPage = this->elementPageStart + srcPageIndex;
		uint8_t *srcData = (uint8_t*)flash.getAddress(srcPage);
		
		// copy elements from source to destination page
		int newElementIndex = 0;
		for (int i = 0; i < this->count; ++i) {
			int elementIndex = this->elementIndices[i];
			if ((elementIndex >> PAGE_SHIFT) == srcPageIndex) {
				flash.write(dstPage, newElementIndex * ELEMENT_SIZE,
					srcData + (elementIndex & INDEX_MASK) * ELEMENT_SIZE,
					sizeof(ELEMENT));
				++newElementIndex;
			}
		}
	
		// write index update
		{
			int indexPage = this->indexPageStart + (this->indexIndex >> PAGE_SHIFT);
			Update u{COUNT * 2, uint16_t(srcPageIndex)};
			flash.write(indexPage, (this->indexIndex & INDEX_MASK) * UPDATE_SIZE, &u, sizeof(Update));
		}

		// increment index index and check if page is full
		if ((this->indexIndex & INDEX_MASK) >= PAGE_UPDATE_COUNT - 1) {
			moveIndex();
		} else {
			++this->indexIndex;
		}

		// erase source page
		flash.erase(this->indexPageStart + srcPageIndex);


		// set current pate index
		this->elementIndex = dstPageIndex << PAGE_SHIFT;
	}
	
	int indexPageStart;
	int elementPageStart;

	uint16_t elementIndices[COUNT];
	uint16_t count;
	
	// current write indices
	uint16_t indexIndex;
	uint16_t elementIndex;
};


static const int ACTOR_COUNT = 8;
static const int SCENARIO_COUNT = 64;
static const int BUTTON_COUNT = 64;
static const int TIMER_COUNT = 64;

struct Actor {
	enum class Type : uint8_t {
		SWITCH,
		BLIND
	};

	char name[16];
	
	// initial value of actor (0-100)
	uint8_t value;
	
	// type of actor (switch, blind)
	Type type;
	
	// speed to reach target value
	uint16_t speed;
	
	// output index
	uint8_t outputIndex;
};

struct Button {
	// maximum number of scenarios per button that can be cycled
	static const int SCENARIO_COUNT = 8;

	// enocean id of button
	uint32_t id;
	
	// button state
	uint8_t state;
	
	// ids of scenarios that are cycled when pressing the button
	uint8_t scenarios[SCENARIO_COUNT];

	int getScenarioCount() const {
		for (int i = 0; i < SCENARIO_COUNT; ++i) {
			if (this->scenarios[i] == 0xff)
				return i;
		}
		return SCENARIO_COUNT;
	}
	
	int getScenario(int index) const {
		return this->scenarios[index];
	}
};

struct Scenario {
	uint8_t id;
	char name[16];
	uint8_t actorValues[ACTOR_COUNT];
};

Storage<Actor, ACTOR_COUNT> actors;
Storage<Scenario, SCENARIO_COUNT> scenarios;
Storage<Button, BUTTON_COUNT> buttons;
//timers


Actor actorsData[] = {
	{"Light1", 0, Actor::Type::SWITCH, 0, 0},
	{"Light2", 0, Actor::Type::SWITCH, 0, 1},
	{"Light3", 0, Actor::Type::SWITCH, 0, 2},
	{"Light4", 0, Actor::Type::SWITCH, 0, 3},
	{"Blind1", 0, Actor::Type::BLIND, 2000, 4},
	{"Blind2", 0, Actor::Type::BLIND, 2000, 6},
	{"Blind3", 0, Actor::Type::BLIND, 2000, 8},
	{"Blind4", 0, Actor::Type::BLIND, 2000, 10},
};

Scenario scenariosData[] = {
	{0, "Light1 On", {100, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{1, "Light1 Off", {0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{2, "Light2 On", {0xff, 100, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{3, "Light2 Dim", {0xff, 50, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{4, "Light2 Off", {0xff, 0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
	{5, "Blind1 Up", {0xff, 0xff, 0xff, 0xff, 100, 0xff, 0xff, 0xff}},
	{6, "Blind1 Down", {0xff, 0xff, 0xff, 0xff, 0, 0xff, 0xff, 0xff}},
	{7, "Blind2 Up", {0xff, 0xff, 0xff, 0xff, 0xff, 100, 0xff, 0xff}},
	{8, "Blind2 Mid", {0xff, 0xff, 0xff, 0xff, 0xff, 50, 0xff, 0xff}},
	{9, "Blind2 Down", {0xff, 0xff, 0xff, 0xff, 0xff, 0, 0xff, 0xff}},
};

Button buttonsData[] = {
	{0xfef3ac9b, 0x10, {0, 1, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}}, // bottom left
	{0xfef3ac9b, 0x30, {2, 3, 4, 0xff, 0xff, 0xff, 0xff, 0xff}}, // top left
	{0xfef3ac9b, 0x50, {5, 6, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}}, // bottom right
	{0xfef3ac9b, 0x70, {7, 8, 9, 0xff, 0xff, 0xff, 0xff, 0xff}}, // top right
};

const Button *findButton(uint32_t id, uint8_t state) {
	for (int i = 0; i < buttons.getCount(); ++i) {
		const Button &button = buttons.get(i);
		if (button.id == id && button.state == state)
			return &button;
	}
	return nullptr;
}

const Scenario *findScenario(uint8_t id) {
	for (int i = 0; i < scenarios.getCount(); ++i) {
		const Scenario &scenario = scenarios.get(i);
		if (scenario.id == id)
			return &scenario;
	}
	return nullptr;
}


class ActorValues {
public:

	ActorValues() {
		for (int i = 0; i < ACTOR_COUNT; ++i) {
			this->speed[i] = 0;
			this->targetValues[i] = 0;
			this->values[i] = 0;
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
	void set(const uint8_t *values) {
		for (int i = 0; i < ACTOR_COUNT; ++i) {
			int value = values[i];
			if (value <= 100) {
				if (this->speed[i] == 0) {
					// set value immediately
					this->targetValues[i] = this->values[i] = value << 16;
				} else {
					// interpolate to value
					this->values[i] = clamp(this->values[i], 0, 100 << 16);
					this->targetValues[i] = (value == 0 ? -2 : (value == 100 ? 102 : value)) << 16;
					this->running[i] = true;
				}
			}
		}
	}

	/**
	 * Get the current value of an actor
	 */
	uint8_t get(int index) {
		return clamp(this->values[index] >> 16, 0, 100);
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
				similarity += abs(clamp(this->targetValues[i] >> 16, 0, 100) - values[i]);
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
				int &currentValue = this->values[i];
				if (currentValue < targetValue) {
					// up
					currentValue += step;
					if (currentValue >= targetValue) {
						currentValue = targetValue;
						this->running[i] = false;
					}
				} else {
					// down
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

	int getSwitchState(int index) {
		return this->values[index] >= (50 << 16) ? 1 : 0;
	}
	
	int getBlindState(int index) {
		if (this->running[index]) {
			int targetValue = this->targetValues[index];
			int currentValue = this->values[index];
			if (currentValue < targetValue) {
				// up
				return 2;
			} else if (currentValue > targetValue) {
				// down
				return 1;
			}
		}
		return 0;
	}

protected:

	int speed[ACTOR_COUNT];

	// target states of actors
	int targetValues[ACTOR_COUNT];
	
	// current states of actors (simulated, not measured)
	int values[ACTOR_COUNT];

	bool running[ACTOR_COUNT];
	
	// last time for determining a time step
	uint32_t time = 0;
};
ActorValues actorValues;


// menu
class Menu {
public:
	/*
		Lights
			Light1
				Name: Light1
				<id> <state>: (push-button | on | off)
		Blinds
			Blind1
				Name: Blind1
				<id> <state>: (push-button | up | down)
		Scenarios
	 		Scenario1
	 			Name: Scenario1
				Light1: (- | off | on | 1-99)
				Light2: (- | off | on | 1-99)
				Blind1: (- | off | on | 1-99)
				Blind2: (- | off | on | 1-99)
				Exit
	 		Scenario2
	 		New
			Exit
	 	Buttons
	 		<name> <id/state>
	 			Name: <name>
	 			Button: <id/state>
				Scenario1: <scenario>
				New Scenario
	 			Delete Button
				Exit
	 		New button
			Exit
		Timers
			<name> <time>
				Name: <name>
				Time: <time>
				Scenario1: <scenario>
				Add Scenario
	 			Delete Timer
				Exit
			New
			Exit
	*/


	// menu state
	enum State {
		IDLE,
		TOAST,
		MAIN,
		ACTORS,
		SCENARIOS,
		EDIT_SCENARIO,
		ADD_SCENARIO,
		BUTTONS,
		EDIT_BUTTON,
		ADD_BUTTON,
		TIMERS,
		SELECT_SCENARIO
	};

	State getState() {return this->state;}

	void update(int delta, bool press) {
		
		if (press) {
			switch (this->state) {
			case IDLE:
				push(MAIN);
				break;
			case MAIN:
				if (this->selected == 0)
					push(ACTORS);
				else if (this->selected == 1)
					push(SCENARIOS);
				else if (this->selected == 2)
					push(BUTTONS);
				else if (this->selected == 3)
					push(TIMERS);
				else {
					// exit
					pop();
				}
				break;
			case ACTORS:
				{
					int actorCount = actors.getCount();
					if (this->selected < actorCount) {
					
					} else {
						// exit
						pop();
					}
				}
				break;
			case SCENARIOS:
				{
					int scenarioCount = scenarios.getCount();
					if (this->selected < scenarioCount) {
						// edit scenario
						this->scenario = scenarios.get(this->selected);
						push(EDIT_SCENARIO);
					} else if (this->selected == scenarioCount) {
						// new scenario
						
					} else {
						// exit
						pop();
					}
				}
				break;
			case EDIT_SCENARIO:
			case ADD_SCENARIO:
				{
					const int actorCount = actors.getCount();
					if (this->selected == 0) {
						// edit name
					} else if (this->selected < 1 + actorCount) {
						// edit value
					} else {
						// exit
						pop();
					}
				}
				break;
			case BUTTONS:
				{
					int buttonCount = buttons.getCount();
					if (this->selected < buttonCount) {
						// edit button
						this->button = buttons.get(this->selected);
						push(EDIT_BUTTON);
					} else if (this->selected == buttonCount) {
						// new button
						this->button.id = 0;
						this->button.state = 0;
						for (int i = 0; i < Button::SCENARIO_COUNT; ++i)
							this->button.scenarios[i] = 0xff;
						push(ADD_BUTTON);
					} else {
						// exit
						pop();
					}
				}
				break;
			case EDIT_BUTTON:
			case ADD_BUTTON:
				{
					int scenarioCount = this->button.getScenarioCount();
					if (this->selected >= 1 && this->selected < 1 + scenarioCount) {
						// select scenario
						int scenarioIndex = button.scenarios[this->selected - 1];
						push(SELECT_SCENARIO);
						this->selected = scenarioIndex;
					} else if (this->selected == 1 + scenarioCount) {
						// add scenario
						push(SELECT_SCENARIO);
						this->selected = scenarioCount == 0 ? 0 : button.scenarios[scenarioCount - 1];
					} else if (this->selected == 1 + scenarioCount + 1) {
						// delete button
						buttons.erase(getThisIndex());
						pop();
					} else {
						// exit
						buttons.write(getThisIndex(), this->button);
						pop();
					}
				}
				break;
			case SELECT_SCENARIO:
				{
					int scenarioIndex = this->selected;
					pop();
					if (scenarioIndex < scenarios.getCount()) {//configuration.getScenarioCount()) {
						// set new scenario index to button
						button.scenarios[this->selected - 1] = scenarioIndex;
						++this->selected;
					} else {
						// remove scenario from button
						for (int i = this->selected - 1; i < Button::SCENARIO_COUNT - 1; ++i) {
							button.scenarios[i] = button.scenarios[i + 1];
						}
						button.scenarios[Button::SCENARIO_COUNT - 1] = 0xff;
					}
				}
				break;
			case TIMERS:
				break;
			}
		}

		// draw menu
		bitmap.clear();
		switch (this->state) {
		case IDLE:
			break;
		case TOAST:
			{
				const char *name = this->string;
				int y = 10;
				int len = tahoma_8pt.calcWidth(name);
				bitmap.drawText((bitmap.WIDTH - len) >> 1, y, tahoma_8pt, name, Mode::SET);

				if (timer.getValue() - this->toastTime > 1000)
					this->state = IDLE;
			}
			break;
		case MAIN:
			{
				menu(delta, 5);
				entry("Actors");
				entry("Scenarios");
				entry("Buttons");
				entry("Timers");
				entry("Exit");
			}
			break;
		case ACTORS:
			{
				int actorCount = actors.getCount();
				menu(delta, actorCount + 1);
				for (int i = 0; i < actorCount; ++i) {
					const Actor & actor = actors.get(i);
					this->string = actor.name;
					entry(this->string);
				}
				entry("Exit");
			}
			break;
		case SCENARIOS:
			{
				int scenarioCount = scenarios.getCount();

				menu(delta, scenarioCount + 2);
				for (int i = 0; i < scenarioCount; ++i) {
					const Scenario &scenario = scenarios.get(i);
					this->string = scenario.name;
					entry(this->string);
				}
				entry("Add Scenario");
				entry("Exit");
			}
			break;
		case EDIT_SCENARIO:
		case ADD_SCENARIO:
			{
				const int actorCount = actors.getCount();
				
				menu(delta, 1 + actorCount + 1);
				this->string = "Scenario: ", this->scenario.name;
				entry(this->string);

				// actors
				for (int i = 0; i < actorCount; ++i) {
					const Actor &actor = actors.get(i);
					this->string = actor.name, ": ";
					int value = this->scenario.actorValues[i];
					if (value <= 100)
						this->string += value;
					else
						this->string += " -";
					entry(this->string);
				}
				entry("Exit");
			}
			break;
		case BUTTONS:
			{
				int buttonCount = buttons.getCount();
				
				menu(delta, buttonCount + 2);
				for (int i = 0; i < buttonCount; ++i) {
					const Button &button = buttons.get(i);
					
					this->string = button.id, ':', button.state;
					entry(this->string);
				}
				entry("Add Button");
				entry("Exit");
			}
			break;
		case EDIT_BUTTON:
		case ADD_BUTTON:
			{
				int scenarioCount = this->button.getScenarioCount();

				menu(delta, 1 + scenarioCount + 3);
				this->string = "Button: ", this->button.id, ':', this->button.state;
				entry(this->string);
				
				// scenarios
				for (int i = 0; i < scenarioCount; ++i) {
					this->string = /*"Scenario ", (i + 1), ": ",*/ scenarios.get(button.scenarios[i]).name;//sconfiguration.getScenario(button.scenarios[i]).name;
					entry(this->string);
				}
				
				entry("Add Scenario");
	 			entry("Delete Button");
				entry("Exit");
			}
			break;
		case SELECT_SCENARIO:
			{
				int scenarioCount = scenarios.getCount();//configuration.getScenarioCount();
				menu(delta, scenarioCount + 1);
				for (int i = 0; i < scenarioCount; ++i) {
					entry(scenarios.get(i).name);//configuration.getScenario(i).name);
				}
				entry("Remove Scenario");
			}
			break;
		case TIMERS:
			{
			
			}
			break;
		}
	}
	
	void onButton(uint32_t id, uint8_t state) {
		switch (this->state) {
		case BUTTONS:
			{
				int buttonCount = buttons.getCount();
				for (int i = 0; i < buttonCount; ++i) {
					const Button &button = buttons.get(i);
					if (button.id == id && button.state == state) {
						this->selected = i;
						this->button = buttons.get(i);
						push(EDIT_BUTTON);
						break;
					}
				}
			}
			break;
		case ADD_BUTTON:
			{
				this->button.id = id;
				this->button.state = state;
			}
			break;
		default:
			;
		}
	}
	
	String<32> &toast() {
		this->toastTime = timer.getValue();
		this->state = TOAST;
		return this->string;
	}
	
protected:
	
	void menu(int delta, int entryCount) {
		// adjust yOffset so that selected entry is visible
		const int lineHeight = tahoma_8pt.height + 4;
		int upper = this->selected * lineHeight;
		int lower = upper + lineHeight;
		if (upper < this->yOffset)
			this->yOffset = upper;
		if (lower > this->yOffset + bitmap.HEIGHT)
			this->yOffset = lower - bitmap.HEIGHT;
	
		// update selected according to delta motion of poti
		this->selected = limit(this->selected + delta, entryCount);
		this->entryIndex = 0;
	}
	
	void entry(const char* s) {
		const int lineHeight = tahoma_8pt.height + 4;
		
		int y = this->entryIndex * lineHeight + 2 - this->yOffset;
		if (this->entryIndex == this->selected)
			bitmap.drawText(0, y, tahoma_8pt, ">");
		bitmap.drawText(10, y, tahoma_8pt, s);

		++this->entryIndex;
	}
	
	void push(State state) {
		this->stack[this->stackIndex++] = {this->state, this->selected, this->yOffset};
		this->state = state;
		this->selected = 0;
		this->yOffset = 0;
	}
	
	void pop() {
		this->state = this->stack[--this->stackIndex].state;
		this->selected = this->stack[this->stackIndex].selected;
		this->yOffset = this->stack[this->stackIndex].yOffset;
	}
	
	int getThisIndex() {
		return this->stack[this->stackIndex - 1].selected;
	}
	
	// menu state
	State state = IDLE;

	// index of selected element
	int selected = 0;
	
	bool edit = false;
	
	// menu display state
	int yOffset = 0;
	int entryIndex;

	// menu stack
	struct StackEntry {State state; int selected; int yOffset;};
	int stackIndex = 0;
	StackEntry stack[6];
	
	// toast data
	int toastTime;
	
	// temporary string
	String<32> string;
	
	// temporary objects
	Button button;
	Scenario scenario;
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
			
			if (menu.getState() == Menu::IDLE || menu.getState() == Menu::TOAST) {
				// find scenario
				const Button *button = findButton(nodeId, state);
				if (button != nullptr) {
					// find current scenario by comparing similarity to actor target values
					int bestSimilarity = 0x7fffffff;
					int bestI = -1;
					for (int i = 0; i < Button::SCENARIO_COUNT && button->scenarios[i] != 0xff; ++i) {
						uint8_t scenario = button->scenarios[i];
						const uint8_t *values = findScenario(scenario)->actorValues;
						int similarity = actorValues.getSimilarity(values);
						if (similarity < bestSimilarity) {
							bestSimilarity = similarity;
							bestI = i;
						}
					}
					if (bestI != -1) {
						int currentScenario = button->scenarios[bestI];
					
						// check if actor (e.g. blind) is still moving
						if (actorValues.stop(findScenario(currentScenario)->actorValues)) {
							// stopped
							//std::cout << "stopped" << std::endl;
							menu.toast() = "Stopped";
						} else {
							// next scenario
							//std::cout << "next" << std::endl;
							int nextI = bestI + 1;
							if (nextI >= Button::SCENARIO_COUNT || button->scenarios[nextI] == 0xff)
								nextI = 0;
							const Scenario *nextScenario = findScenario(button->scenarios[nextI]);

							// set actor values for next scenario
							actorValues.set(nextScenario->actorValues);
						
							// display scenario name
							menu.toast() = nextScenario->name;
						}
					}
				}
			} else {
				if (state != 0)
					menu.onButton(nodeId, state);
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


int main(int argc, const char **argv) {
	if (argc <= 1) {
		std::cout << "usage: emulator <device>" << std::endl;
	#if __APPLE__
		std::cout << "example: emulator /dev/tty.usbserial-FT3PMLOR" << std::endl;
	#elif __linux__
	#endif
		return 1;
	}
	
	// get device from command line
	std::string device = argv[1];
	
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
	}

	// init flash
	int page = 0;
	page += actors.init(page);
	page += scenarios.init(page);
	page += buttons.init(page);
	for (auto && actorInfo : actorsData) {
		actors.write(actors.getCount(), actorInfo);
	}
	for (auto && scenario : scenariosData) {
		scenarios.write(scenarios.getCount(), scenario);
	}
	for (auto && button : buttonsData) {
		buttons.write(buttons.getCount(), button);
	}

	// set speed to actor values
	for (int i = 0; i < actors.getCount(); ++i) {
		actorValues.setSpeed(i, actors.get(i).speed);
	}

	// connection to EnOcean controller
	EnOceanProtocol protocol(device);
	
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
		actorValues.update();
		display.update(bitmap);
		menu.update(poti1.getDelta(), poti1.getPress());

		// get relay outputs
		uint8_t outputs = 0;
		for (int i = 0; i < actors.getCount(); ++i) {
			const Actor &actor = actors.get(i);
			if (actor.type == Actor::Type::SWITCH) {
				outputs |= actorValues.getSwitchState(i) << actor.outputIndex;
			} else {
				outputs |= actorValues.getBlindState(i) << actor.outputIndex;
			}
		}
		//std::cout << int(outputs) << std::endl;
		
		// transfer actor values to emulator
		for (int i = 0; i < 4; ++i) {
			lights[i].setValue(actorValues.get(i));
			blinds[i].setValue(actorValues.get(i + 4));
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
	
	// cleanup
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
