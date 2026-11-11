#include <fstream>
#include <map>
#include <string>
#include <vector>

#include "lib/constants.cpp"
#include "lib/elf_headers.cpp"
#include "lib/decode.cpp"

int main(int argc, char** argv) {
    if (argc != 3) return 1;

    std::ifstream file(argv[1], std::ios::binary);
    if (!file) return 2;

    std::ofstream out(argv[2]);
    if (!out) return 3;

    size_t sh_off          = 0;
    size_t sh_strtab_index = 0;
    size_t sections_num    = 0;
    if (!CheckElfFile(file, sh_off, sh_strtab_index, sections_num)) return 4;

    std::vector<uint8_t> shstrtab;
    if (!ReadStrings(file, shstrtab, sh_strtab_index, sh_off)) return 5;

    std::vector<Symbol> symbols;
    if (!SymtabReader(file, sh_off, sections_num, symbols, shstrtab)) return 6;

    size_t text_address = 0;
    std::vector<uint32_t> commands;
    if (!InstructionsReader(file, sh_off, sections_num, commands, shstrtab, text_address)) return 7;

    const uint32_t text_addr = static_cast<uint32_t>(text_address);
    const uint32_t text_size = static_cast<uint32_t>(commands.size() * kFourByteField);
    const uint32_t text_end  = text_addr + text_size;

    std::map<uint32_t, std::string> labels = BuildLabelMap(symbols, text_addr, text_size);

    std::vector<DecodedInstruction> decoded;
    decoded.reserve(commands.size());
    for (size_t i = 0; i < commands.size(); ++i)
        decoded.push_back(Decode(commands[i], text_addr + static_cast<uint32_t>(i * kFourByteField)));

    unsigned next_label = 0;
    for (const auto& d : decoded) {
        if (!d.has_target) continue;
        if (d.target < text_addr || d.target >= text_end) continue;
        if (labels.count(d.target)) continue;
        labels[d.target] = "L" + std::to_string(next_label++);
    }

    PrintText(decoded, labels, out);
    PrintSymtab(symbols, out);

    return 0;
}
