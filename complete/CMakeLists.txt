cmake_minimum_required(VERSION 3.10)
project(x11game-project)

find_package(X11)

add_executable(x11game main.cpp)

target_link_libraries(x11game
	${X11_LIBRARIES}
	)
