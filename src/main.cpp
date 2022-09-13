#include <iostream>
#include <thread>

#include "config.hpp"
#include "memory.hpp"
#include "chip.hpp"
#include "display.hpp"

int main(int argc, char** argv) {
	if (argc < 2) {
		std::cerr << PROJECT_NAME << " v" << PROJECT_VER << std::endl;
		std::cerr << " -----" << std::endl;
		std::cerr << "This is a SCHIP/CHIP8 emulator. " << std::endl << std::endl;
		std::cerr << "Usage: " << argv[0] << " <path to rom>" << std::endl;
		return EXIT_FAILURE;
	}

	try {
		glutInit(&argc, argv);
		MMU mmu;
		Chip chip(mmu);

		mmu.load_program(argv[1]);

		std::thread ct([&chip]() {
			try {
				chip.run();
			} catch (std::exception& err) {
				std::cerr << "Error: " << err.what() << std::endl;
			}
		});

		Display::init();
		Display::run();

	} catch (std::exception& err) {
		std::cerr << "Error: " << err.what() << std::endl;
		exit(EXIT_FAILURE);
	}

	return EXIT_SUCCESS;
}