#ifndef ENCODER_H
#define ENCODER_H

#include <sstream>

// Add this enum class somewhere visible to the Instruction struct,
// for example, near the top of Encoder.h.
enum class ConditionCode {
    EQ, NE, CS, CC, MI, PL, VS, VC,
    HI, LS, GE, LT, GT, LE, AL, NV, UNKNOWN
};

// JITAttribute: Enum for JIT-related instruction tagging
enum class JITAttribute {
  None,       // Default, no special JIT handling
  JitAddress, // Instruction is part of loading a JIT function's address
  JitCall,    // The final instruction that calls the JIT function (e.g., BLR)
  JitStore,   // Spilling a register specifically for a JIT call
  JitRestore  // Restoring a spilled register after a JIT call
};

#include <cstdint>
#include <string>
#include <vector>
#include "OpType.h" // Correctly include the top-level header
#include <cstdint>
#include <string>
#include <vector>

// Helper to map condition strings to their 4-bit encoding.
uint32_t get_condition_code(const std::string &cond);

// Forward declaration of LabelManager if needed for label creation
class LabelManager;

// Enum for relocation types (e.g., for PC-relative addressing)
// In Encoder.h, update the RelocationType enum
enum class RelocationType {
  NONE,
  PC_RELATIVE_19_BIT_OFFSET,  // For conditional branches (B.cond)
  PC_RELATIVE_26_BIT_OFFSET,  // For B and BL instructions
  PAGE_21_BIT_PC_RELATIVE,    // For ADRP (page-relative addressing, 21 bits)
  ADD_12_BIT_UNSIGNED_OFFSET, // For ADD (immediate, 12-bit unsigned offset,
                              // e.g., ADD Xn, Xn, #imm12)
  MOVZ_MOVK_IMM_0,            // For MOVZ/MOVK with shift 0
  MOVZ_MOVK_IMM_16,           // For MOVZ/MOVK with shift 16
  MOVZ_MOVK_IMM_32,           // For MOVZ/MOVK with shift 32
  MOVZ_MOVK_IMM_48,           // For MOVZ/MOVK with shift 48

  // --- Added for absolute 64-bit address relocations ---
  ABSOLUTE_ADDRESS_LO32,      // Lower 32 bits of a 64-bit absolute address
  ABSOLUTE_ADDRESS_HI32,      // Upper 32 bits of a 64-bit absolute address

  Jump,                       // For jump instructions
  Label                       // For label definitions
};

// Represents a single encoded instruction along with its metadata
enum class SegmentType { CODE, RODATA, DATA };

struct Instruction {
  uint32_t encoding = 0;
  std::string assembly_text;
  size_t address = 0;
  RelocationType relocation = RelocationType::NONE;

  // ** ADD THESE NEW STATIC METHODS **
  static Instruction as_label(const std::string& label_name, SegmentType segment) {
      Instruction instr;
      instr.is_label_definition = true;
      instr.target_label = label_name;
      instr.segment = segment;
      return instr;
  }

  static Instruction as_relocatable_data(const std::string& target_label, SegmentType segment) {
      Instruction instr;
      instr.is_data_value = true;
      instr.relocation = RelocationType::Label; // Use for absolute 64-bit pointers
      instr.target_label = target_label;
      instr.segment = segment;
      instr.encoding = 0; // The Linker will patch this with the final address.
      return instr;
  }
  std::string target_label;
  bool is_data_value = false;
  bool is_label_definition = false;
  bool relocation_applied = false;
  std::string resolved_symbol_name;
  size_t resolved_target_address = 0;
  SegmentType segment = SegmentType::CODE; // Default to CODE

  // --- For peephole branch and label patterns ---
  std::string branch_target;
  std::string label;

  // --- NEW SEMANTIC FIELDS ---
  InstructionDecoder::OpType opcode = InstructionDecoder::OpType::UNKNOWN; // Now fully defined
  int dest_reg = -1;
  int src_reg1 = -1;
  int src_reg2 = -1;
  int base_reg = -1;
  int ra_reg = -1; // For MADD

  // ✅ ADD THIS NEW FIELD:
  ConditionCode cond = ConditionCode::UNKNOWN; // Stores the condition for CSET, B.cond, etc.
  int64_t immediate = 0;
  bool uses_immediate = false;
  bool is_mem_op = false;
  // --- END NEW FIELDS ---

JITAttribute jit_attribute = JITAttribute::None;


