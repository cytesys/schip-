#include <schip/chip.h>

Chip::Chip(Bus& bus) : m_bus(bus) {
	this->reset();
}

Chip::~Chip() {}

void Chip::reset() {
	this->m_v.fill(0);
	this->m_i = 0;
	this->m_sp = 0xea0;
	this->m_pc = 0x200;
	this->m_dtimer = 0;
	this->m_stimer = 0;
}

void Chip::stop() {
	this->m_stopflag.store(true);
}

void Chip::run() {
	fprintf(stderr, "The SChip interpreter has started\n");
	this->m_lasttick = std::chrono::high_resolution_clock::now();
	while (!this->m_stopflag.load()) {
		auto start = std::chrono::high_resolution_clock::now();

		this->m_fetch();
		//fprintf(stderr, "[0x%.4x] OPC 0x%.4x\n", this->m_pc - 2, this->m_opc.packed);

		this->m_decode();
		this->m_update_timers();

		auto stop = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);

		std::this_thread::sleep_for(std::chrono::microseconds(200) - duration);
	}

	fprintf(stderr, "The chip has stopped\n");
}

void Chip::m_notimpl() {
	fprintf(stderr, "Opcode 0x%.4x is not implemented\n", this->m_opc.packed);
	throw std::runtime_error("Unknown opcode");
}

void Chip::m_fetch() {
	this->m_opc.packed = (uint16_t)(this->m_bus.read(this->m_pc++)) << 8;
	this->m_opc.packed |= this->m_bus.read(this->m_pc++) & 0xff;
}

