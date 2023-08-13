#include <random>
#include <algorithm>
#include <stdexcept>
#include <thread>
#include <iostream>

#include <schip/chip.h>
#include <schip/keypad.h>
#include <schip/display.h>

void Chip::reset() {
    m_v.fill(0);
    m_i = 0;
    m_sp = 0xea0;
    m_pc = 0x200;
    m_dtimer = 0;
    m_stimer = 0;
}

void Chip::run() {
    m_chipstate = CHIP_RUNNING;
    std::cout << "The SChip interpreter has started" << std::endl;

    m_lasttick = std::chrono::high_resolution_clock::now();
    PPU::get_instance().disable_extended();

    try {
        while (!m_stopflag.load()) {
            auto start = std::chrono::high_resolution_clock::now();

            if (m_chipstate != CHIP_HALTED)
                fetch();
            decode();
            update_timers();

            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::high_resolution_clock::now() - start
            );

            // 200 us for each instruction was arbitrarily chosen. Idk how fast it should be..
            std::this_thread::sleep_for(std::chrono::microseconds(200) - duration);
        }
    } catch(std::exception& err) {
        std::cerr << "Chip error: " << err.what() << std::endl;
    }

    m_chipstate = CHIP_STOPPED;
    std::cout << "The SChip interpreter has stopped" << std::endl;
}

void Chip::not_implemented() const {
#if(DEBUG)
    printf("Opcode 0x%04x is not implemented\n", m_opc.packed);
#endif
	throw std::runtime_error("Unknown opcode");
}

void Chip::fetch() {
    m_opc.uu = m_bus.read(m_pc++);
    m_opc.kk = m_bus.read(m_pc++);
}

void Chip::decode() {
    switch (m_opc.o) {
		case 0x0:
            if ((m_opc.packed & 0xfff0) == 0x00c0)
                return op_scrd();

            switch (m_opc.packed) {
				case 0x00e0:
                    return op_clr();
				case 0x00ee:
                    return op_ret();
				case 0x00fb:
                    return op_scrr();
				case 0x00fc:
                    return op_scrl();
				case 0x00fd:
                    return op_exit();
				case 0x00fe:
                    return op_dex();
				case 0x00ff:
                    return op_eex();
            }

            break;
		case 0x1:
            return op_jmp();
		case 0x2:
            return op_call();
		case 0x3:
            return op_seq_imm();
		case 0x4:
            return op_sne_imm();
		case 0x5:
            if (m_opc.n == 0) {
                return op_seq();
			}
			break;
		case 0x6:
            return op_ld();
		case 0x7:
            return op_add_imm();
		case 0x8:
            switch (m_opc.n) {
				case 0x0:
                    return op_mov();
				case 0x1:
                    return op_or();
				case 0x2:
                    return op_and();
				case 0x3:
                    return op_xor();
				case 0x4:
                    return op_add();
				case 0x5:
                    return op_sub();
				case 0x6:
                    return op_shr();
				case 0x7:
                    return op_sbr();
				case 0xe:
                    return op_shl();
			}
			break;
		case 0x9:
            return op_sne();
		case 0xa:
            return op_ldi();
		case 0xb:
            return op_jmpr();
		case 0xc:
            return op_rand();
		case 0xd:
            return op_draw();
		case 0xe:
            switch (m_opc.kk) {
				case 0x9e:
                    return op_skp();
				case 0xa1:
                    return op_sknp();
			}
			break;
		case 0xf:
            switch (m_opc.kk) {
				case 0x07:
                    return op_get_delay();
				case 0x0a:
                    return op_get_key();
				case 0x15:
                    return op_set_delay();
				case 0x18:
                    return op_set_stimer();
				case 0x1e:
                    return op_addi();
				case 0x29:
                    return op_ld_sprite();
				case 0x30:
                    return op_ld_esprite();
				case 0x33:
                    return op_set_bcd();
				case 0x55:
                    return op_reg_dump();
				case 0x65:
                    return op_reg_store();
				case 0x75:
                    return op_reg_dump_rpl();
				case 0x85:
                    return op_reg_store_rpl();
			}
			break;
	}

    not_implemented();
}

void Chip::update_timers() {
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::high_resolution_clock::now() - m_lasttick
    );

    // The timers are decremented at a rate of 60Hz if possible.
    if (duration >= std::chrono::microseconds(1'000'000 / 60)) {
        if (m_dtimer > 0) --m_dtimer;
        if (m_stimer > 0) --m_stimer;

        m_lasttick = std::chrono::high_resolution_clock::now();
	}
}

void Chip::push(Addr address) {
    if (m_sp > 0xffe)
        throw std::overflow_error("The stack pointer has overflowed");
	
    m_bus.write(m_sp++, (address & 0xff00) >> 8);
    m_bus.write(m_sp++, address & 0xff);
}

Addr Chip::pop() {
    if (m_sp < 0x0ea2)
		throw std::underflow_error("The stack pointer has underflowed!");

    Addr popped = m_bus.read(--m_sp);
    return popped | (Addr)(m_bus.read(--m_sp)) << 8;
}

