#include <cstdint>
#include <array>
#include <stdexcept>
#include <random>
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <mutex>

#include "chip.hpp"
#include "common.hpp"
#include "display.hpp"

auto _lasttick = std::chrono::high_resolution_clock::now();

struct Chip::Impl {
	Impl(MMU& mmu, Chip& chip);
	~Impl() {};

	// General purpose registers
	std::array<uint8_t, 16> reg_v = {};

	// Address register (12 bits)
	uint16_t reg_i;

	// Stack pointer
	uint16_t reg_sp;

	// Program counter
	uint16_t reg_pc;

	// Timers
	uint8_t d_timer;
	uint8_t s_timer;

	// RPL user flags
	std::array<uint8_t, 8> rpl = {};

	MMU& p_mmu;
	Chip& p_chip;

	std::atomic<bool> stopflag = {false};

	uint16_t opcode;
	bool ext = false;

	void fetch();
	void decode();
	void update_timers();

	void push(uint16_t address);
	uint16_t pop();

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

Chip::Impl::Impl(MMU& mmu, Chip& chip) : p_mmu(mmu), p_chip(chip) {
	this->reg_v.fill(0);
	this->reg_i = 0;
	this->reg_sp = 0xea0;
	this->reg_pc = 0x200;
	this->d_timer = 0;
	this->s_timer = 0;
}

void Chip::Impl::fetch() {
	this->opcode = static_cast<uint16_t>(this->p_mmu.read(this->reg_pc++)) << 8;
	this->opcode |= static_cast<uint16_t>(this->p_mmu.read(this->reg_pc++)) & 0xff;
}

void Chip::Impl::decode() {
	/*std::cout << "Opcode: " << n2hexstr(this->opcode);
	std::cout << " @0x" << n2hexstr(this->reg_pc - 2) << std::endl;*/

	switch (this->opcode & 0xf000) {
		case 0x0000:
			if ((this->opcode & 0xfff0) == 0x00c0) {
				return this->op_scrd();
			}

			switch (this->opcode) {
				case 0x00e0:
					return this->op_clr();
				case 0x00ee:
					return this->op_ret();
				case 0x00fb:
					return this->op_scrr();
				case 0x00fc:
					return this->op_scrl();
				case 0x00fd:
					return this->op_exit();
				case 0x00fe:
					return this->op_dex();
				case 0x00ff:
					return this->op_eex();
			}
			break;
		case 0x1000:
			return this->op_jmp();
		case 0x2000:
			return this->op_call();
		case 0x3000:
			return this->op_seq_imm();
		case 0x4000:
			return this->op_sne_imm();
		case 0x5000:
			if ((this->opcode & 0xf) == 0) {
				return this->op_seq();
			}
			break;
		case 0x6000:
			return this->op_ld();
		case 0x7000:
			return this->op_add_imm();
		case 0x8000:
			switch (this->opcode & 0xf) {
				case 0x0:
					return this->op_mov();
				case 0x1:
					return this->op_or();
				case 0x2:
					return this->op_and();
				case 0x3:
					return this->op_xor();
				case 0x4:
					return this->op_add();
				case 0x5:
					return this->op_sub();
				case 0x6:
					return this->op_shr();
				case 0x7:
					return this->op_sbr();
				case 0xe:
					return this->op_shl();
			}
			break;
		case 0x9000:
			return this->op_sne();
		case 0xa000:
			return this->op_ldi();
		case 0xb000:
			return this->op_jmpr();
		case 0xc000:
			return this->op_rand();
		case 0xd000:
			return this->op_draw();
		case 0xe000:
			switch (this->opcode & 0xff) {
				case 0x9e:
					return this->op_skp();
				case 0xA1:
					return this->op_sknp();
			}
			break;
		case 0xf000:
			switch (this->opcode & 0xff) {
				case 0x07:
					return this->op_get_delay();
				case 0x0a:
					return this->op_get_key();
				case 0x15:
					return this->op_set_delay();
				case 0x18:
					return this->op_set_stimer();
				case 0x1e:
					return this->op_addi();
				case 0x29:
					return this->op_ld_sprite();
				case 0x30:
					return this->op_ld_esprite();
				case 0x33:
					return this->op_set_bcd();
				case 0x55:
					return this->op_reg_dump();
				case 0x65:
					return this->op_reg_store();
				case 0x75:
					return this->op_reg_dump_rpl();
				case 0x85:
					return this->op_reg_store_rpl();
			}
			break;
	}

	throw std::runtime_error(
		"Opcode is not implemented: 0x" + n2hexstr(this->opcode)
	);
}

void Chip::Impl::update_timers() {
	// The timers are decremented at a rate of 60Hz if possible.
	auto stop = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - _lasttick);

