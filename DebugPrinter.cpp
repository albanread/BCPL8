#include "DebugPrinter.h"
#include <string>
#include <iostream>
#include "AST.h"
#include "ASTVisitor.h"



// --- Helper functions to convert operators to strings ---

namespace {
std::string to_string(BinaryOp::Operator op) {
    switch (op) {
        case BinaryOp::Operator::Add: return "Add";
        case BinaryOp::Operator::Subtract: return "Subtract";
        case BinaryOp::Operator::Multiply: return "Multiply";
        case BinaryOp::Operator::Divide: return "Divide";
        case BinaryOp::Operator::Remainder: return "Remainder";
        case BinaryOp::Operator::Equal: return "Equal";
        case BinaryOp::Operator::NotEqual: return "NotEqual";
        case BinaryOp::Operator::Less: return "Less";
        case BinaryOp::Operator::LessEqual: return "LessEqual";
        case BinaryOp::Operator::Greater: return "Greater";
        case BinaryOp::Operator::GreaterEqual: return "GreaterEqual";
        case BinaryOp::Operator::LogicalAnd: return "LogicalAnd";
        case BinaryOp::Operator::LogicalOr: return "LogicalOr";
        case BinaryOp::Operator::Equivalence: return "Equivalence";
        case BinaryOp::Operator::NotEquivalence: return "NotEquivalence";
        case BinaryOp::Operator::LeftShift: return "LeftShift";
        case BinaryOp::Operator::RightShift: return "RightShift";
        case BinaryOp::Operator::FloatAdd: return "FloatAdd";
        case BinaryOp::Operator::FloatSubtract: return "FloatSubtract";
        case BinaryOp::Operator::FloatMultiply: return "FloatMultiply";
        case BinaryOp::Operator::FloatDivide: return "FloatDivide";
        case BinaryOp::Operator::FloatEqual: return "FloatEqual";
        case BinaryOp::Operator::FloatNotEqual: return "FloatNotEqual";
        case BinaryOp::Operator::FloatLess: return "FloatLess";
        case BinaryOp::Operator::FloatLessEqual: return "FloatLessEqual";
        case BinaryOp::Operator::FloatGreater: return "FloatGreater";
        case BinaryOp::Operator::FloatGreaterEqual: return "FloatGreaterEqual";
        default: return "UnknownBinaryOp";
    }


}

std::string to_string(UnaryOp::Operator op) {
    switch (op) {
        case UnaryOp::Operator::AddressOf: return "AddressOf";
        case UnaryOp::Operator::Indirection: return "Indirection";
        case UnaryOp::Operator::LogicalNot: return "LogicalNot";
        case UnaryOp::Operator::Negate: return "Negate";
        case UnaryOp::Operator::FloatConvert: return "FloatConvert";
        case UnaryOp::Operator::FloatSqrt: return "FloatSqrt";
        case UnaryOp::Operator::HeadOf: return "HeadOf";
        case UnaryOp::Operator::HeadOfAsFloat: return "HeadOfAsFloat";
        case UnaryOp::Operator::TailOf: return "TailOf";
        case UnaryOp::Operator::TailOfNonDestructive: return "TailOfNonDestructive (REST)";
        case UnaryOp::Operator::LengthOf: return "LengthOf (LEN)";
        case UnaryOp::Operator::FloatFloor: return "FloatFloor (ENTIER)";
        case UnaryOp::Operator::FloatTruncate: return "FloatTruncate (TRUNC)";
        default: return "UnknownUnaryOp";
    }
}
} // anonymous namespace

void DebugPrinter::print(Program& program) {
    print_line("Program:");
    indent_level_++;
    program.accept(*this);
    indent_level_--;
}

void DebugPrinter::visit(LabelTargetStatement& node) {
    std::cout << "LabelTargetStatement: " << node.labelName << std::endl;
}

