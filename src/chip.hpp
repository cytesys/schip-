#pragma once

#ifndef SCPP_CHIP
#define SCPP_CHIP

#include <memory>

#include "memory.hpp"

/**
 * The SChip interpreter.
 * 
 * This class is actually a wrapper around a SChip interpreter.
 * The actual implementation is in Chip::Impl.
 * 
 * @see https://en.wikipedia.org/wiki/CHIP-8#Registers
 * @see http://devernay.free.fr/hacks/chip8/C8TECH10.HTM#2.2
 * @see http://devernay.free.fr/hacks/chip8/C8TECH10.HTM#3.0
 */
class Chip {
public:
	/**
	 * Initializes the interpreter.
	 *
	 * @param mmu A pointer to a MMU instance.
	 * @see MMU
	 */
	Chip(MMU& mmu);

	/**
	 * Destroys this interpreter object.
	 */
	~Chip();

	/**
	* Runs the interpreter.
	*
	* @throw std::runtime_error if there is an invalid or unimplemented opcode.
	* @throw std::overflow_error if the stack is overflowed.
	* @throw std::underflow_error if the stack is underflowed.
	* @throw std::overflow_error if the screen buffer is overflowed.
	* @throw std::out_of_range if an RPL user flag index is out of range.
	* @throw std::out_of_range if a push operation is attempted with an invalid address.
	* @throw std::out_of_range if a pop operation is attempted with an invalid address.
	*/
	void run();

	/**
	 * Stops the interpreter.
	 *
	 * The interpreter may not stop immediately. The current cycle/tick
	 * has to finish before this is actually stopped.
	 */
	void stop();

private:
	struct Impl;
	std::unique_ptr<Impl> pimpl;
};

#endif
