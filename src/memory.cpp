#include <cassert>
#include <array>
#include <fstream>

#include <schip/memory.h>

constexpr Addr USERCODE_BEG = 0x200;
constexpr Addr USERCODE_END = 0x1000;
constexpr size_t USERCODE_SIZE = USERCODE_END - USERCODE_BEG;

// 4*5 pixel hex font patterns
constexpr std::array<Byte, 0x50> HEX_FONT = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

// 8*10 pixel font patterns(decimal digits only)
constexpr std::array<Byte, 0x64> BIG_FONT = {
    0x3c, 0x7e, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0x7e, 0x3c, // 0
    0x18, 0x38, 0x58, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3c, // 1
    0x3e, 0x7f, 0xc3, 0x06, 0x0c, 0x18, 0x30, 0x60, 0xff, 0xff, // 2
    0x3c, 0x7e, 0xc3, 0x03, 0x0e, 0x0e, 0x03, 0xc3, 0x7e, 0x3c, // 3
    0x06, 0x0e, 0x1e, 0x36, 0x66, 0xc6, 0xff, 0xff, 0x06, 0x06, // 4
    0xff, 0xff, 0xc0, 0xc0, 0xfc, 0xfe, 0x03, 0xc3, 0x7e, 0x3c, // 5
    0x3e, 0x7c, 0xc0, 0xc0, 0xfc, 0xfe, 0xc3, 0xc3, 0x7e, 0x3c, // 6
    0xff, 0xff, 0x03, 0x06, 0x0c, 0x18, 0x30, 0x60, 0x60, 0x60, // 7
    0x3c, 0x7e, 0xc3, 0xc3, 0x7e, 0x7e, 0xc3, 0xc3, 0x7e, 0x3c, // 8
    0x3c, 0x7e, 0xc3, 0xc3, 0x7f, 0x3f, 0x03, 0x03, 0x3e, 0x7c  // 9
};

Bus::Bus()
    : m_data(reinterpret_cast<char*>(calloc(USERCODE_SIZE, sizeof(Byte))))
{}

Bus::~Bus() { if (m_data) delete m_data; }

Byte Bus::read(Addr addr) const {
    if (addr >= USERCODE_END) {
#if(DEBUG)
        fprintf(stderr, "Tried to read from address [0x%04x]\n", addr);
#endif
        throw std::out_of_range("Bus read: Address is out of range");
    }
    
    if (addr < HEX_FONT.size()) {
        return HEX_FONT[addr];
    } else if (addr < HEX_FONT.size() + BIG_FONT.size()) {
        return BIG_FONT[addr - HEX_FONT.size()];
    } else if (addr < USERCODE_BEG) {
#if(DEBUG)
        printf("Warning: Reading garbage from address [0x%04x]\n", addr);
#endif
        // This space contains the code for the chip8 interpreter irl.
        return 0xcc; // Return "garbage". 0xcc is easy to spot.
    }

    return m_data[addr - USERCODE_BEG];
}

void Bus::write(Addr addr, Byte byte) {
    if (addr < USERCODE_BEG || addr >= USERCODE_END) {
#if(DEBUG)
        printf("Tried to write to address [0x%04x]\n", addr);
#endif
        throw std::out_of_range("Bus write: Address is out of range");
    }

    m_data[addr - USERCODE_BEG] = byte;
}

void Bus::load_program(std::filesystem::path filename) {
    std::ifstream file{
        filename,
        std::ios_base::in | std::ios::binary
    };

    if (!file.is_open())
        throw std::runtime_error("File not found");

    if (!file)
        throw std::runtime_error("Cannot open file");

    file.seekg(0, std::ios::end);
    size_t filesize = file.tellg();
    file.seekg(0, std::ios::beg);

#if(DEBUG)
    printf("The file is %lld bytes.\n", filesize);
#endif

    if (!filesize)
        throw std::runtime_error("The file is empty");

    if (filesize > USERCODE_SIZE)
        throw std::invalid_argument("The file is too big to be a chip8/schip program");

    file.read(m_data, USERCODE_SIZE);
    size_t read = file.gcount();

#if(DEBUG)
    printf("Read %lld bytes.\n", read);
#endif

    assert(read <= USERCODE_SIZE);

    if (read == 0)
        throw std::runtime_error("Couldn't read anything from the file...");

    file.close();
}
