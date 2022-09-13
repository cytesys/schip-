cmake_minimum_required (VERSION 3.8)

project ("SCHIP++" VERSION 1.0.0)

configure_file(
	"${CMAKE_SOURCE_DIR}/src/config.hpp.in"
	"${CMAKE_SOURCE_DIR}/src/config.hpp"
)

set(OpenGL_GL_PREFERENCE GLVND)

find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)

add_executable (chip8
	"${CMAKE_SOURCE_DIR}/src/main.cpp"
	"${CMAKE_SOURCE_DIR}/src/common.hpp"
	"${CMAKE_SOURCE_DIR}/src/config.hpp"

	"${CMAKE_SOURCE_DIR}/src/chip.hpp"
	"${CMAKE_SOURCE_DIR}/src/chip.cpp"

	"${CMAKE_SOURCE_DIR}/src/memory.hpp"
	"${CMAKE_SOURCE_DIR}/src/memory.cpp"

	"${CMAKE_SOURCE_DIR}/src/display.hpp"
	"${CMAKE_SOURCE_DIR}/src/display.cpp"
 )

target_link_libraries(chip8 PRIVATE OpenGL::GL)
target_link_libraries(chip8 PRIVATE GLUT::GLUT)

add_executable(c8disasm
	"${CMAKE_SOURCE_DIR}/src/disasm.cpp"
	"${CMAKE_SOURCE_DIR}/src/common.hpp"
	"${CMAKE_SOURCE_DIR}/src/config.hpp"
)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET chip8 PROPERTY CXX_STANDARD 20)
  set_property(TARGET c8disasm PROPERTY CXX_STANDARD 20)
endif()
