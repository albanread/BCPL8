#include "Linker.h"
#include "RuntimeManager.h"
#include <stdexcept>
#include <iostream>

// Default constructor for Linker (stateless, no initialization needed)
Linker::Linker() {}

/**
 * @brief Main entry point to process the instruction stream.
 *
 * This function orchestrates the two main passes of the linking process:
 * 1. Address Assignment: Calculates the final memory address for every instruction and label.
 * 2. Relocation: Patches instructions that depend on the final addresses of labels.
 * @param stream The raw instruction stream from the code generator.
 * @param manager The label manager to store and retrieve label addresses.
 * @param runtime_manager The manager for runtime function lookups.
 * @param code_base_address The starting virtual address for the code segment.
 * @param rodata_base The starting address for read-only data (not used in this model, combined with code).
 * @param data_base The starting virtual address for the read-write data segment.
 * @param enable_tracing A flag to enable verbose logging for debugging.
 * @return A vector of finalized and relocated instructions.
 */
std::vector<Instruction> Linker::process(
    const InstructionStream& stream,
    LabelManager& manager,
    const RuntimeManager& runtime_manager,
    size_t code_base_address,
    void* rodata_base,
    void* data_base,
    bool enable_tracing
) {
    // Pass 1: Assign addresses to all instructions and define labels in the manager.
    std::vector<Instruction> instructions_with_addresses =
        assignAddressesAndResolveLabels(stream, manager, code_base_address, data_base, enable_tracing);

    // Pass 2: Apply relocations to patch instructions with the now-known label addresses.
    performRelocations(instructions_with_addresses, manager, runtime_manager, enable_tracing);

    return instructions_with_addresses;
}

/**
 * @brief (PASS 1) Assigns virtual addresses to instructions and resolves label locations.
 *
 * Iterates through the raw instruction stream, calculating the address for each
 * instruction and data element. It populates the LabelManager with the final
 * address of each label it encounters. This version distinguishes between the
 * code/rodata segment and the read-write data segment.
 *
 * @return A vector of instructions with their `address` field correctly set.
 */

 std::vector<Instruction> Linker::assignAddressesAndResolveLabels(
     const InstructionStream& stream,
     LabelManager& manager,
     size_t code_base_address,
     void* data_base, // This is for .data, which is separate
     bool enable_tracing
 ) {
     if (enable_tracing) std::cerr << "[LINKER-PASS1] Starting address and label assignment...\n";

     std::vector<Instruction> finalized_instructions;
     finalized_instructions.reserve(stream.get_instructions().size());

     // --- Correct Cursor Management ---
     size_t code_cursor = code_base_address;
     size_t data_cursor = reinterpret_cast<size_t>(data_base);
     size_t rodata_cursor = 0; // Will be calculated after code size is known

     // --- Pass 1a: Calculate the total size of the code segment to find where .rodata starts ---
     size_t code_segment_size = 0;
     for (const auto& instr : stream.get_instructions()) {
         if (instr.segment == SegmentType::CODE) {
             if (!instr.is_label_definition) {
                 code_segment_size += 4;
             }
         }
     }

     // --- Calculate the start of the .rodata segment ---
     // Start rodata after the code, plus a gap, aligned to a 4KB page boundary.
     const size_t CODE_RODATA_GAP_BYTES = 16 * 1024;
     rodata_cursor = (code_base_address + code_segment_size + CODE_RODATA_GAP_BYTES + 0xFFF) & ~0xFFF;
     if (enable_tracing) {
         std::cerr << "[LINKER-PASS1] Code segment ends near 0x" << std::hex << (code_base_address + code_segment_size) << std::dec << ".\n";
         std::cerr << "[LINKER-PASS1] Read-only data (.rodata) segment will start at 0x" << std::hex << rodata_cursor << std::dec << ".\n";
     }

     // --- Pass 1b: Assign final addresses to all instructions and define all labels ---
     for (const auto& instr : stream.get_instructions()) {
         Instruction new_instr = instr;

         switch (instr.segment) {
             case SegmentType::CODE:
                 if (instr.is_label_definition) {
                     manager.define_label(instr.target_label, code_cursor);
                     if (enable_tracing) {
                          std::cerr << "[LINKER-PASS1] Defined CODE label '" << instr.target_label
                                   << "' at 0x" << std::hex << code_cursor << std::dec << "\n";
                     }
                 } else {
                     new_instr.address = code_cursor;
                     code_cursor += 4;
                 }
                 break;

             case SegmentType::RODATA:
                 if (instr.is_label_definition) {
                     manager.define_label(instr.target_label, rodata_cursor);
                      if (enable_tracing) {
                          std::cerr << "[LINKER-PASS1] Defined RODATA label '" << instr.target_label
                                   << "' at 0x" << std::hex << rodata_cursor << std::dec << "\n";
                     }
                 } else {
                     new_instr.address = rodata_cursor;
                     rodata_cursor += 4;
                 }
                 break;

             case SegmentType::DATA:
                 if (instr.is_label_definition) {
                     manager.define_label(instr.target_label, data_cursor);
                      if (enable_tracing) {
                          std::cerr << "[LINKER-PASS1] Defined DATA label '" << instr.target_label
                                   << "' at 0x" << std::hex << data_cursor << std::dec << "\n";
                     }
                 } else {
                     new_instr.address = data_cursor;
                     data_cursor += 4;
                 }
                 break;
         }
         finalized_instructions.push_back(new_instr);
     }

     return finalized_instructions;
 }



