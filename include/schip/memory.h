#pragma once

#ifndef MEMORY_H
#define MEMORY_H

#include <cstdint>
#include <filesystem>

using Addr = uint16_t;
using Byte = uint8_t;

/**
 * This class emulates memory for SChip/Chip8.
 * 
 * @see https://en.wikipedia.org/wiki/CHIP-8#Memory
 * @see http://devernay.free.fr/hacks/chip8/C8TECH10.HTM#2.1
 */
class Bus {
public:
    static Bus& get_instance() {
        static Bus instance;
        return instance;
    }

	/**
	 * Reads one byte from memory.
	 *
	 * This function attempts to read one byte from memory.
	 *
	 * @param address				The 16-bit address to read from.
	 * @return						The 8-bit value stored at that address.
	 * @throw std::out_of_range		if the address is out of range.
	 */
    [[nodiscard]] Byte read(Addr) const;

	/**
	 * Writes one byte to memory.
	 *
	 * This function attempts to write one byte to memory.
	 *
	 * @param address			The 16-bit address to write to.
	 * @param byte				The 8-bit value to be written.
	 * @throw std::out_of_range	if the address is out of range.
	 */
    void write(Addr, Byte);

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
    void load_program(std::filesystem::path filename);

private:
    Bus();
    ~Bus();

    char* const m_data{nullptr};
};

#endif