void Chip::pop(Reg& reg) { reg = pop(); }

void Chip::op_scrd() {
	// 0x00Cn
	// Scrolls the display n pixels down
    PPU::get_instance().scroll_down(m_v[m_opc.x]);
}

void Chip::op_clr() {
	// 0x00E0
	// Clears the display.
    PPU::get_instance().clear_screen();
}

void Chip::op_ret() {
	// 0x00EE
	// Returns from a subroutine.
    pop(m_pc);
}

void Chip::op_scrr() {
	// 0x00FB
	// Scrolls screen 4 pixels right
    PPU::get_instance().scroll_right();
}

void Chip::op_scrl() {
	// 0x00FC
	// Scrolls screen 4 pixels left
    PPU::get_instance().scroll_left();
}

void Chip::op_exit() {
	// 0x00FD
	// Exits the interpreter
    stop();
}

void Chip::op_dex() {
	// 0x00FE
	// Disables extended screen mode
    PPU::get_instance().disable_extended();
}

void Chip::op_eex() {
	// 0x00FF
	// Enables extended screen mode
    PPU::get_instance().enable_extended();
}

void Chip::op_jmp() {
	// 0x1nnn
	// Jumps to address nnn.
    if (m_opc.nnn == m_pc - 2) stop(); // Infinite loop
    m_pc = m_opc.nnn;
}

void Chip::op_call() {
	// 0x2nnn
	// Calls subroutine at address nnn.
    if (m_opc.nnn == m_pc - 2) stop(); // Infinite loop
    push(m_pc);
    m_pc = m_opc.nnn;
}

void Chip::op_seq_imm() {
	// 0x3xnn
	// Skips an instruction if Vx == nn.
    if (m_v[m_opc.x] == m_opc.kk) m_pc += 2;
}

void Chip::op_sne_imm() {
	// 0x4xnn
	// Skips an instruction if Vx != nn.
    if (m_v[m_opc.x] != m_opc.kk) m_pc += 2;
}

void Chip::op_seq() {
	// 0x5xy0
	// Skips an instruction if Vx == Vy.
    if (m_v[m_opc.x] == m_v[m_opc.y]) m_pc += 2;
}

void Chip::op_ld() {
	// 0x6xnn
	// Sets Vx = nn.
    m_v[m_opc.x] = m_opc.kk;
}

void Chip::op_add_imm() {
	// 0x7xnn
	// Sets Vx += nn.
    m_v[m_opc.x] += m_opc.kk;
}

void Chip::op_mov() {
	// 0x8xy0
	// Sets Vx = Vy.
    m_v[m_opc.x] = m_v[m_opc.y];
}

void Chip::op_or() {
	// 0x8xy1
    // Sets Vx |= Vy.
    // Quirks: VF is reset for Chip8.
    m_v[m_opc.x] |= m_v[m_opc.y];
//    m_v[0xf] = 0;
}

void Chip::op_and() {
	// 0x8xy2
    // Sets Vx &= Vy.
    // Quirks: VF is reset for Chip8.
    m_v[m_opc.x] &= m_v[m_opc.y];
//    m_v[0xf] = 0;
}

void Chip::op_xor() {
	// 0x8xy3
    // Sets Vx ^= Vy.
    // Quirks: VF is reset for Chip8.
    m_v[m_opc.x] ^= m_v[m_opc.y];
//    m_v[0xf] = 0;
}

void Chip::op_add() {
	// 0x8xy4
	// Sets Vx += Vy. VF is set if carry.
    m_v[m_opc.x] += m_v[m_opc.y];
    m_v[0xf] = (m_v[m_opc.x] < m_v[m_opc.y]) ? 1 : 0;
}

void Chip::op_sub() {
	// 0x8xy5
	// Sets Vx -= Vy. VF is set if no borrow.
    GPReg borrow = (m_v[m_opc.x] < m_v[m_opc.y]) ? 0 : 1;
    m_v[m_opc.x] -= m_v[m_opc.y];
    m_v[0xf] = borrow;
}

void Chip::op_shr() {
	// 0x8xy6
	// Sets Vx >>= 1. VF is set to the least significant bit of Vx.
    // FIXME: quirks
    GPReg lsb = m_v[m_opc.x] & 0x1;
    m_v[m_opc.x] >>= 1;
    m_v[0xf] = lsb;
}

void Chip::op_sbr() {
	// 0x8xy7
	// Sets Vx = Vy - Vx. VF is set if no borrow.
    GPReg borrow = (m_v[m_opc.x] > m_v[m_opc.y]) ? 0 : 1;
    m_v[m_opc.x] = m_v[m_opc.y] - m_v[m_opc.x];
    m_v[0xf] = borrow;
}

void Chip::op_shl() {
	// 0x8xyE
	// Sets Vx <<= 1. VF is set to the most significant bit  of Vx.
    // FIXME: quirks
    GPReg msb = m_v[m_opc.x] >> 7;
    m_v[m_opc.x] <<= 1;
    m_v[0xf] = msb;
}

void Chip::op_sne() {
	// 0x9xy0
	// Skips an instruction if Vx != Vy.
    if (m_v[m_opc.x] != m_v[m_opc.y]) m_pc += 2;
}

