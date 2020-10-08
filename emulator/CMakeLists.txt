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

# CLion: Software directory is in External Libraries -> Header Search Paths -> Software
set(SOFTWARE
	../software/util.hpp
	../software/Array.hpp
	../software/Bitmap.cpp
	../software/Bitmap.hpp
	../software/convert.cpp
	../software/convert.hpp
	../software/Font.cpp
	../software/Font.hpp
	../software/Flash.hpp
	../software/MqttSnClient.cpp
	../software/MqttSnClient.hpp
	../software/MqttSnBroker.cpp
	../software/MqttSnBroker.hpp
	../software/optional.hpp
	../software/RoomControl.cpp
	../software/RoomControl.hpp
	../software/String.hpp
	../software/StringBuffer.hpp
	../software/tahoma_8pt.cpp
	../software/tahoma_8pt.hpp
	../software/Storage.cpp
	../software/Storage.hpp
	../software/SystemTime.hpp
	#../software/Clock.hpp
	#../software/EnOceanProtocol.cpp
	#../software/EnOceanProtocol.hpp
	#../software/Action.cpp
	#../software/Action.hpp
	#../software/Event.hpp
	#../software/Timer.hpp
	#../software/Scenario.hpp
	#../software/Device.cpp
	#../software/Device.hpp
	#../software/DeviceState.cpp
	#../software/DeviceState.hpp
	#../software/System.cpp
	#../software/System.hpp
)
source_group(software FILES ${SOFTWARE})

set(EMULATOR
	main.cpp
	assert.hpp
	config.hpp
	global.cpp
	global.hpp
	SystemTimer.cpp
	SystemTimer.hpp
	Clock.cpp
	Clock.hpp
	#Udp6.cpp
	#Udp6.hpp
	UpLink.cpp
	UpLink.hpp
	DownLink.cpp
	DownLink.hpp
	Lin.cpp
	Lin.hpp
	Display.cpp
	Display.hpp
	Inputs.cpp
	Inputs.hpp
	Outputs.cpp
	Outputs.hpp
	Poti.cpp
	Poti.hpp
	Flash.cpp
	Gui.cpp
	Gui.hpp
	#Ticker.hpp
	#Serial.cpp
	#Serial.hpp
)
source_group(emulator FILES ${EMULATOR})

add_executable(emulator
	${GLAD}
	${SOFTWARE}
	${EMULATOR}
)
include_directories(emulator
	PRIVATE
		.
		../software
		#../MQTTSNPacket/src
		${Boost_INCLUDE_DIR}
)
target_link_libraries(emulator
	PRIVATE
		glfw
		${CMAKE_THREAD_LIBS_INIT}
		${CMAKE_DL_LIBS}
		${Boost_SYSTEM_LIBRARY}
		#MQTTSNPacketClient
		#MQTTSNPacketServer
)


# test broker
add_executable(testBroker
	testBroker.cpp
	../software/MqttSnClient.cpp
	../software/MqttSnClient.hpp
	../software/MqttSnBroker.cpp
	../software/MqttSnBroker.hpp
	UpLink.cpp
	UpLink.hpp
	DownLink.cpp
	DownLink.hpp
	SystemTimer.cpp
	SystemTimer.hpp
	global.cpp
	global.hpp
)
include_directories(testBroker
	PRIVATE
		.
		../software
		${Boost_INCLUDE_DIR}
)
target_link_libraries(emulator
	PRIVATE
		${CMAKE_THREAD_LIBS_INIT}
		${CMAKE_DL_LIBS}
		${Boost_SYSTEM_LIBRARY}
)
