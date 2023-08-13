cmake_minimum_required (VERSION 3.16)

project ("SCHIP++" VERSION 1.2.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)

set(SCHIP_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(SCHIP_INC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(SCHIP_HDR_DIR "${SCHIP_INC_DIR}/schip")

add_subdirectory(src)

add_executable(chip8 ${SCHIP_SOURCES} ${SCHIP_HEADERS})
target_compile_definitions(chip8 PRIVATE "__apple__=$<COMPILE_LANG_AND_ID:CXX,AppleClang>")
target_compile_definitions(chip8 PRIVATE "DEBUG=$<CONFIG:Debug>")
target_include_directories(chip8 PRIVATE ${SCHIP_INC_DIR})
target_link_libraries(chip8 PRIVATE OpenGL::GL)
target_link_libraries(chip8 PRIVATE GLUT::GLUT)
