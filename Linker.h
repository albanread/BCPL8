#ifndef LINKER_H
#define LINKER_H

#include <vector>
#include <cstdint>
#include <string>
#include "InstructionStream.h"
#include "LabelManager.h"
#include "RuntimeManager.h"
#include "Encoder.h" // For Instruction and RelocationType

class Linker {
public:
    Linker();

    // Processes the instruction stream, resolving labels and applying relocations.
    std::vector<Instruction> process(
        const InstructionStream& stream,
        LabelManager& manager,
        const RuntimeManager& runtime_manager,
        size_t code_base_address,
        void* rodata_base,
        void* data_base,
        bool enable_tracing = false // Flag to control debug output
    );

private:
    // --- Pass 1 Helper ---
    // Assigns addresses to all instructions and resolves label locations.
    std::vector<Instruction> assignAddressesAndResolveLabels(
        const InstructionStream& stream,
        LabelManager& manager,
        size_t code_base_address,
        void* data_base,
        bool enable_tracing
    );

    // --- Pass 2 Helper ---
    // Iterates through instructions to patch relocations.
    void performRelocations(
        std::vector<Instruction>& instructions,
        const LabelManager& manager,
        const RuntimeManager& runtime_manager,
        bool enable_tracing
    );

    // Applies a PC-relative relocation to an instruction encoding.
    uint32_t apply_pc_relative_relocation(
        uint32_t instruction_encoding,
        size_t instruction_address,
        size_t target_address,
        RelocationType type,
        bool enable_tracing
    );

    // Applies a MOVZ/MOVK relocation to an instruction encoding.
    uint32_t apply_movz_movk_relocation(
        uint32_t instruction_encoding,
        size_t target_address,
        RelocationType type
    );
};

#endif // LINKER_H