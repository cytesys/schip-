#include <iostream>
#include <fstream>
#include <cstdint>
#include <vector>
#include <stdexcept>

#include <sys/types.h>
#include <sys/stat.h>

#include "config.hpp"
#include "common.hpp"

constexpr size_t BUF_SIZE = 0x1000;

 /** Tries to find an element in a vector.
 * 
 * If the element is found the index of that element is
 * returned. Returns -1 if not found.
 * 
 * @param vec The vector to search in.
 * @param elem The element to look for.
 * 
 * @return The index of the element, or -1 if not found.
 */
template<typename T>
int vecfind(std::vector<T>& vec, T elem) {
	int i;

	for (i = 0; i < vec.size(); i++) {
		if (vec.at(i) == elem) {
			return i;
		}
	}
	
	return -1;
}

int main(int argc, char** argv) {
	if (argc < 2) {
		std::cerr << PROJECT_NAME << " v" << PROJECT_VER << std::endl;
		std::cerr << " -----" << std::endl;
		std::cerr << "This is a simple SCHIP/CHIP8 disassembler. ";
		std::cerr << "The output conforms to the syntax used by CHIPPER 2.11 by Hans Christian egeberg." << std::endl << std::endl;
		std::cout << "Usage: " << argv[0] << " <path to rom>" << std::endl;
		return EXIT_FAILURE;
	}

	struct stat info;
	int status = stat(argv[1], &info);

	if (status == -1) {
		throw std::runtime_error("File not found!");
	}

	char buffer[BUF_SIZE] = {};
	size_t size;

	// Load file
	std::cout << "Loading... ";
	std::ifstream file(argv[1], std::ios_base::in | std::ios::binary);

	if(file.peek() != EOF) {
		file.read(buffer, BUF_SIZE);
		size = file.gcount();
	} else {
		throw std::runtime_error("Can not read from file!");
	}

	file.close();

	std::cout << "Done!" << std::endl << std::endl;

	// Disassemble
	uint16_t opcode, addr, here;
	uint8_t nibble[4] = {};
	uint8_t imm;
	int i, j;
	std::vector<uint16_t> funcs = {};
	std::vector<uint16_t> labels = {};
	std::vector<uint16_t> dataaddrs = {};
	bool nl = false;

	std::cout << " --- Start of disassembly ---" << std::endl;
	std::cout << "main:" << std::endl;

	for (i = 0; i < size; i += 2) {
		here = 0x200 + i;

		// Load the opcode
		opcode = static_cast<uint16_t>(buffer[i]) << 8;
		opcode |= static_cast<uint16_t>(buffer[i + 1]) & 0xff;

		addr = opcode & 0xfff; // = nnn
		imm = opcode & 0xff; // = kk

		// Split the opcode into four nibbles for conveniency
		for (j = 0; j < 4; j++) {
			nibble[j] = (opcode >> (12 - (j * 4))) & 0xf;
		}

		// If this is marked as a function/subroutine a function name is
		// printed.
		j = vecfind(funcs, here);
		if (j >= 0) {
			if (!nl) {
				std::cout << std::endl;
				nl = true;
			}
			std::cout << "func" << j << ":" << std::endl;
		}

		// If this is marked as a label, then a label name is printed.
		j = vecfind(labels, here);
		if (j >= 0) {
			if (!nl) {
				std::cout << std::endl;
				nl = true;
			}
			std::cout << "label" << j << ":" << std::endl;
		}

		nl = false;

		// Print the assumed address
		std::cout << "[" << n2hexstr(here, 3) << "]: ";

		// If this is marked as data, print it as a dw
		if (vecfind(dataaddrs, here) >= 0) {
			std::cout << "dw " << n2hexstr(opcode, 4) << std::endl;
			continue;
		}

		/* *** NOTE ***
		* The comments in this big if-statement describe
		* what each opcode does.
		* *************
		*/
		if ((opcode & 0xfff0) == 0x00c0) {
			// Scroll the screen n pixels down.
			std::cout << "scd #" << n2hexstr(nibble[3], 1) << std::endl;

		} else if (opcode == 0x00e0) {
			// Clear the screen.
			std::cout << "clr" << std::endl;

		} else if (opcode == 0x00ee) {
			// Return from subroutine
			std::cout << "ret" << std::endl << std::endl;
			nl = true;

		} else if (opcode == 0x00fb) {
			// Scroll screen 4 pixels right (2 pixels in normal mode).
			std::cout << "scr" << std::endl;

		} else if (opcode == 0x00fc) {
			// Scroll screen 4 pixels left (2 pixels in normal mode).
			std::cout << "scl" << std::endl;

		} else if (opcode == 0x00fd) {
			// Exit the interpreter.
			std::cout << "exit" << std::endl;

		} else if (opcode == 0x00fe) {
			// Disable extended screen mode.
			std::cout << "low" << std::endl;

		} else if (opcode == 0x00ff) {
			// Enable extended screen mode.
			std::cout << "high" << std::endl;

		} else if ((opcode & 0xf000) == 0x0000) {
			// Call machine code routine code at [nnn].
			// Not implemented in emulators.
			//std::cout << "sys #" << n2hexstr(addr, 3) << std::endl;

			// Interpret this as data instead. This is more likely.
			std::cout << "dw " << n2hexstr(opcode, 4) << std::endl;

		} else if ((opcode & 0xf000) == 0x1000) {
			// Jump to [nnn].
			if (addr < 0x200 || addr >= 0x200 + size) {
				// Jumping to an out of range address? This is probably just data.
				std::cout << "dw " << n2hexstr(opcode, 4) << std::endl;

			} else {
				// Mark function as a new label if it doesn't exist already.
				j = vecfind(labels, addr);
				if (j < 0) {
					j = labels.size();
					labels.push_back(addr);
				}
				std::cout << "jp label" << j << " (0x" << n2hexstr(addr, 3) << ")" << std::endl;
			}

		} else if ((opcode & 0xf000) == 0x2000) {
			// Call subroutine at [nnn].
			if (addr < 0x200 || addr >= 0x200 + size) {
				// Calling an out of range address? This is probably just data.
				std::cout << "dw " << n2hexstr(opcode, 4) << std::endl;

			} else {
				// Mark address as a new function if it doesn't exist already.
				j = vecfind(funcs, addr);
				if (j < 0) {
					j = funcs.size();
					funcs.push_back(addr);
				}
				std::cout << "call func" << j << " (0x" << n2hexstr(addr, 3) << ")" << std::endl;
			}

		} else if ((opcode & 0xf000) == 0x3000) {
			// Skip next instruction if Vx == kk.
			std::cout << "se v" << n2hexstr(nibble[1], 1) << ", #" << n2hexstr(imm, 2) << std::endl;

		} else if ((opcode & 0xf000) == 0x4000) {
			// Skip next instruction if Vx != kk.
			std::cout << "sne v" << n2hexstr(nibble[1], 1) << ", #" << n2hexstr(imm, 2) << std::endl;

		} else if ((opcode & 0xf00f) == 0x5000) {
			// Skip next instruction if Vx == Vy.
			std::cout << "se v" << n2hexstr(nibble[1], 1) << ", v" << n2hexstr(nibble[3], 1) << std::endl;

		} else if ((opcode & 0xf000) == 0x6000) {
			// Set Vx = kk.
			std::cout << "ld v" << n2hexstr(nibble[1], 1) << ", #" << n2hexstr(imm, 2) << std::endl;

		} else if ((opcode & 0xf000) == 0x7000) {
			// Set Vx += kk.
			std::cout << "add v" << n2hexstr(nibble[1], 1) << ", #" << n2hexstr(imm, 2) << std::endl;

		} else if ((opcode & 0xf00f) == 0x8000) {
			// Set Vx = Vy.
			std::cout << "ld v" << n2hexstr(nibble[1], 1) << ", v" << n2hexstr(nibble[2], 1) << std::endl;

		} else if ((opcode & 0xf00f) == 0x8001) {
			// Set Vx |= Vy.
			std::cout << "or v" << n2hexstr(nibble[1], 1) << ", v" << n2hexstr(nibble[2], 1) << std::endl;

		} else if ((opcode & 0xf00f) == 0x8002) {
			// Set Vx &= Vy.
			std::cout << "and v" << n2hexstr(nibble[1], 1) << ", v" << n2hexstr(nibble[2], 1) << std::endl;

		} else if ((opcode & 0xf00f) == 0x8003) {
			// Set Vx ^= Vy.
			std::cout << "xor v" << n2hexstr(nibble[1], 1) << ", v" << n2hexstr(nibble[2], 1) << std::endl;

		} else if ((opcode & 0xf00f) == 0x8004) {
			// Set Vx += Vy.
			std::cout << "add v" << n2hexstr(nibble[1], 1) << ", v" << n2hexstr(nibble[2], 1) << std::endl;

		} else if ((opcode & 0xf00f) == 0x8005) {
			// Set Vx -= Vy.
			std::cout << "sub v" << n2hexstr(nibble[1], 1) << ", v" << n2hexstr(nibble[2], 1) << std::endl;

		} else if ((opcode & 0xf00f) == 0x8006) {
			// Set Vx >>= 1.
			std::cout << "shr v" << n2hexstr(nibble[1], 1) << std::endl;

		} else if ((opcode & 0xf00f) == 0x8007) {
			// Set Vx = Vy - Vx.
			std::cout << "subn v" << n2hexstr(nibble[1], 1) << ", v" << n2hexstr(nibble[2], 1) << std::endl;

		} else if ((opcode & 0xf00f) == 0x800e) {
			// Set Vx <<= 1.
			std::cout << "shl v" << n2hexstr(nibble[1], 1) << std::endl;

		} else if ((opcode & 0xf00f) == 0x9000) {
			// Skip the next instruction if Vx != Vy.
			std::cout << "sne v" << n2hexstr(nibble[1], 1) << ", v" << n2hexstr(nibble[2], 1) << std::endl;

		} else if ((opcode & 0xf000) == 0xa000) {
			// Set I = nnn.
			std::cout << "ld I, #" << n2hexstr(addr, 3) << std::endl;

			// Mark address as a new data address if it doesn't exist already.
			j = vecfind(dataaddrs, addr);
			if (j < 0) {
				dataaddrs.push_back(addr);
			}

		} else if ((opcode & 0xf000) == 0xb000) {
			// Jump to [nnn + V0].
			std::cout << "jp V0, #" << n2hexstr(addr, 3) << std::endl;

		} else if ((opcode & 0xf000) == 0xc000) {
			// Set Vx = Random() & kk
			std::cout << "rnd v" << n2hexstr(nibble[1], 1) << ", #" << n2hexstr(imm, 2) << std::endl;

		} else if ((opcode & 0xf000) == 0xd000) {
			// Draw a 8-bit wide and n-bit high sprite to the screen
			// at coordinates Vx, Vy. 16*16-bit sprites are available
			// if extended mode is enabled and n == 0.
			std::cout << "drw v" << n2hexstr(nibble[1], 1) << ", v" << n2hexstr(nibble[2], 1) << ", #" << n2hexstr(nibble[3], 1) << std::endl;

		} else if ((opcode & 0xf0ff) == 0xe09e) {
			// Skip next instruction if the key in Vx is pressed.
			std::cout << "skp v" << n2hexstr(nibble[1], 1) << std::endl;

		} else if ((opcode & 0xf0ff) == 0xe0a1) {
			// Skip next instruction if the key in Vx is not pressed.
			std::cout << "sknp v" << n2hexstr(nibble[1], 1) << std::endl;

		} else if ((opcode & 0xf0ff) == 0xf007) {
			// Load value of delay timer into Vx.
			std::cout << "ld v" << n2hexstr(nibble[1], 1) << ", DT" << std::endl;

		} else if ((opcode & 0xf0ff) == 0xf00a) {
			// Wait for keypress and store into Vx.
			std::cout << "ld v" << n2hexstr(nibble[1], 1) << ", K" << std::endl;

		} else if ((opcode & 0xf0ff) == 0xf015) {
			// Set delay timer to Vx.
			std::cout << "ld DT, v" << n2hexstr(nibble[1], 1) << std::endl;

		} else if ((opcode & 0xf0ff) == 0xf018) {
			// Set sound timer to Vx.
			std::cout << "ld ST, v" << n2hexstr(nibble[1], 1) << std::endl;

		} else if ((opcode & 0xf0ff) == 0xf01e) {
			// Add Vx to I
			std::cout << "add I, v" << n2hexstr(nibble[1], 1) << std::endl;

		} else if ((opcode & 0xf0ff) == 0xf029) {
			// Load address of 5-byte hex font for value in Vx into I.
			std::cout << "ld LF, v" << n2hexstr(nibble[1], 1) << std::endl;

		} else if ((opcode & 0xf0ff) == 0xf030) {
			// Load address of 10-byte font for value in Vx into I.
			std::cout << "ld HF, v" << n2hexstr(nibble[1], 1) << std::endl;

		} else if ((opcode & 0xf0ff) == 0xf033) {
			// Store the BCD of Vx into [I].
			std::cout << "ld B, v" << n2hexstr(nibble[1], 1) << std::endl;

		} else if ((opcode & 0xf0ff) == 0xf055) {
			// Dump registers V0..Vx to [I].
			std::cout << "ld [I], v" << n2hexstr(nibble[1], 1) << std::endl;

		} else if ((opcode & 0xf0ff) == 0xf065) {
			// Fill registers V0..Vx from [I].
			std::cout << "ld v" << n2hexstr(nibble[1], 1) << ", [I]" << std::endl;

		} else if ((opcode & 0xf0ff) == 0xf075) {
			// Dump registers V0..Vx to RPL user flags (x <= 7).
			std::cout << "ld R, v" << n2hexstr(nibble[1], 1) << std::endl;

		} else if ((opcode & 0xf0ff) == 0xf085) {
			// Fill registers V0..Vx from RPL user flags (x <= 7).
			std::cout << "ld v" << n2hexstr(nibble[1], 1) << ", R" << std::endl;

		} else {
			// Invalid/unknown opcodes are marked as data.
			std::cout << "dw " << n2hexstr(opcode, 4) << std::endl;
		}
	}

	std::cout << " --- End of disassembly ---" << std::endl << std::endl;

	std::cout << "Number of found functions: " << funcs.size() << std::endl;
	std::cout << "Number of found labels: " << labels.size() << std::endl;

	return EXIT_SUCCESS;
}