void DebugPrinter::visit(ConditionalBranchStatement& node) {
    std::cout << "ConditionalBranchStatement: condition=" << node.condition << ", targetLabel=" << node.targetLabel << std::endl;
}

void DebugPrinter::visit(FreeStatement& node) {
    print_line("FreeStatement:");
    print_child(node.list_expr);
}

void DebugPrinter::print_indent() {
    std::cout << std::string(indent_level_ * 2, ' ');
}

void DebugPrinter::print_line(const std::string& text) {
    print_indent();
    std::cout << text << std::endl;
}

// --- Visitor Implementations ---

void DebugPrinter::visit(SysCall& node) {
    print_line("SysCall:");
    indent_level_++;
    print_line("Function Name: " + node.function_name);
    print_line("Arguments:");
    for (const auto& arg : node.arguments) {
        print_child(arg);
    }
    indent_level_--;
}

void DebugPrinter::visit(Program& node) {
    if (!node.declarations.empty()) {
        print_line("Declarations:");
        for (const auto& decl : node.declarations) {
            print_child(decl);
        }
    }
    if (!node.statements.empty()) {
        print_line("Statements:");
        for (const auto& stmt : node.statements) {
            print_child(stmt);
        }
    }
}

void DebugPrinter::visit(LetDeclaration& node) {
    print_line("LetDeclaration:");
    indent_level_++;
    for (size_t i = 0; i < node.names.size(); ++i) {
        print_line("Name: " + node.names[i]);
        if (i < node.initializers.size() && node.initializers[i]) {
            print_line("Initializer:");
            print_child(node.initializers[i]);
        }
    }
    indent_level_--;
}

void DebugPrinter::visit(FunctionDeclaration& node) {
    print_line("FunctionDeclaration: " + node.name);
    indent_level_++;
    if (!node.parameters.empty()) {
        std::string params_str = "Parameters: ";
        for(const auto& p : node.parameters) params_str += p + " ";
        print_line(params_str);
    }
    print_line("Body:");
    print_child(node.body);
    indent_level_--;
}

void DebugPrinter::visit(RoutineDeclaration& node) {
    print_line("RoutineDeclaration: " + node.name);
    indent_level_++;
    if (!node.parameters.empty()) {
        std::string params_str = "Parameters: ";
        for(const auto& p : node.parameters) params_str += p + " ";
        print_line(params_str);
    }
    print_line("Body:");
    print_child(node.body);
    indent_level_--;
}

void DebugPrinter::visit(ManifestDeclaration& node) {
    print_line("ManifestDeclaration: " + node.name + " = " + std::to_string(node.value));
}

void DebugPrinter::visit(StaticDeclaration& node) {
    print_line("StaticDeclaration: " + node.name);
    print_child(node.initializer);
}

void DebugPrinter::visit(GlobalDeclaration& node) {
    print_line("GlobalDeclaration:");
    indent_level_++;
    for (const auto& global_pair : node.globals) {
        print_line("Name: " + global_pair.first + " @ " + std::to_string(global_pair.second));
    }
    indent_level_--;
}

void DebugPrinter::visit(LabelDeclaration& node) {
    print_line("LabelDeclaration: " + node.name);
    print_child(node.command);
}



void DebugPrinter::visit(NumberLiteral& node) {
    if (node.literal_type == NumberLiteral::LiteralType::Integer) {
        print_line("NumberLiteral: " + std::to_string(node.int_value));
    } else {
        print_line("FloatLiteral: " + std::to_string(node.float_value));
    }
}

void DebugPrinter::visit(StringLiteral& node) {
    print_line("StringLiteral: \"" + node.value + "\"");
}

void DebugPrinter::visit(CharLiteral& node) {
    print_line("CharLiteral: '" + std::string(1, node.value) + "'");
}

void DebugPrinter::visit(BooleanLiteral& node) {
    print_line(std::string("BooleanLiteral: ") + (node.value ? "TRUE" : "FALSE"));
}

