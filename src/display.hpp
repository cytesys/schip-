#pragma once

#ifndef SCPP_DISPLAY
#define SCPP_DISPLAY

#define FREEGLUT_STATIC
#define _LIB
#define FREEGLUT_LIB_PRAGMAS 0

#include <array>
#include <mutex>
#include <GL/glut.h>

constexpr int SCR_WIDTH = 128;
constexpr int SCR_HEIGHT = 64;
constexpr int SCR_BUF_SIZE = SCR_WIDTH * SCR_HEIGHT;
constexpr int SCR_ZOOM = 5;
constexpr int KEY_SIZE = 16;

namespace Display {
	extern std::mutex lock;

	extern std::array<bool, (SCR_HEIGHT* SCR_WIDTH)> buffer;
	extern std::array<bool, KEY_SIZE> keys;

	void init();
	void run();

	void reshape(int w, int h);
	void repaint();
	void keydown(unsigned char key, int x, int y);
	void keyup(unsigned char key, int x, int y);
}

#endif