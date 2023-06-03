cmake_minimum_required (VERSION 3.8)

project ("SCHIP++" VERSION 1.1.0)

set(OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)

set(SCHIP_SRC_DIR "${CMAKE_SOURCE_DIR}/src")
set(SCHIP_INC_DIR "${CMAKE_SOURCE_DIR}/include")
set(SCHIP_HDR_DIR "${SCHIP_INC_DIR}/schip")

configure_file(
	"${SCHIP_HDR_DIR}/config.h.in"
	"${SCHIP_HDR_DIR}/config.h"
)

set(SCHIP_HEADERS
	"${SCHIP_HDR_DIR}/config.h"
	"${SCHIP_HDR_DIR}/common.h"
	"${SCHIP_HDR_DIR}/memory.h"
	"${SCHIP_HDR_DIR}/chip.h"
	"${SCHIP_HDR_DIR}/display.h"
)

set(SCHIP_SRC
	"${SCHIP_SRC_DIR}/memory.cpp"
	"${SCHIP_SRC_DIR}/chip.cpp"
	"${SCHIP_SRC_DIR}/display.cpp"
	"${SCHIP_SRC_DIR}/main.cpp"
)

add_executable(chip8 ${SCHIP_SRC} ${SCHIP_HEADERS})
target_include_directories(chip8 PRIVATE ${SCHIP_INC_DIR})
target_link_libraries(chip8 PRIVATE OpenGL::GL)
target_link_libraries(chip8 PRIVATE GLUT::GLUT)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(GLOBAL PROPERTY CXX_STANDARD 20)
endif()
