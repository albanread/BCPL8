NewBCPL/gen/gen_IfStatement.cpp
#include "NewCodeGenerator.h"
#include "LabelManager.h"
#include "InstructionStream.h"
#include "RegisterManager.h"
#include "AST.h"

void NewCodeGenerator::visit(IfStatement& if_stmt) {
    // Generate code for the condition
    if_stmt.condition->accept(*this);
    auto condition_register = register_manager.get_last_used_register();

    // Create labels for branching
    auto then_label = label_manager.create_label("then");
    auto end_label = label_manager.create_label("end_if");

    // Emit a conditional branch based on the condition
    instruction_stream.emit("CMP", condition_register, "0");
    instruction_stream.emit("JE", then_label);

    // Generate code for the then branch
    instruction_stream.emit_label(then_label);
    if_stmt.then_branch->accept(*this);

    // Emit a jump to the end of the if statement
    instruction_stream.emit("JMP", end_label);

    // Emit the end label
    instruction_stream.emit_label(end_label);

    // Free the condition register
    register_manager.free_register(condition_register);
}