  Instruction() = default;

  Instruction(uint32_t enc, std::string text) {
    encoding = enc;
    assembly_text = std::move(text);
    jit_attribute = JITAttribute::None;
  }

  Instruction(uint32_t enc, std::string text, RelocationType rel,
              const std::string &target, bool is_data,
              bool reloc_applied = false) {
    encoding = enc;
    assembly_text = std::move(text);
    relocation = rel;
    target_label = target;
    is_data_value = is_data;
    relocation_applied = reloc_applied;
    jit_attribute = JITAttribute::None;
  };

};


class Encoder {
public:
 
    

  /**

   * @brief Creates a MOV (Move Immediate) instruction. Moves an immediate value
   * into a register.
   * @param xd The destination register.
   * @param immediate The immediate value to move.
   * @return A complete Instruction object.
   */

  // --- Memory Access Instructions ---

  /**
   * @brief Creates an STP (Store Pair) instruction. Stores two 64-bit
   * registers.
   */
  static Instruction create_stp_imm(const std::string &xt1,
                                    const std::string &xt2,
                                    const std::string &xn, int immediate);

  /**
   * @brief Creates an STP (Store Pair) instruction with pre-indexing. Stores
   * two 64-bit registers and updates base.
   */
  static Instruction create_stp_pre_imm(const std::string &xt1,
                                        const std::string &xt2,
                                        const std::string &xn, int immediate);

  /**
   * @brief Creates an LDP (Load Pair) instruction. Loads two 64-bit registers.
   */
  static Instruction create_ldp_imm(const std::string &xt1,
                                    const std::string &xt2,
                                    const std::string &xn, int immediate);

  /**
   * @brief Creates an LDP (Load Pair) instruction with pre-indexing. Loads two
   * 64-bit registers and updates base.
   */
  static Instruction create_ldp_pre_imm(const std::string &xt1,
                                        const std::string &xt2,
                                        const std::string &xn, int immediate);

  /**
   * @brief Creates an STR (Store 128-bit Vector Register) instruction.
   * Stores one 128-bit vector register (Q/V).
   * Example: STR Q8, [X29, #imm]
   */
  static Instruction create_str_vec_imm(const std::string &qt,
                                        const std::string &xn, int immediate,
                                        const std::string &variable_name = "") {
    // ARM64 encoding for STR Qn, [Xn, #imm]
    // For now, just emit the assembly text and leave encoding as 0.
    std::ostringstream oss;
    oss << "STR " << qt << ", [" << xn << ", #" << immediate << "]";
    if (!variable_name.empty()) {
      oss << " ; spill " << variable_name;
    }
    Instruction instr(0, oss.str());
    instr.is_mem_op = true;
    instr.uses_immediate = true;
    instr.immediate = immediate;
    return instr;
  }

  /**
   * @brief Creates an LDR (Load 128-bit Vector Register) instruction.
   * Loads one 128-bit vector register (Q/V).
   * Example: LDR Q8, [X29, #imm]
   */
  static Instruction create_ldr_vec_imm(const std::string &qt,
                                        const std::string &xn, int immediate,
                                        const std::string &variable_name = "") {
    // ARM64 encoding for LDR Qn, [Xn, #imm]
    // For now, just emit the assembly text and leave encoding as 0.
    std::ostringstream oss;
    oss << "LDR " << qt << ", [" << xn << ", #" << immediate << "]";
    if (!variable_name.empty()) {
      oss << " ; reload " << variable_name;
    }
    Instruction instr(0, oss.str());
    instr.is_mem_op = true;
    instr.uses_immediate = true;
    instr.immediate = immediate;
    return instr;
  }

  /**
   * @brief Checks if the given immediate value can be encoded for the given ALU opcode.
   * Delegates ARM64-specific immediate encoding rules to the Encoder.
   * @param opcode The ALU operation type (e.g., ADD, SUB, AND, ORR, EOR).
   * @param immediate The immediate value to check.
   * @return true if the immediate can be encoded for the given opcode, false otherwise.
   */
  static bool canEncodeAsImmediate(InstructionDecoder::OpType opcode, int64_t immediate);

  /**
   * @brief Creates an MVN (Move with NOT) instruction, which is an alias for ORN with XZR.
   * @param xd The destination register.
   * @param xm The source register to be bitwise NOT-ed.
   * @return A complete Instruction object.
   */
  static Instruction create_mvn_reg(const std::string& xd, const std::string& xm);

