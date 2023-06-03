#pragma once

#ifndef DISPLAY_H
#define DISPLAY_H

#define FREEGLUT_STATIC
#define _LIB
#define FREEGLUT_LIB_PRAGMAS 0

#ifdef __apple__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include <schip/common.h>

constexpr int SCR_WIDTH = 128;
constexpr int SCR_HEIGHT = 64;
constexpr int SCR_BUF_SIZE = SCR_WIDTH * SCR_HEIGHT;
constexpr int SCR_ZOOM = 5;
constexpr int NUM_KEYS = 16;

namespace Display {
	extern std::array<bool, (SCR_HEIGHT* SCR_WIDTH)> buffer;
	extern std::array<bool, NUM_KEYS> keys;

	void init();
	void run();

	void reshape(int w, int h);
	void repaint();
	void keydown(unsigned char key, int x, int y);
	void keyup(unsigned char key, int x, int y);
}

#endif