	if (duration >= std::chrono::microseconds(16666)) {
		if (this->d_timer > 0) {
			this->d_timer--;
		}

		if (this->s_timer > 0) {
			this->s_timer--;
		}
		_lasttick = stop;
	}
}

void Chip::Impl::push(uint16_t address) {
	// Pushes an address onto the stack.
	if (this->reg_sp == 0x1000) {
		// Cannot push another address onto the stack when it is full.
		throw std::overflow_error("The stack pointer has overflowed");
	}

	if (address > 0xfff) {
		throw std::out_of_range("Cannot push address 0x" + n2hexstr(address));
	}
	
	this->p_mmu.write(
		this->reg_sp++,
		(address & 0xff00) >> 8
	);

	this->p_mmu.write(
		this->reg_sp++,
		address & 0xff
	);
}

uint16_t Chip::Impl::pop() {
	// Pops an address off the stack and returns it.
	if (this->reg_sp == 0x0ea0) {
		// Cannot pop an address off the stack when it's empty.
		throw std::underflow_error("The stack pointer has underflowed!");
	}

	uint16_t address = this->p_mmu.read(
		--this->reg_sp
	);

	address |= this->p_mmu.read(
		--this->reg_sp
	) << 8;

	if (address > 0xfff) {
		throw std::out_of_range("Cannot pop address 0x" + n2hexstr(address));
	}

	return address;
}

void Chip::Impl::op_scrd() {
	// 0x00Cn
	// Scrolls the display n pixels down
	int x, y, oldc, newc;
	uint8_t n = this->opcode & 0xf;

	std::lock_guard lck(Display::lock);

	for (y = SCR_HEIGHT - 1 - n; y >= 0; y--) {
		for (x = 0; x < SCR_WIDTH; x++) {
			// Move every pixel row down
			oldc = (y * SCR_WIDTH) + x;
			newc = ((y + n) * SCR_WIDTH) + x;
			Display::buffer.at(newc) = Display::buffer.at(oldc);
			Display::buffer.at(oldc) = false;
		}
	}
}

void Chip::Impl::op_clr() {
	// 0x00E0
	// Clears the display.
	std::lock_guard lck(Display::lock);
	Display::buffer.fill(false);
}

void Chip::Impl::op_ret() {
	// 0x00EE
	// Returns from a subroutine.
	this->reg_pc = this->pop();
}

void Chip::Impl::op_scrr() {
	// 0x00FB
	// Scrolls screen 4 pixels right
	int x, y, oldc, newc;

	std::lock_guard lck(Display::lock);

	for (x = SCR_WIDTH - 5; x >= 0; x--) {
		for (y = 0; y < SCR_HEIGHT; y++) {
			oldc = (y * SCR_WIDTH) + x;
			newc = (y * SCR_WIDTH) + (x + 4);
			Display::buffer.at(newc) = Display::buffer.at(oldc);
			Display::buffer.at(oldc) = false;
		}
	}
}

void Chip::Impl::op_scrl() {
	// 0x00FC
	// Scrolls screen 4 pixels left
	int x, y, oldc, newc;

	std::lock_guard lck(Display::lock);

	for (x = 4; x < SCR_WIDTH; x++) {
		for (y = 0; y < SCR_HEIGHT; y++) {
			oldc = (y * SCR_WIDTH) + x;
			newc = (y * SCR_WIDTH) + (x - 4);
			Display::buffer.at(newc) = Display::buffer.at(oldc);
			Display::buffer.at(oldc) = false;
		}
	}
}