  /**
   * @brief Creates an STR (Store Register) instruction. Stores one 64-bit
   * register.
   */
  static Instruction create_str_imm(const std::string &xt,
                                    const std::string &xn, int immediate,
                                    const std::string &variable_name = "");

  /**
   * @brief Creates an LDR (Load Register) instruction. Loads one 64-bit
   * register.
   */
  static Instruction create_ldr_imm(const std::string &xt,
                                    const std::string &xn, int immediate,
                                    const std::string &variable_name = "");

  /**
   * @brief Creates an LDRB (Load Register Byte) instruction. Loads one byte
   * into a 64-bit register.
   * @param xt The destination register.
   * @param xn The base address register.
   * @param immediate An unsigned 12-bit immediate byte offset [0, 4095].
   * @return A complete Instruction object.
   */
  static Instruction create_ldrb_imm(const std::string &xt,
                                     const std::string &xn, int immediate);

  /**
   * @brief Creates an STR (Store Floating-Point Register) instruction. Stores one 64-bit floating-point register.
   * @param dt The destination floating-point register (must be D register).
   * @param xn The base address register (must be X register or SP).
   * @param immediate An unsigned 12-bit immediate byte offset [0, 32760], must be a multiple of 8.
   * @return A complete Instruction object.
   */
  static Instruction create_str_fp_imm(const std::string &dt,
                                       const std::string &xn,
                                       int immediate);

  /**
   * @brief Creates an LDR (Load Register Word, 32-bit) instruction. Loads one 32-bit word into a 32-bit register.
   * @param wt The destination register (must be W register).
   * @param xn The base address register (must be X register or SP).
   * @param immediate An unsigned 12-bit immediate byte offset [0, 4095].
   * @return A complete Instruction object.
   */
  static Instruction create_ldr_word_imm(const std::string &wt,
                                         const std::string &xn, int immediate);

  /**
   * @brief Creates an STR (Store Register Word, 32-bit) instruction. Stores one 32-bit word from a W register.
   * @param wt The source register (must be W register).
   * @param xn The base address register (must be X register or SP).
   * @param immediate An unsigned 12-bit immediate byte offset [0, 4095].
   * @return A complete Instruction object.
   */
  static Instruction create_str_word_imm(const std::string& wt, const std::string& xn, int immediate);


  /**
   * @brief Creates an LDP (Load Pair) instruction with post-increment
   * addressing. Loads two 64-bit registers and increments the base register by
   * the immediate.
   * @param xt1 The first destination register.
   * @param xt2 The second destination register.
   * @param xn The base address register (will be updated).
   * @param immediate The immediate offset (must be a multiple of 16 for
   * 64-bit).
   * @return A complete Instruction object.
   */
  static Instruction create_ldp_post_imm(const std::string &xt1,
                                         const std::string &xt2,
                                         const std::string &xn, int immediate);

  /**
   * @brief Creates an LDR (Load Register) instruction with scaled register
   * offset. (Wt = [Xn + (Xm << shift)])
   * @param wt The destination 32-bit register.
   * @param xn The base address register.
   * @param xm The index register.
   * @param shift The left shift amount (0, 1, 2, 3).
   * @return A complete Instruction object.
   */
  static Instruction create_ldr_scaled_reg_64bit(const std::string &xt,
                                                 const std::string &xn,
                                                 const std::string &xm,
                                                 int shift);

  // --- Data Processing Instructions ---

  /**
   * @brief Creates an ADD instruction with a register operand. (Xd = Xn + Xm)
   * @param xd The destination register.
   * @param xn The first source register.
   * @param xm The second source register.
   * @return A complete Instruction object.
   */
  static Instruction create_add_reg(const std::string &xd,
                                    const std::string &xn,
                                    const std::string &xm);

  /**
   * @brief Creates a SUB instruction with a register operand. (Xd = Xn - Xm)
   * @param xd The destination register.
   * @param xn The first source register.
   * @param xm The second source register.
   * @return A complete Instruction object.
   */
  static Instruction create_sub_reg(const std::string &xd,
                                    const std::string &xn,
                                    const std::string &xm);

