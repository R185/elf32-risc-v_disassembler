#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>

inline constexpr size_t kTwoByteField     = 2;
inline constexpr size_t kFourByteField    = 4;
inline constexpr size_t kSixteenByteField = 16;

inline constexpr uint8_t kElfMagic[4]     = {0x7F, 'E', 'L', 'F'};
inline constexpr uint8_t kMachineRiscV[2] = {0xF3, 0x00};

inline constexpr size_t  kClassOffset  = 4;
inline constexpr size_t  kEndianOffset = 5;
inline constexpr uint8_t kElfClass32   = 1;
inline constexpr uint8_t kElfDataLsb   = 1;

inline constexpr char kTextSection[]   = ".text";
inline constexpr char kSymtabSection[] = ".symtab";

inline constexpr size_t kElfHeaderSize       = 52;
inline constexpr size_t kMachineTypeOffset   = 18;
inline constexpr size_t kSectionHeaderOffset = 32;
inline constexpr size_t kSectionNumOffset    = 48;
inline constexpr size_t kHeaderStringsOffset = 50;

inline constexpr size_t kSectionHeaderEntSize = 40;
inline constexpr size_t kShNameOffset         = 0;
inline constexpr size_t kShAddrOffset         = 12;
inline constexpr size_t kShOffsetOffset       = 16;
inline constexpr size_t kShSizeOffset         = 20;
inline constexpr size_t kShLinkOffset         = 24;

inline constexpr size_t kSymbolEntSize   = 16;
inline constexpr size_t kSymNameOffset   = 0;
inline constexpr size_t kSymValueOffset  = 4;
inline constexpr size_t kSymSizeOffset   = 8;
inline constexpr size_t kSymInfoOffset   = 12;
inline constexpr size_t kSymOtherOffset  = 13;
inline constexpr size_t kSymShndxOffset  = 14;

inline constexpr uint16_t kShnUndef  = 0;
inline constexpr uint16_t kShnAbs    = 0xFFF1;
inline constexpr uint16_t kShnCommon = 0xFFF2;

inline constexpr uint8_t kSttNotype = 0;
inline constexpr uint8_t kSttFunc   = 2;

struct Symbol {
    int         index;
    uint32_t    value;
    uint32_t    size;
    uint8_t     type;
    uint8_t     bind;
    uint8_t     vis;
    uint16_t    shndx;
    std::string name;
};

struct DecodedInstruction {
    uint32_t    address;
    uint32_t    raw;
    std::string mnemonic;
    std::string operands;
    bool        has_target;
    uint32_t    target;
};

const char* const kRegNames[32] = {
    "zero", "ra", "sp",  "gp",  "tp", "t0", "t1", "t2",
    "s0",   "s1", "a0",  "a1",  "a2", "a3", "a4", "a5",
    "a6",   "a7", "s2",  "s3",  "s4", "s5", "s6", "s7",
    "s8",   "s9", "s10", "s11", "t3", "t4", "t5", "t6"
};

const char* Reg(uint32_t index) {
    return kRegNames[index & 0x1F];
}

int32_t SignExtend(uint32_t value, int bits) {
    uint32_t sign_bit = 1u << (bits - 1);
    if (value & sign_bit) value |= ~((1u << bits) - 1);
    return static_cast<int32_t>(value);
}

bool SliceCheckChar(const uint8_t* buf, size_t from, const uint8_t* pat, size_t n) {
    return std::memcmp(buf + from, pat, n) == 0;
}

std::string TypeToStr(uint8_t type) {
    switch (type) {
        case 0: return "NOTYPE";
        case 1: return "OBJECT";
        case 2: return "FUNC";
        case 3: return "SECTION";
        case 4: return "FILE";
        default: return "UNKNOWN";
    }
}

std::string BindToStr(uint8_t bind) {
    switch (bind) {
        case 0: return "LOCAL";
        case 1: return "GLOBAL";
        case 2: return "WEAK";
        default: return "UNKNOWN";
    }
}

std::string VisToStr(uint8_t vis) {
    switch (vis) {
        case 0: return "DEFAULT";
        case 1: return "INTERNAL";
        case 2: return "HIDDEN";
        case 3: return "PROTECTED";
        default: return "UNKNOWN";
    }
}

std::string ShndxToStr(uint16_t shndx) {
    if (shndx == kShnUndef)  return "UNDEF";
    if (shndx == kShnAbs)    return "ABS";
    if (shndx == kShnCommon) return "COMMON";
    return std::to_string(shndx);
}
