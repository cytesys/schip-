#pragma once

#ifndef SCPP_MEMORY
#define SCPP_MEMORY

#include <array>
#include <cstdint>

/**
 * This emulates memory for the SChip/Chip8.
 *
 * As SChip/Chip8 programs are run on virtual machines, memory
 * management unit (MMU) for this class may be a bad word choice.
 * Anyway, this class emulates the virtual memoryspace that Chip8 has.
 * 
 * @see https://en.wikipedia.org/wiki/CHIP-8#Memory
 * @see http://devernay.free.fr/hacks/chip8/C8TECH10.HTM#2.1
 */
class MMU {
public:
	MMU();
	~MMU() {};

	/**
	 * Reads from memory.
	 *
	 * This function attempts to read a byte from memory.
	 *
	 * @param address			The 16-bit address to read from.
	 * @return					The byte at that address
	 * @throw std::out_of_range	if the address is non-readable.
	 */
	uint8_t read(uint16_t address);

	/**
	 * Writes to memory.
	 *
	 * This function attempts to write a byte to memory.
	 *
	 * @param address			The 16-bit address to write to.
	 * @param byte				The 8-bit value to be written.
	 * @throw std::out_of_range	if the address is non-writable.
	 */
	void write(uint16_t address, uint8_t byte);

	/**
	 * Loads a program into memory.
	 *
	 * This function attempts to load a program into memory.
	 *
	 * @param filename			 The path to the file to be loaded.
	 * @throw std::length_error  if the file is bigger than the allocated
	 *							 space in memory.
	 * @throw std::runtime_error if the file cannot be read.
	 */
	void load_program(const char* filename);

private:
	std::array<uint8_t, 0x1000> m_mem = {};
};

#endif