void Chip::Impl::op_exit() {
	// 0x00FD
	// Exits the interpreter
	this->p_chip.stop();
}

void Chip::Impl::op_dex() {
	// 0x00FE
	// Disables extended screen mode
	this->ext = false;
}

void Chip::Impl::op_eex() {
	// 0x00FF
	// Enables extended screen mode
	this->ext = true;
}

void Chip::Impl::op_jmp() {
	// 0x1nnn
	// Jumps to address nnn.
	uint16_t tmp = this->opcode & 0xfff;
	if ((this->reg_pc - 2) == tmp) {
		// Infinite loop
		std::cerr << "An infinite loop was detected" << std::endl;
		this->p_chip.stop();
	}

	this->reg_pc = tmp;
}

void Chip::Impl::op_call() {
	// 0x2nnn
	// Calls subroutine at address nnn.
	uint16_t tmp = this->opcode & 0xfff;
	if ((this->reg_pc - 2) == tmp) {
		// Infinite loop
		std::cerr << "An infinite loop was detected" << std::endl;
		this->p_chip.stop();
	}

	this->push(this->reg_pc);
	this->reg_pc = tmp;
}

void Chip::Impl::op_seq_imm() {
	// 0x3xnn
	// Skips an instruction if Vx == nn.
	if (this->reg_v.at((this->opcode & 0xf00) >> 8) ==
		(this->opcode & 0xff)) {
		this->reg_pc += 2;
	}
}

void Chip::Impl::op_sne_imm() {
	// 0x4xnn
	// Skips an instruction if Vx != nn.
	if (this->reg_v.at((this->opcode & 0xf00) >> 8) !=
		(this->opcode & 0xff)) {
		this->reg_pc += 2;
	}
}

void Chip::Impl::op_seq() {
	// 0x5xy0
	// Skips an instruction if Vx == Vy.
	if (this->reg_v.at((this->opcode & 0xf00) >> 8) ==
		this->reg_v.at((this->opcode & 0xf0) >> 4)) {
		this->reg_pc += 2;
	}
}

void Chip::Impl::op_ld() {
	// 0x6xnn
	// Sets Vx = nn.
	this->reg_v.at((this->opcode & 0xf00) >> 8) = this->opcode & 0xff;
}

void Chip::Impl::op_add_imm() {
	// 0x7xnn
	// Sets Vx += nn.
	this->reg_v.at((this->opcode & 0xf00) >> 8) += this->opcode & 0xff;
}

void Chip::Impl::op_mov() {
	// 0x8xy0
	// Sets Vx = Vy.
	this->reg_v.at(
		(this->opcode & 0xf00) >> 8
	) = this->reg_v.at((this->opcode & 0xf0) >> 4);
}

void Chip::Impl::op_or() {
	// 0x8xy1
	// Sets Vx |= Vy.
	this->reg_v.at(
		(this->opcode & 0xf00) >> 8
	) |= this->reg_v.at((this->opcode & 0xf0) >> 4);
}

void Chip::Impl::op_and() {
	// 0x8xy2
	// Sets Vx &= Vy.
	this->reg_v.at(
		(this->opcode & 0xf00) >> 8
	) &= this->reg_v.at((this->opcode & 0xf0) >> 4);
}

void Chip::Impl::op_xor() {
	// 0x8xy3
	// Sets Vx ^= Vy.
	this->reg_v.at(
		(this->opcode & 0xf00) >> 8
	) ^= this->reg_v.at((this->opcode & 0xf0) >> 4);
}

void Chip::Impl::op_add() {
	// 0x8xy4
	// Sets Vx += Vy. VF is set if carry.
	uint8_t x = (this->opcode & 0xf00) >> 8;
	uint8_t y = (this->opcode & 0xf0) >> 4;

	if (static_cast<unsigned int>(
		this->reg_v.at(x)
		) + this->reg_v.at(y) > 0xff) {
		// Carry
		this->reg_v.at(0xf) = 1;
	} else {
		this->reg_v.at(0xf) = 0;
	}

	this->reg_v.at(x) += this->reg_v.at(y);
}

