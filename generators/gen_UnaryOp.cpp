#include "../NewCodeGenerator.h"
#include "../LabelManager.h"
#include <stdexcept>
#include <cctype>
#include "../analysis/ASTAnalyzer.h" // For infer_expression_type

void NewCodeGenerator::visit(UnaryOp& node) {
    debug_print("Visiting UnaryOp node.");

    using Op = UnaryOp::Operator;

    // --- HD and HDf (HeadOf / HeadOfAsFloat) Operations ---
    if (node.op == Op::HeadOf || node.op == Op::HeadOfAsFloat) {
        debug_print("Generating INLINE code for hybrid static/runtime HD operation.");
        bool is_float_op = (node.op == Op::HeadOfAsFloat);

        // At COMPILE TIME, we ask the analyzer for the best type information it has.
        VarType operand_type = ASTAnalyzer::getInstance().infer_expression_type(node.operand.get());
        
        generate_expression_code(*node.operand);
        std::string ptr_reg = expression_result_reg_;
        std::string result_reg = is_float_op ? register_manager_.acquire_fp_scratch_reg() : register_manager_.acquire_scratch_reg(*this);

        // This path is taken for FOREACH cursors, where we know the type is a node.
        if (operand_type == VarType::POINTER_TO_LIST_NODE) {
            // --- STATIC FAST PATH ---
            debug_print("HD operand is statically known as a List Node. Generating direct load.");
            
            std::string end_label = label_manager_.create_label();
            std::string is_null_label = label_manager_.create_label();

            // Check if the node pointer itself is null before dereferencing
            emit(Encoder::create_cmp_imm(ptr_reg, 0));
            emit(Encoder::create_branch_conditional("EQ", is_null_label));

            // Not null: Load value from node->value (offset 8)
            if (is_float_op) {
                emit(Encoder::create_ldr_fp_imm(result_reg, ptr_reg, 8));
            } else {
                emit(Encoder::create_ldr_imm(result_reg, ptr_reg, 8));
            }
            emit(Encoder::create_branch_unconditional(end_label));

            // Is null: produce a zero value
            instruction_stream_.define_label(is_null_label);
             if (is_float_op) {
                std::string temp_zero = register_manager_.acquire_scratch_reg(*this);
                emit(Encoder::create_mov_reg(temp_zero, "XZR"));
                emit(Encoder::create_scvtf_reg(result_reg, temp_zero));
                register_manager_.release_register(temp_zero);
            } else {
                emit(Encoder::create_mov_reg(result_reg, "XZR"));
            }
            
            instruction_stream_.define_label(end_label);

        } else {
            // --- DYNAMIC SAFE PATH ---
            // The type is ambiguous (e.g., HD(ANY_LIST)). Generate a runtime check on the type tag.
            debug_print("HD operand type is a list header. Generating runtime check logic.");

            std::string node_ptr_reg = register_manager_.acquire_scratch_reg(*this);
            std::string end_label = label_manager_.create_label();
            std::string is_empty_label = label_manager_.create_label();
            
            // 1. Load the head node pointer from the list header (offset 16).
            emit(Encoder::create_ldr_imm(node_ptr_reg, ptr_reg, 16, "Load head pointer"));

            // 2. Check if the head pointer is null (empty list).
            emit(Encoder::create_cmp_imm(node_ptr_reg, 0));
            emit(Encoder::create_branch_conditional("EQ", is_empty_label));

            // 3. List is not empty: Load value from the node's value field (offset 8).
            if (is_float_op) {
                emit(Encoder::create_ldr_fp_imm(result_reg, node_ptr_reg, 8));
            } else {
                emit(Encoder::create_ldr_imm(result_reg, node_ptr_reg, 8));
                if (operand_type == VarType::POINTER_TO_STRING_LIST || operand_type == VarType::CONST_POINTER_TO_STRING_LIST) {
                    emit(Encoder::create_add_imm(result_reg, result_reg, 8));
                }
            }
            emit(Encoder::create_branch_unconditional(end_label));

            // 4. List is empty: produce a zero value.
            instruction_stream_.define_label(is_empty_label);
            if (is_float_op) {
                std::string temp_zero = register_manager_.acquire_scratch_reg(*this);
                emit(Encoder::create_mov_reg(temp_zero, "XZR"));
                emit(Encoder::create_scvtf_reg(result_reg, temp_zero));
                register_manager_.release_register(temp_zero);
            } else {
                emit(Encoder::create_mov_reg(result_reg, "XZR"));
            }
            
            // 5. Join point.
            instruction_stream_.define_label(end_label);
            register_manager_.release_register(node_ptr_reg);
        }

        expression_result_reg_ = result_reg;
        register_manager_.release_register(ptr_reg);
        return;
    }

    // --- TL (TailOf) destructive operation ---
    if (node.op == Op::TailOf) {
        debug_print("Generating INLINE code for destructive TL operation.");
        generate_expression_code(*node.operand);
        std::string header_reg = expression_result_reg_;

        std::string end_tl_label = label_manager_.create_label();
        std::string not_tail_label = label_manager_.create_label();

        std::string old_head_reg = register_manager_.acquire_scratch_reg(*this);
        std::string new_head_reg = register_manager_.acquire_scratch_reg(*this);
        std::string tail_ptr_reg = register_manager_.acquire_scratch_reg(*this);

        // Load the current head node pointer (from header->head at offset 16).
        emit(Encoder::create_ldr_imm(old_head_reg, header_reg, 16, "Load old head pointer"));

        // If the list is empty (old head is null), there's nothing to do.
        emit(Encoder::create_cmp_imm(old_head_reg, 0));
        emit(Encoder::create_branch_conditional("EQ", end_tl_label));

        // Load the pointer to the *new* head (from old_head->next at offset 16).
        emit(Encoder::create_ldr_imm(new_head_reg, old_head_reg, 16, "Load new head pointer"));

        // Update the list header to point to the new head.
        emit(Encoder::create_str_imm(new_head_reg, header_reg, 16, "Update header->head"));

        // Check if we removed the tail. If so, update the header's tail pointer to NULL.
        emit(Encoder::create_ldr_imm(tail_ptr_reg, header_reg, 8, "Load tail pointer"));
        emit(Encoder::create_cmp_reg(tail_ptr_reg, old_head_reg));
        emit(Encoder::create_branch_conditional("NE", not_tail_label));
        emit(Encoder::create_str_imm("XZR", header_reg, 8, "Clear tail pointer if it was the removed node")); // Store NULL
        instruction_stream_.define_label(not_tail_label);

        // Move the old head pointer to X0 and call the fast freelist return function.
        emit(Encoder::create_mov_reg("X0", old_head_reg));
        emit(Encoder::create_branch_with_link("returnNodeToFreelist"));

        instruction_stream_.define_label(end_tl_label);

        // The result of a destructive TL is the original list pointer, which is still in header_reg.
        expression_result_reg_ = header_reg;

        register_manager_.release_register(old_head_reg);
        register_manager_.release_register(new_head_reg);
        register_manager_.release_register(tail_ptr_reg);
        return;
    }

    // --- REST (TailOfNonDestructive) operation ---
    if (node.op == Op::TailOfNonDestructive) {
        //
        // CRITICAL EXPLANATION: This logic is intentionally designed with two paths and
        // must NOT be simplified. The 'REST' operator can be applied to two different
        // kinds of pointers, and the generated code must handle both cases correctly.
        //
        // 1. A List Header (e.g., REST(myList)): This requires a "Safe Path" that
        //    dereferences the header to get the first node, and then dereferences that
        //    first node to get the pointer to the second node (header->head->next).
        //
        // 2. A Node Pointer (e.g., REST(cursor) in a loop): This requires a "Fast Path"
        //    that directly dereferences the node to get its 'next' pointer (node->next).
        //
        // Simplifying this to a single path will break one of these two essential use cases.
        //
        debug_print("Generating INLINE code for hybrid static/runtime REST operation.");
        VarType operand_type = ASTAnalyzer::getInstance().infer_expression_type(node.operand.get());

        generate_expression_code(*node.operand);
        std::string ptr_reg = expression_result_reg_;
        std::string result_reg = register_manager_.acquire_scratch_reg(*this);

        if (operand_type == VarType::POINTER_TO_LIST_NODE) {
            // --- FAST PATH (Operand is a Node Pointer) ---
            debug_print("REST operand is statically known as a List Node. Generating direct next-ptr load.");
            std::string end_label = label_manager_.create_label();
            std::string is_null_label = label_manager_.create_label();

            // Check if the node pointer itself is null before dereferencing.
            emit(Encoder::create_cmp_imm(ptr_reg, 0));
            emit(Encoder::create_branch_conditional("EQ", is_null_label));

            // Not null: Load the next pointer from node->next (offset 16).
            emit(Encoder::create_ldr_imm(result_reg, ptr_reg, 16));
            emit(Encoder::create_branch_unconditional(end_label));

            // Is null: The result is also null.
            instruction_stream_.define_label(is_null_label);
            emit(Encoder::create_mov_reg(result_reg, "XZR"));

            instruction_stream_.define_label(end_label);

        } else {
            // --- SAFE PATH (Operand is a List Header) ---
            debug_print("REST operand type is a list header. Generating safe, double-dereference logic.");
            std::string head_node_reg = register_manager_.acquire_scratch_reg(*this);
            std::string end_label = label_manager_.create_label();
            std::string is_null_label = label_manager_.create_label();

            // 1. Load the head node pointer from the list header (offset 16).
            emit(Encoder::create_ldr_imm(head_node_reg, ptr_reg, 16, "Load head pointer"));

            // 2. Check if the head pointer is null (empty list).
            emit(Encoder::create_cmp_imm(head_node_reg, 0));
            emit(Encoder::create_branch_conditional("EQ", is_null_label));

            // 3. List is not empty: Load the 'next' pointer from the head node (offset 16).
            // This is the pointer to the second node, which is our result.
            emit(Encoder::create_ldr_imm(result_reg, head_node_reg, 16, "Load next pointer (the 'rest')"));
            emit(Encoder::create_branch_unconditional(end_label));

            // 4. List was empty or head was null: the 'rest' is null.
            instruction_stream_.define_label(is_null_label);
            emit(Encoder::create_mov_reg(result_reg, "XZR"));

            // 5. Join point.
            instruction_stream_.define_label(end_label);
            register_manager_.release_register(head_node_reg);
        }

        expression_result_reg_ = result_reg;
        register_manager_.release_register(ptr_reg);
        return;
    }

    // --- TYPEOF intrinsic ---
    if (node.op == Op::TypeOf) {
        debug_print("Generating code for TYPEOF operator.");
        generate_expression_code(*node.operand);
        std::string ptr_reg = expression_result_reg_;
        std::string dest_reg = register_manager_.acquire_scratch_reg(*this);
        // Load the 32-bit integer type tag from offset 0 of the ListAtom struct.
        emit(Encoder::create_ldr_word_imm("W" + dest_reg.substr(1), ptr_reg, 0));
        register_manager_.release_register(ptr_reg);
        expression_result_reg_ = dest_reg;
        return;
    }

    // --- LEN (LengthOf) operator for vectors AND lists ---
    if (node.op == Op::LengthOf) {
        debug_print("Generating code for LEN operator.");
        VarType operand_type = ASTAnalyzer::getInstance().infer_expression_type(node.operand.get());
        generate_expression_code(*node.operand);
        std::string payload_ptr_reg = expression_result_reg_;
        std::string dest_reg = register_manager_.acquire_scratch_reg(*this);

        if (
            operand_type == VarType::POINTER_TO_INT_VEC ||
            operand_type == VarType::POINTER_TO_FLOAT_VEC ||
            operand_type == VarType::POINTER_TO_STRING
        ) {
            // This logic is now correct for both VEC and TABLE.
            std::string base_addr_reg = register_manager_.acquire_scratch_reg(*this);
            emit(Encoder::create_sub_imm(base_addr_reg, payload_ptr_reg, 8));
            emit(Encoder::create_ldr_imm(dest_reg, base_addr_reg, 0, "Load vector/table/string length"));
            register_manager_.release_register(base_addr_reg);

        } else if (
            operand_type == VarType::POINTER_TO_ANY_LIST ||
            operand_type == VarType::POINTER_TO_INT_LIST ||
            operand_type == VarType::POINTER_TO_FLOAT_LIST ||
            operand_type == VarType::CONST_POINTER_TO_ANY_LIST ||
            operand_type == VarType::CONST_POINTER_TO_INT_LIST ||
            operand_type == VarType::CONST_POINTER_TO_FLOAT_LIST
        ) {
            // It's a list. The length is at offset 24 in the ListHeader struct.
            emit(Encoder::create_ldr_imm(dest_reg, payload_ptr_reg, 24, "Load list length"));
        } else {
            throw std::runtime_error("Code generation for LEN operator failed: operand was not a vector or list.");
        }

        register_manager_.release_register(payload_ptr_reg);
        expression_result_reg_ = dest_reg;
        debug_print("Finished LEN operator. Result in " + expression_result_reg_);
        return;
    }

    // --- Standard Unary Operations (Float and Integer) ---

    generate_expression_code(*node.operand);
    std::string operand_reg = expression_result_reg_;
    
    // Determine if we need floating-point instructions
    bool operand_is_float_reg = register_manager_.is_fp_register(operand_reg);
    bool op_is_float_specific = (node.op == Op::FloatConvert || node.op == Op::FloatSqrt ||
                                 node.op == Op::FloatFloor || node.op == Op::FloatTruncate);

    if (operand_is_float_reg || op_is_float_specific) {
        // --- Floating-Point Path ---
        std::string result_reg;
        std::string source_fp_reg = operand_reg;
        bool created_temp_fp_source = false;

        // If the operand is an integer but the operation is float-specific, convert it first.
        if (!operand_is_float_reg && op_is_float_specific) {
            source_fp_reg = register_manager_.acquire_fp_scratch_reg();
            emit(Encoder::create_scvtf_reg(source_fp_reg, operand_reg));
            created_temp_fp_source = true;
        }

        switch (node.op) {
            case Op::FloatConvert:
                expression_result_reg_ = source_fp_reg; // It's already a float or has been converted.
                if (created_temp_fp_source) register_manager_.release_register(operand_reg);
                return;
            case Op::FloatSqrt:
                result_reg = register_manager_.acquire_fp_scratch_reg();
                emit(Encoder::create_fsqrt_reg(result_reg, source_fp_reg));
                expression_result_reg_ = result_reg;
                break;
            case Op::FloatFloor: // ENTIER
            case Op::FloatTruncate:
                result_reg = register_manager_.acquire_scratch_reg(*this); // Integer result
                if (node.op == Op::FloatFloor) {
                    emit(Encoder::create_fcvtms_reg(result_reg, source_fp_reg));
                } else { // TRUNC
                    emit(Encoder::create_fcvtzs_reg(result_reg, source_fp_reg));
                }
                expression_result_reg_ = result_reg;
                break;
            case Op::Negate:
                result_reg = register_manager_.acquire_fp_scratch_reg();
                emit(Encoder::create_fneg_reg(result_reg, source_fp_reg));
                expression_result_reg_ = result_reg;
                break;
            default:
                throw std::runtime_error("Unsupported unary operation on a floating-point operand.");
        }

        // Cleanup intermediate registers
        if (created_temp_fp_source) {
            register_manager_.release_register(operand_reg);
            register_manager_.release_fp_register(source_fp_reg);
        } else {
            register_manager_.release_fp_register(operand_reg);
        }

    } else {
        // --- Integer Path ---
        std::string dest_reg = register_manager_.acquire_scratch_reg(*this);
        switch (node.op) {
            case Op::AddressOf:
                if (auto* var_access = dynamic_cast<VariableAccess*>(node.operand.get())) {
                    if (current_frame_manager_ && current_frame_manager_->has_local(var_access->name)) {
                        int offset = current_frame_manager_->get_offset(var_access->name);
                        emit(Encoder::create_add_imm(dest_reg, "X29", offset));
                    } else {
                        // Assume global/static
                        emit(Encoder::create_adrp(dest_reg, var_access->name));
                        emit(Encoder::create_add_literal(dest_reg, dest_reg, var_access->name));
                    }
                } else {
                    throw std::runtime_error("AddressOf operator must be applied to a variable.");
                }
                break;
            case Op::Indirection:
                emit(Encoder::create_ldr_imm(dest_reg, operand_reg, 0));
                break;
            case Op::BitwiseNot:
                // Emits MVN (Move with NOT), which is the ARM64 instruction for bitwise NOT.
                emit(Encoder::create_mvn_reg(dest_reg, operand_reg));
                break;
            case Op::LogicalNot:
                emit(Encoder::create_cmp_reg(operand_reg, "XZR"));
                emit(Encoder::create_cset(dest_reg, "EQ")); // Set to 1 if equal to zero, else 0
                break;
            case Op::Negate:
                emit(Encoder::create_sub_reg(dest_reg, "XZR", operand_reg));
                break;
            default:
                 throw std::runtime_error("Unhandled integer unary operator.");
        }
        register_manager_.release_register(operand_reg);
        expression_result_reg_ = dest_reg;
    }
}
