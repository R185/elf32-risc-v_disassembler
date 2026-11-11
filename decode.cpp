#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <map>
#include <ostream>
#include <string>
#include <vector>

namespace {

constexpr uint32_t kOpcodeLui    = 0x37;
constexpr uint32_t kOpcodeAuipc  = 0x17;
constexpr uint32_t kOpcodeJal    = 0x6F;
constexpr uint32_t kOpcodeJalr   = 0x67;
constexpr uint32_t kOpcodeBranch = 0x63;
constexpr uint32_t kOpcodeLoad   = 0x03;
constexpr uint32_t kOpcodeStore  = 0x23;
constexpr uint32_t kOpcodeImm    = 0x13;
constexpr uint32_t kOpcodeReg    = 0x33;
constexpr uint32_t kOpcodeFence  = 0x0F;
constexpr uint32_t kOpcodeSystem = 0x73;

uint32_t Rd(uint32_t raw)     { return (raw >> 7) & 0x1F; }
uint32_t Rs1(uint32_t raw)    { return (raw >> 15) & 0x1F; }
uint32_t Rs2(uint32_t raw)    { return (raw >> 20) & 0x1F; }
uint32_t Funct3(uint32_t raw) { return (raw >> 12) & 0x07; }
uint32_t Funct7(uint32_t raw) { return (raw >> 25) & 0x7F; }

int32_t ImmI(uint32_t raw) {
    return SignExtend((raw >> 20) & 0xFFF, 12);
}

int32_t ImmS(uint32_t raw) {
    return SignExtend((((raw >> 25) & 0x7F) << 5) | ((raw >> 7) & 0x1F), 12);
}

int32_t ImmB(uint32_t raw) {
    return SignExtend(
        (((raw >> 31) & 1) << 12) | (((raw >> 7) & 1) << 11) |
        (((raw >> 25) & 0x3F) << 5) | (((raw >> 8) & 0xF) << 1), 13);
}

int32_t ImmU(uint32_t raw) {
    return static_cast<int32_t>(raw & 0xFFFFF000);
}

int32_t ImmJ(uint32_t raw) {
    return SignExtend(
        (((raw >> 31) & 1) << 20) | (((raw >> 12) & 0xFF) << 12) |
        (((raw >> 20) & 1) << 11) | (((raw >> 21) & 0x3FF) << 1), 21);
}

}