void Chip::Impl::op_sub() {
	// 0x8xy5
	// Sets Vx -= Vy. VF is set if no borrow.
	uint8_t x = (this->opcode & 0xf00) >> 8;
	uint8_t y = (this->opcode & 0xf0) >> 4;

	if (this->reg_v.at(x) < this->reg_v.at(y)) {
		// Borrow
		this->reg_v.at(0xf) = 0;
	} else {
		this->reg_v.at(0xf) = 1;
	}

	this->reg_v.at(x) -= this->reg_v.at(y);
}

void Chip::Impl::op_shr() {
	// 0x8xy6
	// Sets Vx >>= 1. VF is set to the least significant bit  of Vx.
	// TODO: quirks
	uint8_t x = (this->opcode & 0xf00) >> 8;

	this->reg_v.at(0xf) = this->reg_v.at(x) & 0x1;
	this->reg_v.at(x) >>= 1;
}

void Chip::Impl::op_sbr() {
	// 0x8xy7
	// Sets Vx = Vy - Vx. VF is set if no borrow.
	uint8_t x = (this->opcode & 0xf00) >> 8;
	uint8_t y = (this->opcode & 0xf0) >> 4;

	if (this->reg_v.at(y) < this->reg_v.at(x)) {
		// Borrow
		this->reg_v.at(0xf) = 0;
	} else {
		this->reg_v.at(0xf) = 1;
	}

	this->reg_v.at(x) = this->reg_v.at(y) - this->reg_v.at(x);
}

void Chip::Impl::op_shl() {
	// 0x8xyE
	// Sets Vx <<= 1. VF is set to the most significant bit  of Vx.
	// TODO: quirks
	uint8_t x = (this->opcode & 0xf00) >> 8;

	this->reg_v.at(0xf) = (this->reg_v.at(x) & 0x80) >> 7;
	this->reg_v.at(x) <<= 1;
}

void Chip::Impl::op_sne() {
	// 0x9xy0
	// Skips an instruction if Vx != Vy.
	if (this->reg_v.at((this->opcode & 0xf00) >> 8) !=
		this->reg_v.at((this->opcode & 0xf0) >> 4)) {
		this->reg_pc += 2;
	}
}

void Chip::Impl::op_ldi() {
	// 0xAnnn
	// Sets I to the address nnn.
	this->reg_i = this->opcode & 0xfff;
}

void Chip::Impl::op_jmpr() {
	// 0xBnnn
	// Jumps to the address V0 + nnn
	uint16_t tmp = this->reg_v.at(0) + (this->opcode & 0xfff);
	if ((this->reg_pc - 2) == tmp) {
		// Infinite loop
		std::cerr << "An infinite loop was detected" << std::endl;
		this->p_chip.stop();
	}

	this->reg_pc = tmp;
}

void Chip::Impl::op_rand() {
	// 0xCxnn
	// Sets Vx to rand() & nn.
	std::random_device dev;
	std::mt19937 rng(dev());
	std::uniform_int_distribution<std::mt19937::result_type> dist(0, 255);

	this->reg_v.at((this->opcode & 0xf00) >> 8) = dist(rng) & (this->opcode & 0xff);
}

