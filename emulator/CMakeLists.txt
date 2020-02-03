cmake_minimum_required(VERSION 3.9)

# set project
project(Emulator)

# add custom module finders
#set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

#set(CMAKE_VERBOSE_MAKEFILE ON)

# use C++17 standard by default
set(CMAKE_CXX_STANDARD 17)

find_package(Threads REQUIRED)

# macOS: brew install glfw3
find_package(glfw3 3.3 REQUIRED)

# macOS: brew install boost
find_package(Boost REQUIRED COMPONENTS system)

# find Display, Poti and Temperature in gui subdirectory
include_directories(gui)

set(GLAD
	glad/glad.c
	glad/glad.h
	glad/khrplatform.h
)
source_group(glad FILES ${GLAD})

set(GUI
	gui/LayoutManager.hpp
	gui/Widget.cpp
	gui/Widget.hpp
	gui/Display.cpp
	gui/Display.hpp
	gui/Button.cpp
	gui/Button.hpp
	gui/MotionDetector.hpp
	gui/Poti.cpp
	gui/Poti.hpp
	gui/DeviceWidget.cpp
	gui/DeviceWidget.hpp
	gui/Light.cpp
	gui/Light.hpp
	gui/Blind.cpp
	gui/Blind.hpp
	gui/Temperature.cpp
	gui/Temperature.hpp
)
source_group(gui FILES ${GUI})

# CLion: Software directory is in External Libraries -> Header Search Paths -> Software
set(SOFTWARE
	../software/util.hpp
	../software/String.cpp
	../software/String.hpp
	../software/Array.hpp
	../software/Bitmap.cpp
	../software/Bitmap.hpp
	../software/Font.cpp
	../software/Font.hpp
	../software/tahoma_8pt.cpp
	../software/tahoma_8pt.hpp
	../software/Flash.hpp
	../software/Storage.cpp
	../software/Storage.hpp
	../software/Clock.hpp
	../software/EnOceanProtocol.cpp
	../software/EnOceanProtocol.hpp
	../software/Action.cpp
	../software/Action.hpp
	../software/Event.hpp
	../software/Timer.hpp
	../software/Scenario.hpp
	../software/Device.cpp
	../software/Device.hpp
	../software/DeviceState.cpp
	../software/DeviceState.hpp
	../software/System.cpp
	../software/System.hpp
)
source_group(software FILES ${SOFTWARE})

set(SOURCES
	main.cpp
	config.hpp
	assert.hpp
	Flash.cpp
	Clock.cpp
	Ticker.hpp
	Serial.cpp
	Serial.hpp
)
source_group("" FILES ${SOURCES})

add_executable(emulator
	${GLAD}
	${GUI}
	${SOFTWARE}
	${SOURCES}
)
include_directories(emulator
	PRIVATE
		.
		../software
		${Boost_INCLUDE_DIR}
)
target_link_libraries(emulator
	PRIVATE
		glfw
		${CMAKE_THREAD_LIBS_INIT}
		${CMAKE_DL_LIBS}
		${Boost_SYSTEM_LIBRARY}
)