  /**
   * @brief Creates a MUL instruction. (Xd = Xn * Xm)
   * @param xd The destination register.
   * @param xn The first source register.
   * @param xm The second source register.
   * @return A complete Instruction object.
   */
  static Instruction create_mul_reg(const std::string &xd,
                                    const std::string &xn,
                                    const std::string &xm);

  /**
   * @brief Creates an ADD instruction with an immediate operand. (Xd = Xn +
   * imm)
   * @param xd The destination register.
   * @param xn The source register.
   * @param immediate An unsigned 12-bit immediate value [0, 4095].
   * @return A complete Instruction object.
   */

  /**
   * @brief Creates a SUB instruction with an immediate operand. (Xd = Xn - imm)
   * @param xd The destination register.
   * @param xn The source register.
   * @param immediate An unsigned 12-bit immediate value [0, 4095].
   * @return A complete Instruction object.
   */
  static Instruction create_add_imm(const std::string &xd,
                                    const std::string &xn, int immediate);

  /**
   * @return A complete Instruction object.
   */
  static Instruction create_sub_imm(const std::string &xd,
                                    const std::string &xn, int immediate);

  /**
   * @brief Creates an AND instruction with a register operand. (Xd = Xn & Xm)
   * @param xd The destination register.
   * @param xn The first source register.
   * @param xm The second source register.
   * @return A complete Instruction object.
   */
  static Instruction create_and_reg(const std::string &xd,
                                    const std::string &xn,
                                    const std::string &xm);

  /**
   * @brief Creates an ORR (OR) instruction with a register operand. (Xd = Xn |
   * Xm)
   * @param xd The destination register.
   * @param xn The first source register.
   * @param xm The second source register.
   * @return A complete Instruction object.
   */
  static Instruction create_orr_reg(const std::string &xd,
                                    const std::string &xn,
                                    const std::string &xm);

  /**
   * @brief Creates an EOR (XOR) instruction with a register operand. (Xd = Xn ^
   * Xm)
   * @param xd The destination register.
   * @param xn The first source register.
   * @param xm The second source register.
   * @return A complete Instruction object.
   */
  static Instruction create_eor_reg(const std::string &xd,
                                    const std::string &xn,
                                    const std::string &xm);

  /**
   * @brief Creates a MOV instruction to copy a register's value. (Xd = Xm)
   * @param xd The destination register.
   * @param xm The source register.
   * @return A complete Instruction object.
   */
  static Instruction create_mov_reg(const std::string &xd,
                                    const std::string &xs);

  // FMOV for D registers (float argument passing)
  static Instruction create_fmov_reg(const std::string &dd,
                                     const std::string &ds);

  /**
   * @brief Creates a sequence of MOVZ/MOVK instructions to load a 64-bit
   * absolute address into a register.
   * @param xd The destination register.
   * @param address The 64-bit absolute address.
   * @return A vector of Instruction objects (MOVZ + up to 3 MOVK).
   */
  static std::vector<Instruction>
  create_movz_movk_abs64(const std::string &xd, uint64_t address,
                         const std::string &symbol);

  /**
   * @brief Creates a MOVZ instruction to load a 16-bit immediate into a
   * register, zeroing the other bits.
   * @param xd The destination register.
   * @param immediate The 16-bit immediate value [0, 65535].
   * @param shift The left shift to apply to the immediate. Must be 0, 16, 32,
   * or 48.
   * @return A complete Instruction object.
   */
  /**
   * @brief Creates a MOVZ instruction to load a 16-bit immediate into a
   * register, zeroing the other bits.
   * @param xd The destination register.
   * @param immediate The 16-bit immediate value [0, 65535].
   * @param shift The left shift to apply to the immediate. Must be 0, 16, 32,
   * or 48.
   * @param rel Relocation type (for linker/JIT patching).
   * @param target The symbol name for relocation (if any).
   * @return A complete Instruction object.
   */
  static Instruction create_movz_imm(const std::string &xd, uint16_t immediate,
                                     int shift = 0,
                                     RelocationType rel = RelocationType::NONE,
                                     const std::string &target = "");