void DebugPrinter::visit(VariableAccess& node) {
    print_line("VariableAccess: " + node.name);
}

void DebugPrinter::visit(BinaryOp& node) {
    print_line("BinaryOp: " + to_string(node.op));
    print_child(node.left);
    print_child(node.right);
}

void DebugPrinter::visit(UnaryOp& node) {
    print_line("UnaryOp: " + to_string(node.op));
    print_child(node.operand);
}

void DebugPrinter::visit(VectorAccess& node) {
    print_line("VectorAccess (!):");
    print_child(node.vector_expr);
    print_child(node.index_expr);
}

void DebugPrinter::visit(CharIndirection& node) {
    print_line("CharIndirection (%):");
    print_child(node.string_expr);
    print_child(node.index_expr);
}

void DebugPrinter::visit(FloatVectorIndirection& node) {
    print_line("FloatVectorIndirection (.%):");
    print_child(node.vector_expr);
    print_child(node.index_expr);
}

void DebugPrinter::visit(FunctionCall& node) {
    print_line("FunctionCall:");
    indent_level_++;
    print_line("Callee:");
    print_child(node.function_expr);
    if (!node.arguments.empty()) {
        print_line("Arguments:");
        for (const auto& arg : node.arguments) {
            print_child(arg);
        }
    }
    indent_level_--;
}

void DebugPrinter::visit(ConditionalExpression& node) {
    print_line("ConditionalExpression:");
    indent_level_++;
    print_line("Condition:");
    print_child(node.condition);
    print_line("True Branch:");
    print_child(node.true_expr);
    print_line("False Branch:");
    print_child(node.false_expr);
    indent_level_--;
}

void DebugPrinter::visit(ValofExpression& node) {
    print_line("ValofExpression:");
    print_child(node.body);
}

void DebugPrinter::visit(VecAllocationExpression& node) {
    print_line("VecAllocationExpression:");
    print_child(node.size_expr);
}

void DebugPrinter::visit(StringAllocationExpression& node) {
    print_line("StringAllocationExpression:");
    print_child(node.size_expr);
}

void DebugPrinter::visit(TableExpression& node) {
    print_line("TableExpression:");
    for(const auto& expr : node.initializers) {
        print_child(expr);
    }
}

void DebugPrinter::visit(AssignmentStatement& node) {
    print_line("AssignmentStatement:");
    indent_level_++;
    print_line("LHS:");
    for(const auto& lhs : node.lhs) print_child(lhs);
    print_line("RHS:");
    for(const auto& rhs : node.rhs) print_child(rhs);
    indent_level_--;
}

void DebugPrinter::visit(RoutineCallStatement& node) {
    print_line("RoutineCallStatement:");
    indent_level_++;
    print_line("Callee:");
    print_child(node.routine_expr);
    if (!node.arguments.empty()) {
        print_line("Arguments:");
        for (const auto& arg : node.arguments) {
            print_child(arg);
        }
    }
    indent_level_--;
}

void DebugPrinter::visit(IfStatement& node) {
    print_line("IfStatement:");
    indent_level_++;
    print_line("Condition:");
    print_child(node.condition);
    print_line("Then:");
    print_child(node.then_branch);
    indent_level_--;
}

void DebugPrinter::visit(UnlessStatement& node) {
    print_line("UnlessStatement:");
    indent_level_++;
    print_line("Condition:");
    print_child(node.condition);
    print_line("Then:");
    print_child(node.then_branch);
    indent_level_--;
}

void DebugPrinter::visit(TestStatement& node) {
    print_line("TestStatement:");
    indent_level_++;
    print_line("Condition:");
    print_child(node.condition);
    print_line("Then:");
    print_child(node.then_branch);
    print_line("Else:");
    print_child(node.else_branch);
    indent_level_--;
}

