#pragma once

#ifndef MEMORY_H
#define MEMORY_H

#include <fstream>
#include <sys/types.h>
#include <sys/stat.h>

#include <schip/common.h>

constexpr size_t MEM_ADDR_BEG = 0x200;
constexpr size_t MEM_ADDR_END = 0x1000;

/**
 * This class emulates memory for SChip/Chip8.
 * 
 * @see https://en.wikipedia.org/wiki/CHIP-8#Memory
 * @see http://devernay.free.fr/hacks/chip8/C8TECH10.HTM#2.1
 */
class Bus {
public:
	Bus();
	~Bus();

	/**
	 * Reads one byte from memory.
	 *
	 * This function attempts to read one byte from memory.
	 *
	 * @param address				The 16-bit address to read from.
	 * @return						The 8-bit value stored at that address.
	 * @throw std::out_of_range		if the address is out of range.
	 */
	uint8_t read(uint16_t address) const;

	/**
	 * Writes one byte to memory.
	 *
	 * This function attempts to write one byte to memory.
	 *
	 * @param address			The 16-bit address to write to.
	 * @param byte				The 8-bit value to be written.
	 * @throw std::out_of_range	if the address is out of range.
	 */
	void write(uint16_t address, uint8_t byte);

	/**
	 * Loads a program into memory.
	 *
	 * This function attempts to load a program into memory.
	 *
	 * @param filename			 The path to the file to be loaded into memory.
	 * @throw std::length_error  if the file is bigger than the allocated
	 *							 space in memory.
	 * @throw std::runtime_error if the file cannot be read.
	 */
	void load_program(const char* const filename);

private:
	std::array<uint8_t, MEM_ADDR_END - MEM_ADDR_BEG> m_data;
};

#endif