void Chip::op_ldi() {
	// 0xAnnn
	// Sets I to the address nnn.
    m_i = m_opc.nnn;
}

void Chip::op_jmpr() {
	// 0xBnnn
    // Jumps to the address nnn + V0
    /* Quirks: CHIP48 and SCHIP interprets this instruction as 0xBxnn and
    jumps to the address xnn + Vx */
    Addr loc = m_v[m_opc.x] + m_opc.nnn;
    if (loc == m_pc - 2) stop(); // Infinite loop
    m_pc = loc;
}

void Chip::op_rand() {
	// 0xCxnn
	// Sets Vx to rand() & nn.
	std::random_device dev;
	std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> dist(0, 0xff);

    m_v[m_opc.x] = dist(rng) & m_opc.kk;
}

void Chip::op_draw() {
	// 0xDxyn
	// Draws a sprite from memory address I to the screen.
    // Each bit are interpreted as a pixel. If a pixel is flipped from 1 to 0, VF is set.
    m_v[0xf] = PPU::get_instance().draw_sprite_at(m_i, m_opc.n, m_v[m_opc.x], m_v[m_opc.y]) ? 1 : 0;
}

void Chip::op_skp() {
	// 0xEx9E
    // Skips the next instruction if the key in Vx is pressed.
    if (KeyPad::get_instance().is_pressed(m_v[m_opc.x])) m_pc += 2;
}

void Chip::op_sknp() {
	// 0xExA1
    // Skips the next instruction if the key in Vx is not pressed.
    if (!KeyPad::get_instance().is_pressed(m_v[m_opc.x])) m_pc += 2;
}

void Chip::op_get_delay() {
	// 0xFx07
	// Sets Vx = delay timer
    m_v[m_opc.x] = m_dtimer;
}

void Chip::op_get_key() {
	// 0xFx0A
    // Await a keypress and store it into Vx. Blocking operation.
    int k = KeyPad::get_instance().get_key();

    switch (m_key_state) {
    case GK_NOTHING:
        // Wait for keypress
        m_chipstate = CHIP_HALTED;
        if (k > KeyPad::NO_KEY) {
            m_v[m_opc.x] = k;
            m_key_state = GK_PRESSED;
        }
        break;

    case GK_PRESSED:
        if (k == KeyPad::NO_KEY) {
            m_key_state = GK_NOTHING;
            m_chipstate = CHIP_RUNNING;
        }
        break;
    }
}

void Chip::op_set_delay() {
	// 0xFx15
	// Sets delay timer to Vx.
    m_dtimer = m_v[m_opc.x];
}

void Chip::op_set_stimer() {
	// 0xFx18
	// Sets sound timer to Vx.
    m_stimer = m_v[m_opc.x];
}

void Chip::op_addi() {
	// 0xFx1E
	// Adds Vx to I. VF is set to 1 if carry.
    m_i += m_v[m_opc.x];
    m_v[0xf] = (m_i > 0xfff) ? 1 : 0;
    m_i %= 0x1000;
}

void Chip::op_ld_sprite() {
	// 0xFx29
	// Sets I to the address of the 5-byte sprite of the hex character in Vx.
    m_i = m_v[m_opc.x] * 5;
}

void Chip::op_ld_esprite() {
	// 0xFx30
	// Sets I to the address of the 10-byte sprite of the character in Vx.
    m_i = (m_v[m_opc.x] * 10) + 0x50;
}

void Chip::op_set_bcd() {
	// 0xFx33
	// Stores the BCD representation of Vx into memory starting at
	// address I.
    m_bus.write(m_i, m_v[m_opc.x] / 100);
    m_bus.write(m_i + 1, (m_v[m_opc.x] % 100) / 10);
    m_bus.write(m_i + 2, m_v[m_opc.x] % 10);
}

void Chip::op_reg_dump() {
	// 0xFx55
	// Stores V0..Vx into memory starting at address I.
    // FIXME: quirks
    for (int i = 0; i <= m_opc.x; i++)
        m_bus.write(m_i + i, m_v[i]);
}

void Chip::op_reg_store() {
	// 0xFx65
	// Fills V0..Vx from memory starting at address I.
	// TODO: quirks
    for (int i = 0; i <= m_opc.x; i++)
        m_v[i] = m_bus.read(m_i + i);
}

void Chip::op_reg_dump_rpl() {
	// 0xFx75
	// Stores V0..Vx into RPL user flags (x < 8).
    if (m_opc.x >= m_rpl.size())
		throw std::out_of_range("Cannot dump to RPL; x is out of range");

    for (int i = 0; i <= m_opc.x; i++)
        m_rpl[i] = m_v[i];
}

void Chip::op_reg_store_rpl() {
	// 0xFx85
	// Fills V0..Vx from RPL user flags (x < 8).
    if (m_opc.x >= m_rpl.size())
		throw std::out_of_range("Cannot load from RPL; x is out of range");

    for (int i = 0; i <= m_opc.x; i++)
        m_v[i] = m_rpl[i];
}
