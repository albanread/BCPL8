#include "Encoder.h"
#include <string>

std::vector<Instruction> Encoder::create_movz_movk_jit_addr(const std::string& xd, uint64_t address, const std::string& symbol) {
    std::vector<Instruction> instructions;

    // 1. Split the 64-bit address into four 16-bit chunks.
    uint16_t chunk0 = (address >> 0)  & 0xFFFF;
    uint16_t chunk1 = (address >> 16) & 0xFFFF;
    uint16_t chunk2 = (address >> 32) & 0xFFFF;
    uint16_t chunk3 = (address >> 48) & 0xFFFF;

    // 2. Generate MOVZ for the lowest chunk.
    instructions.push_back(Encoder::create_movz_imm(xd, chunk0, 0, RelocationType::NONE, symbol));
    instructions.back().jit_attribute = JITAttribute::JitAddress;

    // 3. Generate MOVK for the three remaining chunks, unconditionally.
    instructions.push_back(Encoder::create_movk_imm(xd, chunk1, 16, RelocationType::NONE, symbol));
    instructions.back().jit_attribute = JITAttribute::JitAddress;

    instructions.push_back(Encoder::create_movk_imm(xd, chunk2, 32, RelocationType::NONE, symbol));
    instructions.back().jit_attribute = JITAttribute::JitAddress;

    instructions.push_back(Encoder::create_movk_imm(xd, chunk3, 48, RelocationType::NONE, symbol));
    instructions.back().jit_attribute = JITAttribute::JitAddress;

    return instructions;
}