void Chip::Impl::op_draw() {
	// 0xDxyn
	// Draws a sprite from memory address I to the screen.
	// Each pixel are interpreted as a bit. If a bit is flipped from 1 to 0,
	// VF is set to 1, else 0.
	unsigned int i, j, k, l, c, ny, nx;
	uint8_t row, x, y, n;
	uint16_t drow;
	bool pix;

	x = this->reg_v.at((this->opcode & 0xf00) >> 8);
	y = this->reg_v.at((this->opcode & 0xf0) >> 4);
	n = this->opcode & 0xf;

	this->reg_v.at(0xf) = 0;

	std::lock_guard lck(Display::lock);

	if (n == 0 && this->ext) {
		// 16*16 sprite
		for (i = 0; i < 16; i++) {
			drow = static_cast<uint16_t>(this->p_mmu.read(this->reg_i + (i * 2))) << 8;
			drow |= static_cast<uint16_t>(this->p_mmu.read(this->reg_i + (i * 2) + 1)) & 0xff;

			for (j = 0; j < 16; j++) {
				c = (((y + i) % SCR_HEIGHT) * SCR_WIDTH) + ((x + j) % SCR_WIDTH);
				pix = (drow >> (15 - j)) & 1;

				if (c >= Display::buffer.size()) {
					// Screen overflow
					// Should not happen if c is calculated correctly.
					throw std::overflow_error("In draw: screen buffer overflowed: c = 0x" + n2hexstr(c));
				}

				if (Display::buffer.at(c) && pix) {
					// Collision
					this->reg_v.at(0xf) = 1;
				}

				Display::buffer.at(c) ^= pix;
			}
		}
	} else if (n == 0) {
		// N must be in the range 1..15
		std::cerr << "Warning: in draw: n was 0 while not in extended mode!" << std::endl;
		return;

	} else {
		for (i = 0; i < n; i++) {
			row = this->p_mmu.read(this->reg_i + i);
			for (j = 0; j < 8; j++) {
				pix = (row >> (7 - j)) & 1;
				if (this->ext) {
					// Extended mode
					c = (((y + i) % SCR_HEIGHT) * SCR_WIDTH) + ((x + j) % SCR_WIDTH);

					if (c >= Display::buffer.size()) {
						// Screen overflow
						// Should not happen if c is calculated correctly.
						throw std::overflow_error("In draw: screen buffer overflowed: c = 0x" + n2hexstr(c));
					}

					if (Display::buffer.at(c) && pix) {
						// Collision
						this->reg_v.at(0xf) = 1;
					}

					Display::buffer.at(c) ^= pix;
				} else {
					// Normal mode
					for (k = 0; k < 2; k++) {
						for (l = 0; l < 2; l++) {
							// Draw 2*2 pixels
							ny = ((((y + i) * 2) + k) % SCR_HEIGHT);
							nx = ((((x + j) * 2) + l) % SCR_WIDTH);
							c = (ny * SCR_WIDTH) + nx;

							if (c >= Display::buffer.size()) {
								// Screen overflow
								// Should not happen if c is calculated correctly.
								throw std::overflow_error("In draw: screen buffer overflowed: c = 0x" + n2hexstr(c));
							}

							if (Display::buffer.at(c) && pix) {
								// Collision
								this->reg_v.at(0xf) = 1;
							}

							Display::buffer.at(c) ^= pix;
						}
					}
				}
			}
		}
	}
}

void Chip::Impl::op_skp() {
	// 0xEx9E
	// Skips the next instruction if the key in Vx is pressed.
	uint8_t x = this->reg_v.at((this->opcode & 0xf00) >> 8);

	std::lock_guard lck(Display::lock);

	if (Display::keys.at(x)) {
		this->reg_pc += 2;
		Display::keys.at(x) = false;
	}
}

void Chip::Impl::op_sknp() {
	// 0xExA1
	// Skips the next instruction if the key in Vx is not pressed.
	std::lock_guard lck(Display::lock);

	if (!Display::keys.at(this->reg_v.at((this->opcode & 0xf00) >> 8))) {
		this->reg_pc += 2;
	}
}

void Chip::Impl::op_get_delay() {
	// 0xFx07
	// Sets Vx = delay timer
	this->reg_v.at((this->opcode & 0xf00) >> 8) = this->d_timer;
}

