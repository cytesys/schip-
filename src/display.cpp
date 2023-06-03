#include <schip/display.h>

std::array<bool, (SCR_HEIGHT* SCR_WIDTH)> Display::buffer = {};
std::array<bool, NUM_KEYS> Display::keys = {};

void Display::init() {
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
	glutInitWindowSize(
		SCR_WIDTH * SCR_ZOOM,
		SCR_HEIGHT * SCR_ZOOM
	);
	glutCreateWindow("Chip8 Emulator");
	glColor3f(1.0f, 1.0f, 1.0f);

	glutDisplayFunc(Display::repaint);
	glutReshapeFunc(Display::reshape);
	glutIdleFunc(Display::repaint);

	glutKeyboardFunc(Display::keydown);
	glutKeyboardUpFunc(Display::keyup);
}

void Display::reshape(int w, int h) {
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(
		0.0f, SCR_WIDTH * SCR_ZOOM,
		SCR_HEIGHT * SCR_ZOOM, 0.0f,
		0.0f, 1.0f
	);
}

void Display::repaint() {
	int i, x, y;

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	for (i = 0; i < (SCR_WIDTH * SCR_HEIGHT); i++) {
		if (Display::buffer.at(i)) {
			x = (i % SCR_WIDTH) * SCR_ZOOM;
			y = (i / SCR_WIDTH) * SCR_ZOOM;

			glRecti(
				x, y,
				x + SCR_ZOOM,
				y + SCR_ZOOM
			);
		}
	}

	glutSwapBuffers();
}

int lookup_key(unsigned char key) {
	switch (key) {
		case 'x':
			return 0x0;
		case '1':
			return 0x1;
		case '2':
			return 0x2;
		case '3':
			return 0x3;
		case 'q':
			return 0x4;
		case 'w':
			return 0x5;
		case 'e':
			return 0x6;
		case 'a':
			return 0x7;
		case 's':
			return 0x8;
		case 'd':
			return 0x9;
		case 'z':
			return 0xa;
		case 'c':
			return 0xb;
		case '4':
			return 0xc;
		case 'r':
			return 0xd;
		case 'f':
			return 0xe;
		case 'v':
			return 0xf;
		default:
			return -1;
	}
}

void Display::keydown(unsigned char key, int x, int y) {
	int index = lookup_key(key);
	
	if (index >= 0) {
		Display::keys.at(index) = true;
	}
}

void Display::keyup(unsigned char key, int x, int y) {
	int index = lookup_key(key);

	if (index >= 0) {
		Display::keys.at(index) = false;
	}
}

void Display::run() {
	glutMainLoop();
}