  /**
   * @brief Creates a MOVK instruction to move a 16-bit immediate into a
   * register, keeping other bits unchanged.
   * @param xd The destination register.
   * @param immediate The 16-bit immediate value [0, 65535].
   * @param shift The left shift to apply to the immediate. Must be 0, 16, 32,
   * or 48.
   * @param rel Relocation type (for linker/JIT patching).
   * @param target The symbol name for relocation (if any).
   * @return A complete Instruction object.
   */
  static Instruction create_movk_imm(const std::string &xd, uint16_t immediate,
                                     int shift = 0,
                                     RelocationType rel = RelocationType::NONE,
                                     const std::string &target = "");
    
    
    static std::vector<Instruction> create_movz_movk_jit_addr(const std::string& xd, uint64_t address, const std::string& symbol); // New function

  /**
   * @brief Creates a SDIV instruction. (Xd = Xn / Xm) (signed division)
   * @param xd The destination register.
   * @param xn The first source register.
   * @param xm The second source register.
   * @return A complete Instruction object.
   */
  static Instruction create_sdiv_reg(const std::string &xd,
                                     const std::string &xn,
                                     const std::string &xm);

  /**
   * @brief Creates a CMP (Compare) instruction. (Compares Xn and Xm, sets
   * flags)
   * @param xn The first source register.
   * @param xm The second source register.
   * @return A complete Instruction object.
   */
  static Instruction create_cmp_reg(const std::string &xn,
                                    const std::string &xm);

  // FSQRT (Floating-point Square Root) instruction for D registers
  static Instruction create_fsqrt_reg(const std::string& dd, const std::string& dn);

  /**
   * @brief Creates a CSET (Conditional Set) instruction for EQ condition. (Xd =
   * 1 if Z=1, else 0)
   * @param xd The destination register.
   * @return A complete Instruction object.
   */
  static Instruction create_cset_eq(const std::string &xd);

  /**
   * @brief Creates a CSET (Conditional Set) instruction for NE condition. (Xd =
   * 1 if Z=0, else 0)
   * @param xd The destination register.
   * @return A complete Instruction object.
   */
  static Instruction create_cset_ne(const std::string &xd);

  /**
   * @brief Creates a CSET (Conditional Set) instruction for LT condition. (Xd =
   * 1 if N!=V, else 0)
   * @param xd The destination register.
   * @return A complete Instruction object.
   */
  static Instruction create_cset_lt(const std::string &xd);

  /**
   * @brief Creates a CSET (Conditional Set) instruction for LE condition. (Xd =
   * 1 if Z=1 or N!=V, else 0)
   * @param xd The destination register.
   * @return A complete Instruction object.
   */
  static Instruction create_cset_le(const std::string &xd);

  /**
   * @brief Creates a CSET (Conditional Set) instruction for GT condition. (Xd =
   * 1 if Z=0 and N=V, else 0)
   * @param xd The destination register.
   * @return A complete Instruction object.
   */
  static Instruction create_cset_gt(const std::string &xd);

  /**
   * @brief Creates a CSET (Conditional Set) instruction for GE condition. (Xd =
   * 1 if N=V, else 0)
   * @param xd The destination register.
   * @return A complete Instruction object.
   */
  static Instruction create_cset_ge(const std::string &xd);

  /**
   * @brief Creates a CMP (Compare) instruction with an immediate value.
   * @param xn The register to compare.
   * @param immediate The immediate value to compare against.
   * @return A complete Instruction object.
   */
  static Instruction create_cmp_imm(const std::string &xn, int immediate);

  /**
   * @brief Creates an LSL (Logical Shift Left) instruction. (Xd = Xn << Xm)
   * @param xd The destination register.
   * @param xn The first source register.
   * @param xm The second source register (shift amount).
   * @return A complete Instruction object.
   */
  static Instruction create_lsl_reg(const std::string &xd,
                                    const std::string &xn,
                                    const std::string &xm);

  /**
   * @brief Creates an LSL (Logical Shift Left) instruction with an immediate
   * value. (Xd = Xn << #shift_amount)
   * @param xd The destination register.
   * @param xn The source register.
   * @param shift_amount The immediate shift amount.
   * @return A complete Instruction object.
   */
  static Instruction create_lsl_imm(const std::string &xd,
                                    const std::string &xn, int shift_amount);

  /**
   * @brief Creates an LSR (Logical Shift Right) instruction. (Xd = Xn >> Xm)
   * @param xd The destination register.
   * @param xn The first source register.
   * @param xm The second source register (shift amount).
   * @return A complete Instruction object.
   */
  static Instruction create_lsr_reg(const std::string &xd,
                                    const std::string &xn,
                                    const std::string &xm);