void DebugPrinter::visit(BrkStatement& node) {
    print_line("BrkStatement");
}

void DebugPrinter::visit(WhileStatement& node) {
    print_line("WhileStatement:");
    indent_level_++;
    print_line("Condition:");
    print_child(node.condition);
    print_line("Body:");
    print_child(node.body);
    indent_level_--;
}

void DebugPrinter::visit(UntilStatement& node) {
    print_line("UntilStatement:");
    indent_level_++;
    print_line("Condition:");
    print_child(node.condition);
    print_line("Body:");
    print_child(node.body);
    indent_level_--;
}

void DebugPrinter::visit(RepeatStatement& node) {
    print_line("RepeatStatement:");
    indent_level_++;
    print_line("Body:");
    print_child(node.body);
    if (node.condition) {
        print_line("Condition:");
        print_child(node.condition);
    }
    indent_level_--;
}

void DebugPrinter::visit(ForStatement& node) {
    print_line("ForStatement: " + node.loop_variable);
    indent_level_++;
    print_line("Start:");
    print_child(node.start_expr);
    print_line("End:");
    print_child(node.end_expr);
    if (node.step_expr) {
        print_line("Step:");
        print_child(node.step_expr);
    }
    print_line("Body:");
    print_child(node.body);
    indent_level_--;
}

void DebugPrinter::visit(SwitchonStatement& node) {
    print_line("SwitchonStatement:");
    indent_level_++;
    print_line("Expression:");
    print_child(node.expression);
    print_line("Cases:");
    for(const auto& c : node.cases) print_child(c);
    if (node.default_case) {
        print_line("Default:");
        print_child(node.default_case);
    }
    indent_level_--;
}

void DebugPrinter::visit(CaseStatement& node) {
    print_line("CaseStatement:");
    indent_level_++;
    print_line("Constant:");
    print_child(node.constant_expr);
    print_line("Command:");
    print_child(node.command);
    indent_level_--;
}

void DebugPrinter::visit(DefaultStatement& node) {
    print_line("DefaultStatement:");
    print_child(node.command);
}

void DebugPrinter::visit(GotoStatement& node) {
    print_line("GotoStatement:");
    print_child(node.label_expr);
}

void DebugPrinter::visit(ReturnStatement& node) {
    print_line("ReturnStatement");
}

void DebugPrinter::visit(FinishStatement& node) {
    print_line("FinishStatement");
}

void DebugPrinter::visit(BreakStatement& node) {
    print_line("BreakStatement");
}

void DebugPrinter::visit(LoopStatement& node) {
    print_line("LoopStatement");
}

void DebugPrinter::visit(EndcaseStatement& node) {
    print_line("EndcaseStatement");
}

void DebugPrinter::visit(ResultisStatement& node) {
    print_line("ResultisStatement:");
    print_child(node.expression);
}

void DebugPrinter::visit(CompoundStatement& node) {
    print_line("CompoundStatement:");
    for(const auto& stmt : node.statements) {
        print_child(stmt);
    }
}

void DebugPrinter::visit(BlockStatement& node) {
    print_line("BlockStatement:");
    indent_level_++;
    if (!node.statements.empty()) {
        print_line("Statements:");
        for(const auto& stmt : node.statements) print_child(stmt);
    }
    indent_level_--;
}

void DebugPrinter::visit(StringStatement& node) {
    print_line("StringStatement:");
    print_child(node.size_expr);
}

void DebugPrinter::visit(GlobalVariableDeclaration& node) {
    print_line("GlobalVariableDeclaration");
    indent_level_++;
    for (size_t i = 0; i < node.names.size(); ++i) {
        print_line("Name: " + node.names[i]);
        if (i < node.initializers.size() && node.initializers[i]) {
            print_line("Initializer:");
            print_child(node.initializers[i]);
        }
    }
    if (node.is_float_declaration) {
        print_line("Type: Float");
    }
    indent_level_--;
}


