#pragma once

#ifndef DISPLAY_H
#define DISPLAY_H

#define FREEGLUT_STATIC
#define _LIB
#define FREEGLUT_LIB_PRAGMAS 0

#if(__apple__)
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include <cstdint>
#include <array>

#include <schip/keypad.h>
#include <schip/memory.h>

class PPU final {
public:
    static PPU& get_instance();

    void enable_extended();

    void disable_extended();

    void clear_screen();

    void scroll_down(unsigned lines);

    void scroll_left();

    void scroll_right();

    void render();

    bool draw_sprite_at(Addr loc, unsigned lines, unsigned x, unsigned y);

    void make_test_pattern(); // Debugging

    static constexpr int screen_width = 128;
    static constexpr int screen_height = 64;
    static constexpr int zoom = 5;

private:
    PPU() {}

    void lock();

    void release();

    std::array<char, screen_width * screen_height> m_pixels{};
    std::atomic<bool> m_busy{false};
    bool m_is_extended{false};
};

namespace Display {

void init();
void run();

void reshape(int w, int h);
void repaint();
void keydown(unsigned char key, int x, int y);
void keyup(unsigned char key, int x, int y);

}

#endif