/**
 * @brief (PASS 2) Applies relocations to instructions.
 *
 * Iterates through the address-assigned instructions and patches the machine code
 * for any that have a relocation type set. It uses the addresses resolved in Pass 1.
 */
void Linker::performRelocations(
    std::vector<Instruction>& instructions,
    const LabelManager& manager,
    const RuntimeManager& runtime_manager,
    bool enable_tracing
) {
    if (enable_tracing) std::cerr << "[LINKER-PASS2] Starting instruction relocation...\n";

    for (Instruction& instr : instructions) {
        if (instr.relocation == RelocationType::NONE) {
            continue;
        }

        if (instr.target_label.empty()) {
            if (enable_tracing) std::cerr << "[LINKER-WARNING] Instruction at 0x" << std::hex << instr.address << " has relocation type but no target label. Skipping.\n" << std::dec;
            continue;
        }

        size_t target_address;
        instr.resolved_symbol_name = instr.target_label;
        instr.relocation_applied = true;

        if (runtime_manager.is_function_registered(instr.target_label)) {
            target_address = reinterpret_cast<size_t>(runtime_manager.get_function(instr.target_label).address);
        } else if (manager.is_label_defined(instr.target_label)) {
            target_address = manager.get_label_address(instr.target_label);
        } else {
            throw std::runtime_error("Error: Undefined label '" + instr.target_label + "' encountered during linking.");
        }

        instr.resolved_target_address = target_address;

        switch (instr.relocation) {
            case RelocationType::PC_RELATIVE_26_BIT_OFFSET:
            case RelocationType::PC_RELATIVE_19_BIT_OFFSET:
            case RelocationType::PAGE_21_BIT_PC_RELATIVE:
            case RelocationType::ADD_12_BIT_UNSIGNED_OFFSET:
                instr.encoding = apply_pc_relative_relocation(instr.encoding, instr.address, target_address, instr.relocation, enable_tracing);
                break;

            case RelocationType::MOVZ_MOVK_IMM_0:
            case RelocationType::MOVZ_MOVK_IMM_16:
            case RelocationType::MOVZ_MOVK_IMM_32:
            case RelocationType::MOVZ_MOVK_IMM_48:
                instr.encoding = apply_movz_movk_relocation(instr.encoding, target_address, instr.relocation);
                break;

            default:
                // Do nothing for NONE or other unhandled types.
                break;
        }
    }
}