  /**
   * @brief Creates a BIC (Bit Clear) instruction. (Xd = Xn & ~Xm)
   * @param xd The destination register.
   * @param xn The first source register.
   * @param xm The second source register (mask).
   * @return A complete Instruction object.
   */
  static Instruction create_bic_reg(const std::string &xd,
                                    const std::string &xn,
                                    const std::string &xm);

  // In Encoder.h, inside the Encoder class definition
  // --- Control Flow Instructions ---

  /**
   * @brief Creates a B (unconditional branch) instruction to a label.
   * @param label_name The target label.
   * @return A complete Instruction object with relocation info.
   */
  static Instruction create_branch_unconditional(const std::string &label_name);

  /**
   * @brief Creates a conditional branch instruction (e.g., B.EQ) to a label.
   * @param condition The condition code as a string (e.g., "EQ", "NE", "LT",
   * "GE").
   * @param label_name The target label.
   * @return A complete Instruction object with relocation info.
   */
  static Instruction create_branch_conditional(const std::string &condition,
                                               const std::string &label_name);

  /**
   * @brief Creates a BL (Branch with Link) instruction for a function call.
   * @param label_name The target function's label.
   * @return A complete Instruction object with relocation info.
   */
  static Instruction create_branch_with_link(const std::string &label_name);

  /**
   * @brief Creates a BLR (Branch with Link to Register) for an indirect
   * function call.
   * @param xn The register holding the target address.
   * @return A complete Instruction object.
   */
  static Instruction create_branch_with_link_register(const std::string &xn);

  /**
   * @brief Creates a RET (Return) instruction.
   * @return A complete Instruction object.
   */
  static Instruction create_return();
  static Instruction create_nop();

  static Instruction create_brk(uint16_t imm = 0);

  /**
   * @brief Creates a BR (Branch to Register) instruction for an indirect
   * branch.
   * @param xn The register holding the target address.
   * @return A complete Instruction object.
   */
  static Instruction create_br_reg(const std::string &xn);

  // --- Floating-Point Instructions ---

  /**
   * @brief Creates an FADD instruction for double-precision registers. (Dd = Dn
   * + Dm)
   * @param dd The destination register (e.g., "D0").
   * @param dn The first source register.
   * @param dm The second source register.
   * @return A complete Instruction object.
   */
  static Instruction create_fadd_reg(const std::string &dd,
                                     const std::string &dn,
                                     const std::string &dm);

  /**
   * @brief Creates an FSUB instruction for double-precision registers. (Dd = Dn
   * - Dm)
   * @param dd The destination register.
   * @param dn The first source register.
   * @param dm The second source register.
   * @return A complete Instruction object.
   */
  static Instruction create_fsub_reg(const std::string &dd,
                                     const std::string &dn,
                                     const std::string &dm);

  /**
   * @brief Creates an FMUL instruction for double-precision registers. (Dd = Dn
   * * Dm)
   * @param dd The destination register.
   * @param dn The first source register.
   * @param dm The second source register.
   * @return A complete Instruction object.
   */
  static Instruction create_fmul_reg(const std::string &dd,
                                     const std::string &dn,
                                     const std::string &dm);

  /**
   * @brief Creates an FDIV instruction for double-precision registers. (Dd = Dn
   * / Dm)
   * @param dd The destination register.
   * @param dn The first source register.
   * @param dm The second source register.
   * @return A complete Instruction object.
   */
  static Instruction create_fdiv_reg(const std::string &dd,
                                     const std::string &dn,
                                     const std::string &dm);

  /**
   * @brief Creates an FCMP instruction to compare two double-precision
   * registers.
   * @param dn The first register to compare.
   * @param dm The second register to compare.
   * @return A complete Instruction object.
   */
  static Instruction create_fcmp_reg(const std::string &dn,
                                     const std::string &dm);

  /**
   * @brief Creates an SCVTF instruction to convert a 64-bit integer to
   * double-precision float.
   * @param dd The destination float register (e.g., "D0").
   * @param xn The source integer register (e.g., "X0").
   * @return A complete Instruction object.
   */
  static Instruction create_scvtf_reg(const std::string &dd,
                                      const std::string &xn);

  /**
   * @brief Creates an FCVTZS instruction to convert a double-precision float to
   * a 64-bit integer.
   * @param xd The destination integer register (e.g., "X0").
   * @param dn The source float register (e.g., "D0").
   * @return a complete Instruction object.
   */
  static Instruction create_fcvtzs_reg(const std::string &xd,
                                       const std::string &dn);