DecodedInstruction Decode(uint32_t raw, uint32_t addr) {
    DecodedInstruction d;
    d.address    = addr;
    d.raw        = raw;
    d.has_target = false;
    d.target     = 0;

    const uint32_t rd     = Rd(raw);
    const uint32_t rs1    = Rs1(raw);
    const uint32_t rs2    = Rs2(raw);
    const uint32_t funct3 = Funct3(raw);
    const uint32_t funct7 = Funct7(raw);

    const int32_t imm_i = ImmI(raw);
    const int32_t imm_s = ImmS(raw);
    const int32_t imm_b = ImmB(raw);
    const int32_t imm_u = ImmU(raw);
    const int32_t imm_j = ImmJ(raw);

    switch (raw & 0x7F) {
        case kOpcodeLui: {
            d.mnemonic = "lui";
            d.operands = std::string(Reg(rd)) + ", " + std::to_string(imm_u >> 12);
            break;
        }
        case kOpcodeAuipc: {
            d.mnemonic = "auipc";
            d.operands = std::string(Reg(rd)) + ", " + std::to_string(imm_u >> 12);
            break;
        }
        case kOpcodeJal: {
            d.mnemonic   = "jal";
            d.operands   = std::string(Reg(rd)) + ", ";
            d.has_target = true;
            d.target     = addr + static_cast<uint32_t>(imm_j);
            break;
        }
        case kOpcodeJalr: {
            if (funct3 != 0) {
                d.mnemonic = "unknown_instruction";
                break;
            }
            d.mnemonic = "jalr";
            d.operands = std::string(Reg(rd)) + ", " + std::to_string(imm_i) + "(" + Reg(rs1) + ")";
            break;
        }
        case kOpcodeBranch: {
            const char* m = nullptr;
            switch (funct3) {
                case 0: m = "beq";  break;
                case 1: m = "bne";  break;
                case 4: m = "blt";  break;
                case 5: m = "bge";  break;
                case 6: m = "bltu"; break;
                case 7: m = "bgeu"; break;
            }
            if (!m) {
                d.mnemonic = "unknown_instruction";
                break;
            }
            d.mnemonic   = m;
            d.operands   = std::string(Reg(rs1)) + ", " + Reg(rs2) + ", ";
            d.has_target = true;
            d.target     = addr + static_cast<uint32_t>(imm_b);
            break;
        }
        case kOpcodeLoad: {
            const char* m = nullptr;
            switch (funct3) {
                case 0: m = "lb";  break;
                case 1: m = "lh";  break;
                case 2: m = "lw";  break;
                case 4: m = "lbu"; break;
                case 5: m = "lhu"; break;
            }
            if (!m) {
                d.mnemonic = "unknown_instruction";
                break;
            }
            d.mnemonic = m;
            d.operands = std::string(Reg(rd)) + ", " + std::to_string(imm_i) + "(" + Reg(rs1) + ")";
            break;
        }
        case kOpcodeStore: {
            const char* m = nullptr;
            switch (funct3) {
                case 0: m = "sb"; break;
                case 1: m = "sh"; break;
                case 2: m = "sw"; break;
            }
            if (!m) {
                d.mnemonic = "unknown_instruction";
                break;
            }
            d.mnemonic = m;
            d.operands = std::string(Reg(rs2)) + ", " + std::to_string(imm_s) + "(" + Reg(rs1) + ")";
            break;
        }
        case kOpcodeImm: {
            switch (funct3) {
                case 0: {
                    d.mnemonic = "addi";
                    d.operands = std::string(Reg(rd)) + ", " + Reg(rs1) + ", " + std::to_string(imm_i);
                    break;
                }
                case 2: {
                    d.mnemonic = "slti";
                    d.operands = std::string(Reg(rd)) + ", " + Reg(rs1) + ", " + std::to_string(imm_i);
                    break;
                }
                case 3: {
                    d.mnemonic = "sltiu";
                    d.operands = std::string(Reg(rd)) + ", " + Reg(rs1) + ", " + std::to_string(imm_i);
                    break;
                }
                case 4: {
                    d.mnemonic = "xori";
                    d.operands = std::string(Reg(rd)) + ", " + Reg(rs1) + ", " + std::to_string(imm_i);
                    break;
                }
                case 6: {
                    d.mnemonic = "ori";
                    d.operands = std::string(Reg(rd)) + ", " + Reg(rs1) + ", " + std::to_string(imm_i);
                    break;
                }
                case 7: {
                    d.mnemonic = "andi";
                    d.operands = std::string(Reg(rd)) + ", " + Reg(rs1) + ", " + std::to_string(imm_i);
                    break;
                }
                case 1: {
                    d.mnemonic = "slli";
                    d.operands = std::string(Reg(rd)) + ", " + Reg(rs1) + ", " + std::to_string((raw >> 20) & 0x1F);
                    break;
                }
                case 5: {
                    if (funct7 == 0x00)      d.mnemonic = "srli";
                    else if (funct7 == 0x20) d.mnemonic = "srai";
                    else                     d.mnemonic = "unknown_instruction";
                    d.operands = std::string(Reg(rd)) + ", " + Reg(rs1) + ", " + std::to_string((raw >> 20) & 0x1F);
                    break;
                }
                default:
                    d.mnemonic = "unknown_instruction";
            }
            break;
        }
        case kOpcodeReg: {
            const char* m = nullptr;
            if (funct7 == 0x00) {
                switch (funct3) {
                    case 0: m = "add";  break;
                    case 1: m = "sll";  break;
                    case 2: m = "slt";  break;
                    case 3: m = "sltu"; break;
                    case 4: m = "xor";  break;
                    case 5: m = "srl";  break;
                    case 6: m = "or";   break;
                    case 7: m = "and";  break;
                }
            } else if (funct7 == 0x20) {
                switch (funct3) {
                    case 0: m = "sub"; break;
                    case 5: m = "sra"; break;
                }
            } else if (funct7 == 0x01) {
                switch (funct3) {
                    case 0: m = "mul";    break;
                    case 1: m = "mulh";   break;
                    case 2: m = "mulhsu"; break;
                    case 3: m = "mulhu";  break;
                    case 4: m = "div";    break;
                    case 5: m = "divu";   break;
                    case 6: m = "rem";    break;
                    case 7: m = "remu";   break;
                }
            }
            if (!m) {
                d.mnemonic = "unknown_instruction";
                break;
            }
            d.mnemonic = m;
            d.operands = std::string(Reg(rd)) + ", " + Reg(rs1) + ", " + Reg(rs2);
            break;
        }
        case kOpcodeFence: {
            d.mnemonic = "fence";
            break;
        }
        case kOpcodeSystem: {
            uint32_t funct12 = (raw >> 20) & 0xFFF;
            if (funct12 == 0)      d.mnemonic = "ecall";
            else if (funct12 == 1) d.mnemonic = "ebreak";
            break;
        }
        default:
            break;
    }

    if (d.mnemonic.empty())
        d.mnemonic = "unknown_instruction";

    return d;
}

void PrintText(const std::vector<DecodedInstruction>& decoded,
               const std::map<uint32_t, std::string>& labels,
               std::ostream& out) {
    out << ".text\n";

    for (const auto& d : decoded) {
        auto it = labels.find(d.address);
        if (it != labels.end()) {
            char buf[64];
            snprintf(buf, sizeof(buf), "%08x   <%s>:\n", d.address, it->second.c_str());
            out << buf;
        }

        std::string operands = d.operands;
        if (d.has_target) {
            auto tit = labels.find(d.target);
            char tbuf[96];
            if (tit != labels.end())
                snprintf(tbuf, sizeof(tbuf), "%s ; 0x%x", tit->second.c_str(), d.target);
            else
                snprintf(tbuf, sizeof(tbuf), "; 0x%x", d.target);
            operands += tbuf;
        }

        char buf[256];
        snprintf(buf, sizeof(buf), "   %05x:\t%08x\t%7s\t%s\n",
                 d.address, d.raw, d.mnemonic.c_str(), operands.c_str());
        out << buf;
    }

    out << "\n";
}
