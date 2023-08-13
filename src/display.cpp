#include <algorithm>
#include <thread>
#include <cassert>

#include <schip/display.h>

PPU& PPU::get_instance() {
    static PPU instance;
    return instance;
}

void PPU::lock() {
    while (m_busy.load())
        std::this_thread::yield();
    m_busy.store(true);
}

void PPU::release() {
    m_busy.store(false);
}

void PPU::enable_extended() {
    lock();
    m_is_extended = true;
    release();
}

void PPU::disable_extended() {
    lock();
    m_is_extended = false;
    release();
}

void PPU::clear_screen() {
    lock();
    m_pixels.fill(0);
    release();
}

void PPU::scroll_down(unsigned lines) {
    if (lines < 1 || lines > screen_height)
        return;

    if (lines == screen_height)
        return clear_screen();

    lock();
    std::shift_right(m_pixels.begin(), m_pixels.end(), lines * screen_width);
    std::fill_n(m_pixels.begin(), lines * screen_width, 0);
    release();
}

void PPU::scroll_left() {
    lock();
    for (auto i = m_pixels.begin(); i < m_pixels.end(); std::advance(i, screen_width)) {
        std::shift_left(i, i + screen_width, 4);
        std::fill_n(i + screen_width - 4, 4, 0);
    }
    release();
}

void PPU::scroll_right() {
    lock();
    for (auto i = m_pixels.begin(); i < m_pixels.end(); std::advance(i, screen_width)) {
        std::shift_right(i, i + screen_width, 4);
        std::fill_n(i, 4, 0);
    }
    release();
}

void PPU::render() {
    if (m_busy.load()) return; // If the PPU is busy, just return. Avoids flickering.

    lock();

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    int i, x, y, z = zoom;
    if (!m_is_extended)
        z *= 2; // Double zoom

    for (i = 0; i < m_pixels.size(); i++) {
        if (!m_is_extended) {
            if (i / screen_width >= screen_height / 2) break;
            if (i % screen_width >= screen_width / 2) {
                i += screen_width / 2 - 1;
                continue;
            }
        }

        if (m_pixels.at(i)) {
            x = (i % screen_width) * z;
            y = (i / screen_width) * z;

            glRecti(x, y, x + z, y + z);
        }
    }

    release();

    glutSwapBuffers();
}

bool PPU::draw_sprite_at(Addr loc, unsigned lines, unsigned x, unsigned y) {
    unsigned width{8};
    unsigned adj{0};

    if (lines == 0 && m_is_extended) {
        width = 16;
        lines = 16;
    }

    if (lines < 1 || lines > 16)
        return false; // Out of range

    if (m_is_extended) {
        x %= screen_width;
        y %= screen_height;
    } else {
        x %= screen_width / 2;
        y %= screen_height / 2;
    }

    if (unsigned n{screen_width - x}; n < width)
        adj = width - n; // Remove overflow

    if (unsigned n{screen_height - y}; n < lines)
        lines = n; // Remove overflow

    Bus& bus = Bus::get_instance();
    lock();

    uint8_t pix;
    uint16_t row;
    unsigned i, j, c;
    bool collision{false};

    for (i = 0; i < lines; i++) {
        if (width == 16) {
            row = bus.read(loc + (i * 2));
            row <<= 8;
            row |= bus.read(loc + (i * 2) + 1);
        } else {
            row = bus.read(loc + i);
        }

        c = ((y + i) * screen_width + x);
        assert(c < m_pixels.size());

        for (j = 0; j < width - adj; j++) {
            pix = (row >> (width - 1 - j)) & 1;
            if (pix && m_pixels.at(c + j))
                collision = true;

            m_pixels.at(c + j) ^= pix;
        }
    }

    release();
    return collision;
}

void PPU::make_test_pattern() {
    Bus& bus = Bus::get_instance();
    int i = 0x200;
    for (Byte b : {0b01111111, 0b11111110,
                   0b11000000, 0b00000011,
                   0b10100000, 0b00000101,
                   0b10010000, 0b00001001,
                   0b10001000, 0b00010001,
                   0b10000100, 0b00100001,
                   0b10000010, 0b01000001,
                   0b10000000, 0b10000001,
                   0b10000001, 0b00000001,
                   0b10000010, 0b01000001,
                   0b10000100, 0b00100001,
                   0b10001000, 0b00010001,
                   0b10010000, 0b00001001,
                   0b10100000, 0b00000101,
                   0b11000000, 0b00000011,
                   0b01111111, 0b11111110}) {
        bus.write(i++, b);
    }

    i = 0x300;
    for (Byte b : {0b11111111,
                   0b10000001,
                   0b10000001,
                   0b10000001,
                   0b10000001,
                   0b10000001,
                   0b10000001,
                   0b10000001,
                   0b11111111}) {
        bus.write(i++, b);
    }

//    enable_extended();

//    draw_sprite_at(0x200, 0, 120, 60);

    draw_sprite_at(0x300, 8, 60, 28);

//    lock();
//    m_is_extended = true;
//    for (int i = 0; i < m_pixels.size(); i++)
//        m_pixels.at(i) = (i / screen_width == 63) ? 1 : 0;
//    release();
}

void Display::init() {
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(
        PPU::screen_width * PPU::zoom,
        PPU::screen_height * PPU::zoom
	);
    glutCreateWindow("S-Chip Emulator");
	glColor3f(1.0f, 1.0f, 1.0f);

	glutDisplayFunc(Display::repaint);
	glutReshapeFunc(Display::reshape);
    glutIdleFunc(Display::repaint);

    glutKeyboardFunc(Display::keydown);
    glutKeyboardUpFunc(Display::keyup);

//    PPU::get_instance().make_test_pattern();
}

void Display::reshape(int w, int h) {
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(
        0.0f, PPU::screen_width * PPU::zoom,
        PPU::screen_height * PPU::zoom, 0.0f,
		0.0f, 1.0f
	);
}

void Display::repaint() {
    PPU::get_instance().render();
}

void Display::keydown(unsigned char key, int x, int y) { KeyPad::get_instance().press_key(key); }

void Display::keyup(unsigned char key, int x, int y) { KeyPad::get_instance().release_key(key); }

void Display::run() { glutMainLoop(); }