  // FNEG (Floating-point Negate) instruction for D registers
  static Instruction create_fneg_reg(const std::string& dd, const std::string& dn);


  // FCVTMS (Floating-point Convert to Signed integer, round toward Minus infinity)
  static Instruction create_fcvtms_reg(const std::string &xd,
                                       const std::string &dn);


  /**
   * @brief Creates an LDR instruction to load a double-precision register from
   * memory.
   * @param dt The destination float register.
   * @param xn The base address register.
   * @param immediate A byte offset. Must be a multiple of 8, from 0 to 32760.
   * @return A complete Instruction object.
   */
  static Instruction create_ldr_fp_imm(const std::string &dt,
                                       const std::string &xn, int immediate);



  // --- Address Loading Instructions ---
  /**
   * @brief Creates an ADRP instruction. Calculates address of a 4KB page.
   * @param xd The destination register.
   * @param label_name The target label.
   * @return A complete Instruction object with relocation info.
   */
  static Instruction create_adrp(const std::string &xd,
                                 const std::string &label_name);

  /**
   * @brief Creates an ADD instruction to add the lower 12 bits of an address.
   * @param xd The destination register.
   * @param xn The base register (from ADRP).
   * @param label_name The target label (for relocation).
   * @return A complete Instruction object with relocation info.
   */
  static Instruction create_add_literal(const std::string &xd,
                                        const std::string &xn,
                                        const std::string &label_name);

  /**
   * @brief Creates an ADR instruction. Loads the address of a label into a register.
   * @param xd The destination register.
   * @param label_name The target label.
   * @return A complete Instruction object with relocation info.
   */
  static Instruction create_adr(const std::string &xd,
                                const std::string &label_name);

  // --- NEON / SIMD Instructions ---

  /**
   * @brief Creates a vector FMLA (Fused Multiply-Add) instruction. (Vd = Va +
   * (Vn * Vm))
   * @param vd The destination and addend vector register (e.g., "V0").
   * @param vn The first source vector register for multiplication.
   * @param vm The second source vector register for multiplication.
   * @param arrangement The data arrangement (e.g., "4S" for four 32-bit
   * floats).
   * @return A complete Instruction object.
   */
  static Instruction create_fmla_vector_reg(const std::string &vd,
                                            const std::string &vn,
                                            const std::string &vm,
                                            const std::string &arrangement);

  /**
   * @brief Creates a vector FMUL (Floating-Point Multiply) instruction. (Vd =
   * Vn * Vm)
   * @param vd The destination vector register.
   * @param vn The first source vector register.
   * @param vm The second source vector register.
   * @param arrangement The data arrangement (e.g., "4S").
   * @return A complete Instruction object.
   */
  static Instruction create_fmul_vector_reg(const std::string &vd,
                                            const std::string &vn,
                                            const std::string &vm,
                                            const std::string &arrangement);

  /**
   * @brief Creates a vector ADD instruction for integers. (Vd = Vn + Vm)
   * @param vd The destination vector register.
   * @param vn The first source vector register.
   * @param vm The second source vector register.
   * @param arrangement The data arrangement (e.g., "4S" for four 32-bit ints).
   * @return A complete Instruction object.
   */
  static Instruction create_add_vector_reg(const std::string &vd,
                                           const std::string &vn,
                                           const std::string &vm,
                                           const std::string &arrangement);

  /**
   * @brief Creates an LD1 instruction to load multiple elements into one vector
   * register.
   * @param vt The destination vector register.
   * @param xn The base address register.
   * @param arrangement The data arrangement (e.g., "4S").
   * @return A complete Instruction object.
   */
  static Instruction create_ld1_vector_reg(const std::string &vt,
                                           const std::string &xn,
                                           const std::string &arrangement);

  /**
   * @brief Creates a vector FADD (Floating-Point Add) instruction. (Vd = Vn +
   * Vm)
   * @param vd The destination vector register (e.g., "V0").
   * @param vn The first source vector register.
   * @param vm The second source vector register.
   * @param arrangement The data arrangement (e.g., "4S" for 4x32-bit, "2D" for
   * 2x64-bit).
   * @return A complete Instruction object.
   */
  static Instruction create_fadd_vector_reg(const std::string &vd,
                                            const std::string &vn,
                                            const std::string &vm,
                                            const std::string &arrangement);

