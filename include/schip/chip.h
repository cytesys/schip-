#pragma once

#ifndef CHIP_H
#define CHIP_H

#include <cstdint>
#include <array>
#include <chrono>
#include <atomic>

#include <schip/memory.h>

using Reg = uint16_t;
using GPReg = uint8_t;
using TimerReg = uint8_t;

enum GKState {
    GK_NOTHING = 0,
    GK_PRESSED
};

enum ChipState {
    CHIP_READY = 0,
    CHIP_RUNNING,
    CHIP_HALTED,
    CHIP_STOPPED
};

#pragma pack(push, 2)
union Opcode {
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
        uint8_t u: 4;
	};
#endif
	uint16_t packed;
};
#pragma pack(pop)

/**
 * A class to emulate the SChip interpreter.
 * 
 * @see https://en.wikipedia.org/wiki/CHIP-8#Registers
 * @see http://devernay.free.fr/hacks/chip8/C8TECH10.HTM#2.2
 * @see http://devernay.free.fr/hacks/chip8/C8TECH10.HTM#3.0
 */
class Chip {
public:
    static Chip& get_instance() {
        static Chip instance;
        return instance;
    }

	/**
    * Runs the emulator (starts a run loop).
    *
    * This will stop either when the SChip/Chip8 program exits or when stop() is called.
	*
    * @throws std::runtime_error if there is an invalid or unimplemented opcode.
    * @throws std::overflow_error if the stack is overflowed.
    * @throws std::underflow_error if the stack is underflowed.
    * @throws std::overflow_error if the screen buffer is overflowed.
    * @throws std::out_of_range if an RPL user flag index is out of range.
    * @throws std::out_of_range if a push operation is attempted with an out of range address.
    * @throws std::out_of_range if a pop operation is attempted with an out of range address.
	*/
	void run();

	/**
     * Sets the stop flag, breaking the run loop.
     */
    void stop() { m_stopflag.store(true); }

	/**
	 * Resets this instance.
	 */
	void reset();

private:
	// General purpose registers
    std::array<GPReg, 16> m_v{};

    // Address register (12 bits IRL, 16 bits here)
    Reg m_i{};

    // Stack pointer
    Reg m_sp{};

    // Program counter
    Reg m_pc{};

    // Timers
    TimerReg m_dtimer{};
    TimerReg m_stimer{};

    // RPL user flags (S-CHIP)
    std::array<Byte, 8> m_rpl{};

    std::atomic<bool> m_stopflag{false};
    std::chrono::high_resolution_clock::time_point m_lasttick{};

    // Contains the current opcode
    Opcode m_opc{};

    Bus& m_bus;

    ChipState m_chipstate{CHIP_READY};
    GKState m_key_state{GK_NOTHING};

    Chip() : m_bus(Bus::get_instance()) { reset(); }

    void not_implemented() const;

    void fetch();
    void decode();
    void update_timers();

    void push(Addr address);
    Addr pop();
    void pop(Reg&);

    inline void op_scrd();
    inline void op_clr();
    inline void op_ret();
    inline void op_scrr();
    inline void op_scrl();
    inline void op_exit();
    inline void op_dex();
    inline void op_eex();
    inline void op_jmp();
    inline void op_call();
    inline void op_seq_imm();
    inline void op_sne_imm();
    inline void op_seq();
    inline void op_ld();
    inline void op_add_imm();
    inline void op_mov();
    inline void op_or();
    inline void op_and();
    inline void op_xor();
    inline void op_add();
    inline void op_sub();
    inline void op_shr();
    inline void op_sbr();
    inline void op_shl();
    inline void op_sne();
    inline void op_ldi();
    inline void op_jmpr();
    inline void op_rand();
    inline void op_draw();
    inline void op_skp();
    inline void op_sknp();
    inline void op_get_delay();
    inline void op_get_key();
    inline void op_set_delay();
    inline void op_set_stimer();
    inline void op_addi();
    inline void op_ld_sprite();
    inline void op_ld_esprite();
    inline void op_set_bcd();
    inline void op_reg_dump();
    inline void op_reg_store();
    inline void op_reg_dump_rpl();
    inline void op_reg_store_rpl();
};

#endif
