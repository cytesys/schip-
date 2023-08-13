#include <iostream>
#include <thread>
#include <filesystem>

#include <schip/config.h>
#include <schip/memory.h>
#include <schip/chip.h>
#include <schip/display.h>

int main(int argc, char** argv) {
	if (argc < 2) {
        // Print help text.
		std::cerr << PROJECT_NAME << " v" << PROJECT_VER << std::endl;
		std::cerr << " -----" << std::endl;
		std::cerr << "This is a SCHIP/CHIP8 emulator. " << std::endl << std::endl;
		std::cerr << "Usage: " << argv[0] << " <path to rom>" << std::endl;
        return EXIT_FAILURE;
	}

	try {
		glutInit(&argc, argv);

        Bus::get_instance().load_program(
            std::filesystem::absolute(argv[1])
        );

        std::thread chipthread([]() { Chip::get_instance().run(); });

        Display::init();
		Display::run();

	} catch (std::exception& err) {
		std::cerr << "Error: " << err.what() << std::endl;
        return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
