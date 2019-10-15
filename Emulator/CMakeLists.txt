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

set(GLAD
	glad/glad.c
	glad/glad.h
	glad/khrplatform.h
)
source_group(glad FILES ${GLAD})

set(GUI
	gui/Widget.cpp
	gui/Widget.hpp
	gui/Display.cpp
	gui/Display.hpp
	gui/Poti.cpp
	gui/Poti.hpp
	gui/Light.cpp
	gui/Light.hpp
	gui/Blind.cpp
	gui/Blind.hpp
	gui/Temperature.cpp
	gui/Temperature.hpp
)
source_group(gui FILES ${GUI})

set(SOURCES
	main.cpp
	util.hpp
	Flash.cpp
	Flash.hpp
	Storage.cpp
	Storage.hpp
	Clock.cpp
	Clock.hpp
	Actor.hpp
	Scenario.hpp
	Button.hpp
	Timer.hpp
	String.hpp
	Ticker.cpp
	Ticker.hpp
	Serial.cpp
	Serial.hpp
	EnOceanProtocol.cpp
	EnOceanProtocol.hpp
	ActorValues.cpp
	ActorValues.hpp
)
source_group("" FILES ${SOURCES})

# CLion: Fonts are in External Libraries -> Header Search Paths -> Fonts
set(DISPLAY
	../Display/Bitmap.cpp
	../Display/Bitmap.hpp
	../Display/Font.cpp
	../Display/Font.hpp
	../Display/tahoma_8pt.cpp
	../Display/tahoma_8pt.hpp
)
source_group(Display FILES ${DISPLAY})

add_executable(emulator
	${GLAD}
	${GUI}
	${SOURCES}
	${DISPLAY}
)
include_directories(emulator
	PRIVATE
		.
		../Display
		${Boost_INCLUDE_DIR}
)
target_link_libraries(emulator
	PRIVATE
		glfw
		${CMAKE_THREAD_LIBS_INIT}
		${CMAKE_DL_LIBS}
		${Boost_SYSTEM_LIBRARY}
)
