cmake_minimum_required(VERSION 3.9)

# set project
project(Emulator)

# add custom module finders
#set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

#set(CMAKE_VERBOSE_MAKEFILE ON)

# use C++14 standard by default
set(CMAKE_CXX_STANDARD 14)

find_package(Threads REQUIRED)
find_package(glfw3 3.3 REQUIRED)

set(SOURCES
	main.cpp
	glad/glad.c
	glad/glad.h
	glad/khrplatform.h
	Widget.cpp
	Widget.hpp
	Display.cpp
	Display.hpp
	Poti.cpp
	Poti.hpp
)
source_group(Sources FILES ${SOURCES})

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
	${SOURCES}
	${DISPLAY}
)
include_directories(emulator
	PRIVATE
		.
		../Display
)
target_link_libraries(emulator
	PRIVATE
		glfw
		${CMAKE_THREAD_LIBS_INIT}
		${CMAKE_DL_LIBS}
)