  /**
   * @brief Creates a vector MUL (Integer Multiply) instruction. (Vd = Vn * Vm)
   * @param vd The destination vector register.
   * @param vn The first source vector register.
   * @param vm The second source vector register.
   * @param arrangement The data arrangement (e.g., "4S", "8H").
   * @return A complete Instruction object.
   */
  static Instruction create_mul_vector_reg(const std::string &vd,
                                           const std::string &vn,
                                           const std::string &vm,
                                           const std::string &arrangement);

  /**
   * @brief Creates a dummy instruction for assembly directives (e.g., .data,
   * .word). This instruction will not be executed but will be listed in the
   * assembly output.
   * @param directive_text The assembly directive as a string.
   * @return A dummy Instruction object.
   */
  static Instruction create_directive(const std::string &directive_text,
                                      uint64_t data_value = 0,
                                      const std::string &target_label = "",
                                      bool is_data = false);

  /**
   * @brief Creates a MOV X29, SP instruction (frame pointer = stack pointer).
   * @return A complete Instruction object.
   */
  static Instruction create_mov_fp_sp();

  /**
   * @brief Creates a MOV SP, X29 instruction (stack pointer = frame pointer).
   * @return A complete Instruction object.
   */
  static Instruction create_mov_sp_fp();

  static Instruction create_dmb();
  /**
   * @brief Creates an SVC (Supervisor Call) instruction with an immediate
   * value.
   * @param immediate The 16-bit immediate value [0, 65535].
   * @return A complete Instruction object.
   */
  static Instruction create_svc_imm(uint16_t immediate);

  // CSETM instructions for direct BCPL boolean generation
  static Instruction create_csetm(const std::string &Rd,
                                  const std::string &cond);
  static Instruction create_cset(const std::string &Rd,
                                 const std::string &cond);
  static Instruction create_csetm_eq(const std::string &xd);
  static Instruction create_csetm_ne(const std::string &xd);
  static Instruction create_csetm_lt(const std::string &xd);
  static Instruction create_csetm_le(const std::string &xd);
  static Instruction create_csetm_gt(const std::string &xd);
  static Instruction create_csetm_ge(const std::string &xd);

  static Instruction opt_create_and_imm(const std::string& xd, const std::string& xn, int64_t immediate);
  static Instruction opt_create_orr_imm(const std::string& xd, const std::string& xn, int64_t immediate);
  static Instruction opt_create_eor_imm(const std::string& xd, const std::string& xn, int64_t immediate);
  static Instruction opt_create_ubfx(const std::string& xd, const std::string& xn, int lsb, int width);
  static Instruction opt_create_sbfx(const std::string& xd, const std::string& xn, int lsb, int width);
  static Instruction opt_create_bfi(const std::string& xd, const std::string& xn, int lsb, int width);
  static Instruction opt_create_csinv(const std::string& rd, const std::string& rn, const std::string& rm, const std::string& cond);
  static Instruction opt_create_add_shifted_reg(const std::string& rd, const std::string& rn, const std::string& rm, const std::string& shift_type, int shift_amount);
  static Instruction opt_create_fmadd(const std::string& vd, const std::string& vn, const std::string& vm, const std::string& va);
  static Instruction opt_create_fmsub(const std::string& vd, const std::string& vn, const std::string& vm, const std::string& va);
  static Instruction opt_create_asr_imm(const std::string& xd, const std::string& xn, int shift_amount);
  static Instruction opt_create_cbz(const std::string& xt, const std::string& label_name);
  static Instruction opt_create_cbnz(const std::string& xt, const std::string& label_name);
  static Instruction opt_create_lsl_imm(const std::string& xd, const std::string& xn, int shift_amount);
  static Instruction opt_create_asr_reg(const std::string& xd, const std::string& xn, const std::string& xm);

private:
  // Private constructor to prevent instantiation (all methods are static)
  Encoder() = delete;

  static bool encode_bitmask_immediate(uint64_t immediate, bool is_64bit, uint32_t& n_val, uint32_t& immr_val, uint32_t& imms_val);

  // Helper function to get the integer encoding of a register name
  static uint32_t get_reg_encoding(const std::string &reg);
  static uint32_t get_cond_encoding(const std::string &cond);
};

#endif // ENCODER_H
