#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <istream>
#include <map>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

uint16_t ReadU16(const uint8_t* p) {
    return static_cast<uint16_t>(p[0] | (static_cast<uint16_t>(p[1]) << 8));
}

uint32_t ReadU32(const uint8_t* p) {
    return static_cast<uint32_t>(p[0])
         | (static_cast<uint32_t>(p[1]) << 8)
         | (static_cast<uint32_t>(p[2]) << 16)
         | (static_cast<uint32_t>(p[3]) << 24);
}

bool ReadSectionHeader(std::istream& file, size_t sh_off, size_t index, uint8_t* section) {
    file.clear();
    file.seekg(static_cast<std::streamoff>(sh_off + index * kSectionHeaderEntSize));
    return static_cast<bool>(file.read(reinterpret_cast<char*>(section), kSectionHeaderEntSize));
}

bool FindSection(std::istream& file, size_t sh_off, size_t sections_num,
                 const std::vector<uint8_t>& shstrtab, const char* wanted,
                 uint8_t* section) {
    for (size_t i = 0; i < sections_num; ++i) {
        if (!ReadSectionHeader(file, sh_off, i, section)) return false;

        size_t name_offset = ReadU32(section + kShNameOffset);
        if (name_offset >= shstrtab.size()) continue;

        const char* base = reinterpret_cast<const char*>(shstrtab.data() + name_offset);
        size_t len = strnlen(base, shstrtab.size() - name_offset);
        if (std::string_view(base, len) == wanted) return true;
    }
    return false;
}

}

bool CheckElfFile(std::istream& file, size_t& sh_off, size_t& sh_strtab_index, size_t& sections_num) {
    file.clear();
    file.seekg(0);

    uint8_t header[kElfHeaderSize];
    if (!file.read(reinterpret_cast<char*>(header), kElfHeaderSize)) return false;

    if (!SliceCheckChar(header, 0, kElfMagic, 4)) return false;
    if (header[kClassOffset] != kElfClass32) return false;
    if (header[kEndianOffset] != kElfDataLsb) return false;
    if (!SliceCheckChar(header, kMachineTypeOffset, kMachineRiscV, 2)) return false;

    sh_off          = ReadU32(header + kSectionHeaderOffset);
    sections_num    = ReadU16(header + kSectionNumOffset);
    sh_strtab_index = ReadU16(header + kHeaderStringsOffset);

    return true;
}

bool ReadStrings(std::istream& file, std::vector<uint8_t>& strtab, size_t section_index, size_t sh_off) {
    uint8_t section[kSectionHeaderEntSize];
    if (!ReadSectionHeader(file, sh_off, section_index, section)) return false;

    size_t str_offset = ReadU32(section + kShOffsetOffset);
    size_t str_size   = ReadU32(section + kShSizeOffset);
    if (str_size == 0) return false;

    file.clear();
    file.seekg(static_cast<std::streamoff>(str_offset));

    strtab.resize(str_size);
    if (!file.read(reinterpret_cast<char*>(strtab.data()), static_cast<std::streamsize>(str_size))) return false;

    return true;
}

bool InstructionsReader(std::istream& file, size_t sh_off, size_t sections_num,
                        std::vector<uint32_t>& commands, const std::vector<uint8_t>& shstrtab,
                        size_t& text_address) {
    uint8_t section[kSectionHeaderEntSize];
    if (!FindSection(file, sh_off, sections_num, shstrtab, kTextSection, section)) return false;

    size_t text_offset = ReadU32(section + kShOffsetOffset);
    size_t text_bytes  = ReadU32(section + kShSizeOffset);
    text_address       = ReadU32(section + kShAddrOffset);

    if (text_bytes == 0 || text_bytes % kFourByteField != 0) return false;

    file.clear();
    file.seekg(static_cast<std::streamoff>(text_offset));

    commands.clear();
    commands.reserve(text_bytes / kFourByteField);

    for (size_t i = 0; i < text_bytes; i += kFourByteField) {
        uint8_t bytes[kFourByteField];
        if (!file.read(reinterpret_cast<char*>(bytes), kFourByteField)) return false;
        commands.push_back(ReadU32(bytes));
    }

    return commands.size() * kFourByteField == text_bytes;
}

