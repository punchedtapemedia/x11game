cmake_minimum_required(VERSION 3.10)
project(main)

find_package(X11)

add_executable(main main.cpp)

target_link_libraries(main
	PRIVATE
	${X11_LIBRARIES}
	)

