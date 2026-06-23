cmake_minimum_required(VERSION 4.1.6)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(standby_server)

find_package(CURL REQUIRED)

file(GLOB_RECURSE SRC_FILES ${CMAKE_SOURCE_DIR}/src/*.cpp)
add_executable(${PROJECT_NAME} ${SRC_FILES})

target_include_directories(${PROJECT_NAME} PUBLIC 
	${CMAKE_SOURCE_DIR}/include
	${CURL_INCLUDE_DIRS}
)
target_link_libraries(${PROJECT_NAME} PRIVATE
	${CURL_LIBRARIES}
)

if (EXISTS ${CMAKE_BINARY_DIR}/compile_commands.json)
	configure_file(
		${CMAKE_BINARY_DIR}/compile_commands.json
		${CMAKE_SOURCE_DIR}/compile_commands.json
		COPYONLY
	)
endif()