bool SymtabReader(std::istream& file, size_t sh_off, size_t sections_num,
                  std::vector<Symbol>& symbols, const std::vector<uint8_t>& shstrtab) {
    uint8_t section[kSectionHeaderEntSize];
    if (!FindSection(file, sh_off, sections_num, shstrtab, kSymtabSection, section)) return false;

    size_t sym_offset  = ReadU32(section + kShOffsetOffset);
    size_t sym_bytes   = ReadU32(section + kShSizeOffset);
    size_t sym_strtab  = ReadU32(section + kShLinkOffset);

    if (sym_bytes == 0 || sym_bytes % kSixteenByteField != 0) return false;

    std::vector<uint8_t> strtab;
    if (!ReadStrings(file, strtab, sym_strtab, sh_off)) return false;

    size_t sym_count = sym_bytes / kSixteenByteField;
    symbols.clear();
    symbols.reserve(sym_count);

    for (size_t i = 0; i < sym_count; ++i) {
        file.clear();
        file.seekg(static_cast<std::streamoff>(sym_offset + i * kSymbolEntSize));

        uint8_t entry[kSymbolEntSize];
        if (!file.read(reinterpret_cast<char*>(entry), kSymbolEntSize)) return false;

        uint32_t st_name  = ReadU32(entry + kSymNameOffset);
        uint32_t st_value = ReadU32(entry + kSymValueOffset);
        uint32_t st_size  = ReadU32(entry + kSymSizeOffset);
        uint8_t  st_info  = entry[kSymInfoOffset];
        uint8_t  st_other = entry[kSymOtherOffset];
        uint16_t st_shndx = ReadU16(entry + kSymShndxOffset);

        std::string name;
        if (st_name != 0 && st_name < strtab.size()) {
            const char* base = reinterpret_cast<const char*>(strtab.data() + st_name);
            size_t len = strnlen(base, strtab.size() - st_name);
            name.assign(base, len);
        }

        Symbol s;
        s.index = static_cast<int>(i);
        s.value = st_value;
        s.size  = st_size;
        s.type  = st_info & 0x0F;
        s.bind  = st_info >> 4;
        s.vis   = st_other & 0x03;
        s.shndx = st_shndx;
        s.name  = std::move(name);
        symbols.push_back(std::move(s));
    }

    return symbols.size() * kSymbolEntSize == sym_bytes;
}

std::map<uint32_t, std::string> BuildLabelMap(const std::vector<Symbol>& symbols,
                                             uint32_t text_addr,
                                             uint32_t text_size) {
    std::map<uint32_t, std::string> labels;
    uint32_t text_end = text_addr + text_size;

    for (const auto& s : symbols) {
        if (s.shndx == kShnUndef) continue;
        if (s.shndx == kShnAbs) continue;
        if (s.shndx == kShnCommon) continue;
        if (s.type != kSttFunc && s.type != kSttNotype) continue;
        if (s.value < text_addr || s.value >= text_end) continue;
        if (s.name.empty()) continue;
        if (s.name[0] == '$') continue;
        if (labels.count(s.value)) continue;

        labels[s.value] = s.name;
    }

    return labels;
}

void PrintSymtab(const std::vector<Symbol>& symbols, std::ostream& out) {
    out << "Symbol Value          \tSize Type \tBind \tVis   \tIndex Name\n";

    for (const auto& s : symbols) {
        std::string type_str = TypeToStr(s.type);
        std::string bind_str = BindToStr(s.bind);
        std::string vis_str  = VisToStr(s.vis);
        std::string idx_str  = ShndxToStr(s.shndx);

        char buf[512];
        snprintf(buf, sizeof(buf),
                 "[%4i] 0x%-15X %5i %-8s %-8s %-8s %6s %s\n",
                 s.index,
                 s.value,
                 s.size,
                 type_str.c_str(),
                 bind_str.c_str(),
                 vis_str.c_str(),
                 idx_str.c_str(),
                 s.name.c_str());
        out << buf;
    }
}