void Chip::Impl::op_get_key() {
	// 0xFx0A
	// Await a keypress and store it into Vx. Blocking operation.
	bool pressed = false;
	unsigned int i;

	while (!pressed && !this->stopflag.load()) {
		Display::lock.lock();
		for (i = 0; i < Display::keys.size(); i++) {
			if (Display::keys.at(i)) {
				// A key is pressed
				this->reg_v.at((this->opcode & 0xf00) >> 8) = i;
				Display::keys.at(i) = false;
				pressed = true;
				break;
			}
		}
		Display::lock.unlock();

		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}

void Chip::Impl::op_set_delay() {
	// 0xFx15
	// Sets delay timer to Vx.
	this->d_timer = this->reg_v.at((this->opcode & 0xf00) >> 8);
}

void Chip::Impl::op_set_stimer() {
	// 0xFx18
	// Sets sound timer to Vx.
	this->s_timer = this->reg_v.at((this->opcode & 0xf00) >> 8);
}

void Chip::Impl::op_addi() {
	// 0xFx1E
	// Adds Vx to I. VF is set to 1 if carry.
	this->reg_i += this->reg_v.at(
		(this->opcode & 0xf00) >> 8
	);

	this->reg_v.at(0xf) = (this->reg_i > 0xfff) ? 1 : 0;

	this->reg_i %= 0x1000;
}

void Chip::Impl::op_ld_sprite() {
	// 0xFx29
	// Sets I to the address of the 5-byte sprite of the hex character in Vx.
	uint8_t x = this->reg_v.at((this->opcode & 0xf00) >> 8);
	this->reg_i = x * 5;
}

void Chip::Impl::op_ld_esprite() {
	// 0xFx30
	// Sets I to the address of the 10-byte sprite of the character in Vx.
	uint8_t x = this->reg_v.at((this->opcode & 0xf00) >> 8);
	this->reg_i = (x * 10) + 80;
}

void Chip::Impl::op_set_bcd() {
	// 0xFx33
	// Stores the BCD representation of Vx into memory starting at
	// address I.
	uint8_t tmp = this->reg_v.at((this->opcode & 0xf00) >> 8);

	this->p_mmu.write(
		this->reg_i,
		tmp / 100
	);

	this->p_mmu.write(
		this->reg_i + 1,
		(tmp % 100) / 10
	);

	this->p_mmu.write(
		this->reg_i + 2,
		tmp % 10
	);
}

void Chip::Impl::op_reg_dump() {
	// 0xFx55
	// Stores V0..Vx into memory starting at address I.
	// TODO: quirks
	unsigned int i;
	uint8_t x = (this->opcode & 0xf00) >> 8;

	for (i = 0; i <= x; i++) {
		this->p_mmu.write(
			this->reg_i + i,
			this->reg_v.at(i)
		);
	}
}

void Chip::Impl::op_reg_store() {
	// 0xFx65
	// Fills V0..Vx from memory starting at address I.
	// TODO: quirks
	unsigned int i;
	uint8_t x = (this->opcode & 0xf00) >> 8;

	for (i = 0; i <= x; i++) {
		this->reg_v.at(i) = this->p_mmu.read(
			this->reg_i + i
		);
	}
}

void Chip::Impl::op_reg_dump_rpl() {
	// 0xFx75
	// Stores V0..Vx into RPL user flags (x <= 7).
	unsigned int i;
	uint8_t x = (this->opcode & 0xf00) >> 8;

	if (x > 7) {
		throw std::out_of_range("The length of the RPL is 8; x was " + n2hexstr(x));
	}

	for (i = 0; i <= x; i++) {
		this->rpl.at(i) = this->reg_v.at(i);
	}
}

void Chip::Impl::op_reg_store_rpl() {
	// 0xFx85
	// Fills V0..Vx from RPL user flags (x <= 7).
	unsigned int i;
	uint8_t x = (this->opcode & 0xf00) >> 8;

	if (x > 7) {
		throw std::out_of_range("The length of the RPL is 8; x was " + n2hexstr(x));
	}

	for (i = 0; i <= x; i++) {
		this->reg_v.at(i) = this->rpl.at(i);
	}
}

Chip::Chip(MMU& mmu) : pimpl(new Impl(mmu, *this)) {}

Chip::~Chip() {
	pimpl.reset();
}

void Chip::stop() {
	this->pimpl->stopflag.store(true);
}

void Chip::run() {
	while (!this->pimpl->stopflag.load()) {
		auto start = std::chrono::high_resolution_clock::now();

		this->pimpl->fetch();
		this->pimpl->decode();
		this->pimpl->update_timers();

		auto stop = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);

		std::this_thread::sleep_for(std::chrono::microseconds(200) - duration);
	}

	std::cout << "The chip has stopped." << std::endl;
}
