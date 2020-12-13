cmake_minimum_required(VERSION 3.9)

# set project
project(Emulator)

#set(CMAKE_VERBOSE_MAKEFILE ON)

# use C++17 standard by default
set(CMAKE_CXX_STANDARD 17)

# suppress warning for own s and ms literal operators that are used instad of those in std::chrono
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-user-defined-literals")
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-literal-suffix")
endif()

# enable address sanitizer
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")


# install conan dependencies if not done yet (CLion: use https://github.com/conan-io/conan-clion-plugin)
if(${PROJECT_SOURCE_DIR}/conanfile.py IS_NEWER_THAN ${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
	set(PROFILE default)
	if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
		set(PROFILE Debug)
	endif()
	message("install conan dependencies for profile ${PROFILE}")
	execute_process(COMMAND conan install ${PROJECT_SOURCE_DIR}/conanfile.py --profile ${PROFILE} WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
endif()

# conan dependencies
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS KEEP_RPATHS)

# set debug define
add_definitions(-DDEBUG)

# strip unised dylibs
#add_link_options(-dead_strip_dylibs)

set(GLAD
	glad/glad.c
	glad/glad.h
	glad/khrplatform.h
)
source_group(glad FILES ${GLAD})

# CLion: Software directory is in External Libraries -> Header Search Paths -> Software
set(SOFTWARE
	../software/Array.hpp
	../software/ArrayList.hpp
	../software/Bitmap.cpp
	../software/Bitmap.hpp
	../software/ClockTime.hpp
	../software/convert.cpp
	../software/convert.hpp
	../software/defines.hpp
	../software/Device.hpp
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
	../software/StringOperators.hpp
	../software/Storage.cpp
	../software/Storage.hpp
	../software/StringSet.hpp
	../software/SystemTime.hpp
	../software/tahoma_8pt.cpp
	../software/tahoma_8pt.hpp
	../software/TopicBuffer.cpp
	../software/TopicBuffer.hpp
	../software/util.hpp
)
source_group(software FILES ${SOFTWARE})

set(EMULATOR
	main.cpp
	assert.hpp
	config.hpp
	global.cpp
	global.hpp
	Bus.cpp
	Bus.hpp
	Clock.cpp
	Clock.hpp
	Display.cpp
	Display.hpp
	Flash.cpp
	Gui.cpp
	Gui.hpp
	Inputs.cpp
	Inputs.hpp
	Network.cpp
	Network.hpp
	Outputs.cpp
	Outputs.hpp
	Poti.cpp
	Poti.hpp
	SystemTimer.cpp
	SystemTimer.hpp
)
source_group(emulator FILES ${EMULATOR})

# emulator executable
add_executable(emulator
	${GLAD}
	${SOFTWARE}
	${EMULATOR}
)
include_directories(emulator
	PRIVATE
		.
		../software
)
conan_target_link_libraries(emulator)

# install the emulator
install(TARGETS emulator
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
	RUNTIME DESTINATION bin
)



# test broker
add_executable(testBroker
	testBroker.cpp
	../software/MqttSnClient.cpp
	../software/MqttSnClient.hpp
	../software/MqttSnBroker.cpp
	../software/MqttSnBroker.hpp
	../software/SystemTime.hpp
	Network.cpp
	Network.hpp
	SystemTimer.cpp
	SystemTimer.hpp
	global.cpp
	global.hpp
)
include_directories(testBroker
	PRIVATE
		.
		../software
)
conan_target_link_libraries(testBroker)



# enable testing, adds test or RUN_TESTS target to run all tests
#enable_testing()
#add_subdirectory(test)