void Chip::m_decode() {
	switch (this->m_opc.o) {
		case 0x0:
			if ((this->m_opc.packed & 0xfff0) == 0x00c0) {
				return this->op_scrd();
			}

			switch (this->m_opc.packed) {
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
		case 0x1:
			return this->op_jmp();
		case 0x2:
			return this->op_call();
		case 0x3:
			return this->op_seq_imm();
		case 0x4:
			return this->op_sne_imm();
		case 0x5:
			if (this->m_opc.n == 0) {
				return this->op_seq();
			}
			break;
		case 0x6:
			return this->op_ld();
		case 0x7:
			return this->op_add_imm();
		case 0x8:
			switch (this->m_opc.n) {
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
		case 0x9:
			return this->op_sne();
		case 0xa:
			return this->op_ldi();
		case 0xb:
			return this->op_jmpr();
		case 0xc:
			return this->op_rand();
		case 0xd:
			return this->op_draw();
		case 0xe:
			switch (this->m_opc.kk) {
				case 0x9e:
					return this->op_skp();
				case 0xa1:
					return this->op_sknp();
			}
			break;
		case 0xf:
			switch (this->m_opc.kk) {
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

	this->m_notimpl();
}

void Chip::m_update_timers() {
	// The timers are decremented at a rate of 60Hz if possible.
	auto stop = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - this->m_lasttick);

	if (duration >= std::chrono::microseconds(16666)) {
		if (this->m_dtimer > 0) {
			this->m_dtimer--;
		}

		if (this->m_stimer > 0) {
			this->m_stimer--;
		}
		this->m_lasttick = stop;
	}
}

void Chip::m_push(uint16_t address) {
	// Pushes an address onto the stack.
	if (this->m_sp > 0xffe) {
		// Cannot push another address onto the stack when it is full.
		throw std::overflow_error("The stack pointer has overflowed");
	}
	
	this->m_bus.write(this->m_sp++, (address & 0xff00) >> 8);
	this->m_bus.write(this->m_sp++, address & 0xff);
}

uint16_t Chip::m_pop() {
	// Pops an address off the stack and returns it.
	if (this->m_sp < 0x0ea2) {
		// Cannot pop an address off the stack when it's empty.
		throw std::underflow_error("The stack pointer has underflowed!");
	}

	return (uint16_t)(this->m_bus.read(--this->m_sp)) | ((uint16_t)(this->m_bus.read(--this->m_sp)) << 8);
}

void Chip::op_scrd() {
	// 0x00Cn
	// Scrolls the display n pixels down
	int x, y, oldc, newc;

	for (y = SCR_HEIGHT - 1 - this->m_opc.n; y >= 0; y--) {
		for (x = 0; x < SCR_WIDTH; x++) {
			// Move every pixel row down
			oldc = (y * SCR_WIDTH) + x;
			newc = ((y + this->m_opc.n) * SCR_WIDTH) + x;
			Display::buffer.at(newc) = Display::buffer.at(oldc);
			Display::buffer.at(oldc) = false;
		}
	}
}

void Chip::op_clr() {
	// 0x00E0
	// Clears the display.
	Display::buffer.fill(false);
}

void Chip::op_ret() {
	// 0x00EE
	// Returns from a subroutine.
	this->m_pc = this->m_pop();
}

void Chip::op_scrr() {
	// 0x00FB
	// Scrolls screen 4 pixels right
	int x, y, oldc, newc;

	for (x = SCR_WIDTH - 5; x >= 0; x--) {
		for (y = 0; y < SCR_HEIGHT; y++) {
			oldc = (y * SCR_WIDTH) + x;
			newc = (y * SCR_WIDTH) + (x + 4);
			Display::buffer.at(newc) = Display::buffer.at(oldc);
			Display::buffer.at(oldc) = false;
		}
	}
}

void Chip::op_scrl() {
	// 0x00FC
	// Scrolls screen 4 pixels left
	int x, y, oldc, newc;

	for (x = 4; x < SCR_WIDTH; x++) {
		for (y = 0; y < SCR_HEIGHT; y++) {
			oldc = (y * SCR_WIDTH) + x;
			newc = (y * SCR_WIDTH) + (x - 4);
			Display::buffer.at(newc) = Display::buffer.at(oldc);
			Display::buffer.at(oldc) = false;
		}
	}
}

void Chip::op_exit() {
	// 0x00FD
	// Exits the interpreter
	this->stop();
}

void Chip::op_dex() {
	// 0x00FE
	// Disables extended screen mode
	this->m_ext = false;
}

void Chip::op_eex() {
	// 0x00FF
	// Enables extended screen mode
	this->m_ext = true;
}

void Chip::op_jmp() {
	// 0x1nnn
	// Jumps to address nnn.
	if (this->m_opc.nnn == this->m_pc - 2) {
		// Infinite loop
		fprintf(stderr, "An infinite jump loop was detected\n");
		this->stop();
	}

	this->m_pc = this->m_opc.nnn;
}

void Chip::op_call() {
	// 0x2nnn
	// Calls subroutine at address nnn.
	if (this->m_opc.nnn == this->m_pc - 2) {
		// Infinite loop
		fprintf(stderr, "An infinite call loop was detected\n");
		this->stop();
	}

	this->m_push(this->m_pc);
	this->m_pc = this->m_opc.nnn;
}

void Chip::op_seq_imm() {
	// 0x3xnn
	// Skips an instruction if Vx == nn.
	if (CHIP_VX == this->m_opc.kk) {
		this->m_pc += 2;
	}
}

void Chip::op_sne_imm() {
	// 0x4xnn
	// Skips an instruction if Vx != nn.
	if (CHIP_VX != this->m_opc.kk) {
		this->m_pc += 2;
	}
}

void Chip::op_seq() {
	// 0x5xy0
	// Skips an instruction if Vx == Vy.
	if (CHIP_VX == CHIP_VY) {
		this->m_pc += 2;
	}
}

void Chip::op_ld() {
	// 0x6xnn
	// Sets Vx = nn.
	CHIP_VX = this->m_opc.kk;
}

void Chip::op_add_imm() {
	// 0x7xnn
	// Sets Vx += nn.
	CHIP_VX += this->m_opc.kk;
}

void Chip::op_mov() {
	// 0x8xy0
	// Sets Vx = Vy.
	CHIP_VX = CHIP_VY;
}

void Chip::op_or() {
	// 0x8xy1
	// Sets Vx |= Vy.
	CHIP_VX |= CHIP_VY;
}

void Chip::op_and() {
	// 0x8xy2
	// Sets Vx &= Vy.
	CHIP_VX &= CHIP_VY;
}

void Chip::op_xor() {
	// 0x8xy3
	// Sets Vx ^= Vy.
	CHIP_VX ^= CHIP_VY;
}

void Chip::op_add() {
	// 0x8xy4
	// Sets Vx += Vy. VF is set if carry.
	uint8_t cmp = CHIP_VX;
	CHIP_VX += CHIP_VY;
	CHIP_VF = (CHIP_VX < cmp) ? 1 : 0;
}

void Chip::op_sub() {
	// 0x8xy5
	// Sets Vx -= Vy. VF is set if no borrow.
	CHIP_VF = (CHIP_VX < CHIP_VY) ? 0 : 1;
	CHIP_VX -= CHIP_VY;
}

void Chip::op_shr() {
	// 0x8xy6
	// Sets Vx >>= 1. VF is set to the least significant bit of Vx.
	// TODO: quirks
	CHIP_VF = CHIP_VX & 0x1;
	CHIP_VX >>= 1;
}

void Chip::op_sbr() {
	// 0x8xy7
	// Sets Vx = Vy - Vx. VF is set if no borrow.
	CHIP_VF = (CHIP_VX > CHIP_VY) ? 0 : 1;
	CHIP_VX = CHIP_VY - CHIP_VX;
}

void Chip::op_shl() {
	// 0x8xyE
	// Sets Vx <<= 1. VF is set to the most significant bit  of Vx.
	// TODO: quirks
	CHIP_VF = CHIP_VX >> 7;
	CHIP_VX <<= 1;
}

void Chip::op_sne() {
	// 0x9xy0
	// Skips an instruction if Vx != Vy.
	if (CHIP_VX != CHIP_VY) {
		this->m_pc += 2;
	}
}

void Chip::op_ldi() {
	// 0xAnnn
	// Sets I to the address nnn.
	this->m_i = this->m_opc.nnn;
}

void Chip::op_jmpr() {
	// 0xBnnn
	// Jumps to the address V0 + nnn
	uint16_t addr = this->m_v.at(0) + this->m_opc.nnn;
	if (addr == this->m_pc - 2) {
		// Infinite loop
		fprintf(stderr, "An infinite jump loop was detected\n");
		this->stop();
	}

	this->m_pc = addr;
}

void Chip::op_rand() {
	// 0xCxnn
	// Sets Vx to rand() & nn.
	std::random_device dev;
	std::mt19937 rng(dev());
	std::uniform_int_distribution<std::mt19937::result_type> dist(0, 255);

	CHIP_VX = dist(rng) & this->m_opc.kk;
}

void Chip::op_draw() {
	// 0xDxyn
	// Draws a sprite from memory address I to the screen.
	// Each pixel are interpreted as a bit. If a bit is flipped from 1 to 0,
	// VF is set to 1, else 0.
	unsigned int i, j, k, l, c, ny, nx;
	uint8_t row, x, y, n;
	uint16_t drow;
	bool pix;

	x = CHIP_VX;
	y = CHIP_VY;
	n = this->m_opc.n;

	CHIP_VF = 0;

	if (n == 0 && this->m_ext) {
		// 16*16 sprite
		for (i = 0; i < 16; i++) {
			drow = (uint16_t)(this->m_bus.read(this->m_i + (i * 2))) << 8;
			drow |= (uint16_t)(this->m_bus.read(this->m_i + (i * 2) + 1)) & 0xff;

			for (j = 0; j < 16; j++) {
				c = (((y + i) % SCR_HEIGHT) * SCR_WIDTH) + ((x + j) % SCR_WIDTH);
				pix = (drow >> (15 - j)) & 1;

				if (c >= Display::buffer.size()) {
					// Screen overflow
					// Should not happen if c is calculated correctly.
					throw std::overflow_error("Draw(extended mode+): Screen buffer overflowed");
				}

				if (Display::buffer.at(c) && pix) {
					// Collision
					CHIP_VF = 1;
				}

				Display::buffer.at(c) ^= pix;
			}
		}
	} else if (n == 0) {
		// N must be in the range 1..15
		fprintf(stderr, "Warning: Draw: n was 0 while not in extended mode!\n");
		return;

	} else {
		for (i = 0; i < n; i++) {
			row = this->m_bus.read(this->m_i + i);
			for (j = 0; j < 8; j++) {
				pix = (row >> (7 - j)) & 1;
				if (this->m_ext) {
					// Extended mode
					c = (((y + i) % SCR_HEIGHT) * SCR_WIDTH) + ((x + j) % SCR_WIDTH);

					if (c >= Display::buffer.size()) {
						// Screen overflow
						// Should not happen if c is calculated correctly.
						throw std::overflow_error("Draw(extended mode): Screen buffer overflowed");
					}

					if (Display::buffer.at(c) && pix) {
						// Collision
						CHIP_VF = 1;
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
								throw std::overflow_error("Draw(normal mode): Screen buffer overflowed");
							}

							if (Display::buffer.at(c) && pix) {
								// Collision
								CHIP_VF = 1;
							}

							Display::buffer.at(c) ^= pix;
						}
					}
				}
			}
		}
	}
}

void Chip::op_skp() {
	// 0xEx9E
	// Skips the next instruction if the key in Vx is pressed.
	if (Display::keys.at(CHIP_VX)) {
		Display::keys.at(CHIP_VX) = false;
		this->m_pc += 2;
	}
}

void Chip::op_sknp() {
	// 0xExA1
	// Skips the next instruction if the key in Vx is not pressed.
	if (!Display::keys.at(CHIP_VX)) {
		this->m_pc += 2;
	}
}

void Chip::op_get_delay() {
	// 0xFx07
	// Sets Vx = delay timer
	CHIP_VX = this->m_dtimer;
}

void Chip::op_get_key() {
	// 0xFx0A
	// Await a keypress and store it into Vx. Blocking operation.
	int i = 0;

	while (!this->m_stopflag.load()) {
		if (Display::keys.at(i)) {
			// A key is pressed
			CHIP_VX = i;
			Display::keys.at(i) = false;
			break;
		}

		i = (i + 1) % NUM_KEYS;

		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}

void Chip::op_set_delay() {
	// 0xFx15
	// Sets delay timer to Vx.
	this->m_dtimer = CHIP_VX;
}

void Chip::op_set_stimer() {
	// 0xFx18
	// Sets sound timer to Vx.
	this->m_stimer = CHIP_VX;
}

void Chip::op_addi() {
	// 0xFx1E
	// Adds Vx to I. VF is set to 1 if carry.
	this->m_i += CHIP_VX;
	CHIP_VF = (this->m_i > 0xfff) ? 1 : 0;
	this->m_i %= 0x1000;
}

void Chip::op_ld_sprite() {
	// 0xFx29
	// Sets I to the address of the 5-byte sprite of the hex character in Vx.
	this->m_i = CHIP_VX * 5;
}

void Chip::op_ld_esprite() {
	// 0xFx30
	// Sets I to the address of the 10-byte sprite of the character in Vx.
	this->m_i = (CHIP_VX * 10) + 80;
}

void Chip::op_set_bcd() {
	// 0xFx33
	// Stores the BCD representation of Vx into memory starting at
	// address I.
	uint8_t tmp = CHIP_VX;

	this->m_bus.write(this->m_i, tmp / 100);
	this->m_bus.write(this->m_i + 1, (tmp % 100) / 10);
	this->m_bus.write(this->m_i + 2, tmp % 10);
}

void Chip::op_reg_dump() {
	// 0xFx55
	// Stores V0..Vx into memory starting at address I.
	// TODO: quirks
	for (int i = 0; i <= this->m_opc.x; i++) {
		this->m_bus.write(this->m_i + i, this->m_v.at(i));
	}
}

void Chip::op_reg_store() {
	// 0xFx65
	// Fills V0..Vx from memory starting at address I.
	// TODO: quirks
	for (int i = 0; i <= this->m_opc.x; i++) {
		this->m_v.at(i) = this->m_bus.read(this->m_i + i);
	}
}

void Chip::op_reg_dump_rpl() {
	// 0xFx75
	// Stores V0..Vx into RPL user flags (x < 8).
	if (this->m_opc.x >= 8) {
		throw std::out_of_range("Cannot dump to RPL; x is out of range");
	}

	for (int i = 0; i <= this->m_opc.x; i++) {
		this->m_rpl.at(i) = this->m_v.at(i);
	}
}

void Chip::op_reg_store_rpl() {
	// 0xFx85
	// Fills V0..Vx from RPL user flags (x < 8).
	if (this->m_opc.x >= 8) {
		throw std::out_of_range("Cannot load from RPL; x is out of range");
	}

	for (int i = 0; i <= this->m_opc.x; i++) {
		this->m_v.at(i) = this->m_rpl.at(i);
	}
}
