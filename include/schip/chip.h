#pragma once

#ifndef CHIP_H
#define CHIP_H

#include <random>
#include <thread>
#include <chrono>
#include <atomic>

#include <schip/common.h>
#include <schip/memory.h>
#include <schip/display.h>

#pragma pack(push, 1)
union OPC {
#ifdef __BIG_ENDIAN__
	struct {
		uint8_t h : 4;
		uint8_t x : 4;
		uint8_t y : 4;
		uint8_t l : 4;
};
	struct {
		uint8_t uu;
		uint8_t kk;
	};
	struct {
		uint16_t n : 4;
		uint8_t nnn : 12;
	};
#else
	struct {
		uint8_t n : 4;
		uint8_t y : 4;
		uint8_t x : 4;
		uint8_t o : 4;
	};
	struct {
		uint8_t kk;
		uint8_t uu;
	};
	struct {
		uint16_t nnn : 12;
		uint8_t z: 4;
	};
#endif
	uint16_t packed;
};
#pragma pack(pop)

#define CHIP_VX (this->m_v.at(this->m_opc.x))
#define CHIP_VY (this->m_v.at(this->m_opc.y))
#define CHIP_VF (this->m_v.at(0xf))

/**
 * A class to emulate the SChip interpreter.
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
	 * @param bus A pointer to a Bus instance.
	 * @see Bus
	 */
	Chip(Bus& bus);

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
	 * Sets the stop flag for this instance.
	 */
	void stop();

	/**
	 * Resets this instance.
	 */
	void reset();

private:
	// General purpose registers
	std::array<uint8_t, 16> m_v = {};

	// Address register (12 bits)
	uint16_t m_i;

	// Stack pointer
	uint16_t m_sp;

	// Program counter
	uint16_t m_pc;

	// Timers
	uint8_t m_dtimer;
	uint8_t m_stimer;

	// RPL user flags
	std::array<uint8_t, 8> m_rpl = {};

	Bus& m_bus;

	std::atomic<bool> m_stopflag = {false};
	std::chrono::high_resolution_clock::time_point m_lasttick;

#pragma pack(push, 1)
	OPC m_opc;
#pragma pack(pop)
	
	bool m_ext = false;

	void m_notimpl();

	void m_fetch();
	void m_decode();
	void m_update_timers();

	void m_push(uint16_t address);
	uint16_t m_pop();

	void op_scrd();
	void op_clr();
	void op_ret();
	void op_scrr();
	void op_scrl();
	void op_exit();
	void op_dex();
	void op_eex();
	void op_jmp();
	void op_call();
	void op_seq_imm();
	void op_sne_imm();
	void op_seq();
	void op_ld();
	void op_add_imm();
	void op_mov();
	void op_or();
	void op_and();
	void op_xor();
	void op_add();
	void op_sub();
	void op_shr();
	void op_sbr();
	void op_shl();
	void op_sne();
	void op_ldi();
	void op_jmpr();
	void op_rand();
	void op_draw();
	void op_skp();
	void op_sknp();
	void op_get_delay();
	void op_get_key();
	void op_set_delay();
	void op_set_stimer();
	void op_addi();
	void op_ld_sprite();
	void op_ld_esprite();
	void op_set_bcd();
	void op_reg_dump();
	void op_reg_store();
	void op_reg_dump_rpl();
	void op_reg_store_rpl();
};

#endif
