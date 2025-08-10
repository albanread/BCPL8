#include "CodeFormatter.h"
#include <algorithm>
#include <cctype>
#include <sstream>

// --- Helpers for operator stringification ---
static std::string binop_to_str(BinaryOp::Operator op) {
    using Op = BinaryOp::Operator;
    switch (op) {
        case Op::Add: return "+";
        case Op::Subtract: return "-";
        case Op::Multiply: return "*";
        case Op::Divide: return "/";
        case Op::Remainder: return "%";
        case Op::Equal: return "=";
        case Op::NotEqual: return "~=";
        case Op::Less: return "<";
        case Op::LessEqual: return "<=";
        case Op::Greater: return ">";
        case Op::GreaterEqual: return ">=";
        case Op::LogicalAnd: return "&";
        case Op::LogicalOr: return "!";
        case Op::Equivalence: return "==";
        case Op::NotEquivalence: return "!=";
        case Op::LeftShift: return "<<";
        case Op::RightShift: return ">>";
        case Op::FloatAdd: return "+.";
        case Op::FloatSubtract: return "-.";
        case Op::FloatMultiply: return "*.";
        case Op::FloatDivide: return "/.";
        case Op::FloatEqual: return "=.F";
        case Op::FloatNotEqual: return "~=.F";
        case Op::FloatLess: return "<.F";
        case Op::FloatLessEqual: return "<=.F";
        case Op::FloatGreater: return ">.F";
        case Op::FloatGreaterEqual: return ">=.F";
        default: return "?";
    }
}

static std::string unop_to_str(UnaryOp::Operator op) {
    using Op = UnaryOp::Operator;
    switch (op) {
        case Op::AddressOf: return "@";
        case Op::Indirection: return "*";
        case Op::LogicalNot: return "~";
        case Op::Negate: return "-";
        case Op::FloatConvert: return "FLOAT";
        default: return "?";
    }
}

// --- CodeFormatter Implementation ---

CodeFormatter::CodeFormatter() {}

std::string CodeFormatter::format(Program& program) {
    output_.str("");
    indent_level_ = 0;
    program.accept(*this);
    return output_.str();
}

void CodeFormatter::indent() { indent_level_++; }
void CodeFormatter::dedent() { if (indent_level_ > 0) indent_level_--; }
void CodeFormatter::write(const std::string& text) { output_ << text; }
void CodeFormatter::write_line(const std::string& text) {
    std::string line;
    std::stringstream ss;
    write_indent();
    ss << text;
    line = ss.str();
    // Remove trailing whitespace
    while (!line.empty() && (line.back() == ' ' || line.back() == '\t'))
        line.pop_back();
    output_ << line << "\n";
}
void CodeFormatter::write_indent() {
    for (int i = 0; i < indent_level_; ++i) {
        output_ << "  "; // 2 spaces per indent
    }
}
std::string CodeFormatter::uppercase(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c){ return std::toupper(c); });
    return s;
}

// --- Expression Formatting ---
std::string CodeFormatter::format_expression(Expression* expr) {
    if (!expr) return "";
    switch (expr->getType()) {
        case ASTNode::NodeType::NumberLit: {
            auto* lit = static_cast<NumberLiteral*>(expr);
            if (lit->literal_type == NumberLiteral::LiteralType::Integer)
                return std::to_string(lit->int_value);
            else
                return std::to_string(lit->float_value);
        }
        case ASTNode::NodeType::StringLit: {
            auto* lit = static_cast<StringLiteral*>(expr);
            std::string escaped;
            for (char c : lit->value) {
                if (c == '\n') escaped += "\\n";
                else if (c == '\t') escaped += "\\t";
                else if (c == '\"') escaped += "\\\"";
                else escaped += c;
            }
            return "\"" + escaped + "\"";
        }
        case ASTNode::NodeType::CharLit: {
            auto* lit = static_cast<CharLiteral*>(expr);
            return "'" + std::string(1, lit->value) + "'";
        }
        case ASTNode::NodeType::BooleanLit: {
            auto* lit = static_cast<BooleanLiteral*>(expr);
            return lit->value ? "TRUE" : "FALSE";
        }
        case ASTNode::NodeType::VariableAccessExpr: {
            auto* var = static_cast<VariableAccess*>(expr);
            return var->name;
        }
        case ASTNode::NodeType::BinaryOpExpr: {
            auto* bin = static_cast<BinaryOp*>(expr);
            return format_expression(bin->left.get()) + " " + binop_to_str(bin->op) + " " + format_expression(bin->right.get());
        }
        case ASTNode::NodeType::UnaryOpExpr: {
            auto* un = static_cast<UnaryOp*>(expr);
            return unop_to_str(un->op) + format_expression(un->operand.get());
        }
        case ASTNode::NodeType::VectorAccessExpr: {
            auto* v = static_cast<VectorAccess*>(expr);
            return format_expression(v->vector_expr.get()) + "[" + format_expression(v->index_expr.get()) + "]";
        }
        case ASTNode::NodeType::CharIndirectionExpr: {
            auto* v = static_cast<CharIndirection*>(expr);
            return format_expression(v->string_expr.get()) + " ! " + format_expression(v->index_expr.get());
        }
        case ASTNode::NodeType::FloatVectorIndirectionExpr: {
            auto* v = static_cast<FloatVectorIndirection*>(expr);
            return format_expression(v->vector_expr.get()) + " # " + format_expression(v->index_expr.get());
        }
        case ASTNode::NodeType::FunctionCallExpr: {
            auto* call = static_cast<FunctionCall*>(expr);
            std::string result = format_expression(call->function_expr.get()) + "(";
            for (size_t i = 0; i < call->arguments.size(); ++i) {
                result += format_expression(call->arguments[i].get());
                if (i < call->arguments.size() - 1) result += ", ";
            }
            // Remove all spaces before every ')'
            std::string cleaned;
            for (size_t i = 0; i < result.size(); ++i) {
                if (result[i] == ')') {
                    size_t j = cleaned.size();
                    while (j > 0 && cleaned[j-1] == ' ') --j;
                    cleaned.erase(j, cleaned.size() - j);
                }
                cleaned += result[i];
            }
            cleaned += ")";
            return cleaned;
        }
        case ASTNode::NodeType::SysCallExpr: {
            auto* call = static_cast<SysCall*>(expr);
            std::string result = call->function_name + "(";
            for (size_t i = 0; i < call->arguments.size(); ++i) {
                result += format_expression(call->arguments[i].get());
                if (i < call->arguments.size() - 1) result += ", ";
            }
            // Remove all spaces before every ')'
            std::string cleaned;
            for (size_t i = 0; i < result.size(); ++i) {
                if (result[i] == ')') {
                    size_t j = cleaned.size();
                    while (j > 0 && cleaned[j-1] == ' ') --j;
                    cleaned.erase(j, cleaned.size() - j);
                }
                cleaned += result[i];
            }
            cleaned += ")";
            return cleaned;
        }
        case ASTNode::NodeType::ConditionalExpr: {
            auto* cond = static_cast<ConditionalExpression*>(expr);
            return format_expression(cond->condition.get()) + " ? " + format_expression(cond->true_expr.get()) + " : " + format_expression(cond->false_expr.get());
        }
        case ASTNode::NodeType::ValofExpr: {
            auto* v = static_cast<ValofExpression*>(expr);
            std::stringstream ss;
            ss << "VALOF ";
            int prev_indent = indent_level_;
            indent();
            if (v->body) v->body->accept(*this);
            dedent();
            return ss.str();
        }
        case ASTNode::NodeType::FloatValofExpr: {
            auto* v = static_cast<FloatValofExpression*>(expr);
            std::stringstream ss;
            ss << "FLOATVALOF ";
            int prev_indent = indent_level_;
            indent();
            if (v->body) v->body->accept(*this);
            dedent();
            return ss.str();
        }
        case ASTNode::NodeType::VecAllocationExpr: {
            auto* v = static_cast<VecAllocationExpression*>(expr);
            return "VEC " + format_expression(v->size_expr.get());
        }
        case ASTNode::NodeType::StringAllocationExpr: {
            auto* v = static_cast<StringAllocationExpression*>(expr);
            return "STRING " + format_expression(v->size_expr.get());
        }
        case ASTNode::NodeType::TableExpr: {
            auto* t = static_cast<TableExpression*>(expr);
            std::string result = "TABLE(";
            for (size_t i = 0; i < t->initializers.size(); ++i) {
                result += format_expression(t->initializers[i].get());
                if (i < t->initializers.size() - 1) result += ", ";
            }
            result += ")";
            return result;
        }
        default:
            return "[expr]";
    }
}

// --- Visitor Overrides ---

void CodeFormatter::visit(Program& node) {
    for (size_t i = 0; i < node.declarations.size(); ++i) {
        node.declarations[i]->accept(*this);
        if (i < node.declarations.size() - 1) write_line();
    }
    if (!node.statements.empty()) {
        write_line();
        for (size_t i = 0; i < node.statements.size(); ++i) {
            node.statements[i]->accept(*this);
            if (i < node.statements.size() - 1) write_line();
        }
    }
}

// --- Declarations ---
void CodeFormatter::visit(LetDeclaration& node) {
    write_indent();
    write(uppercase(node.is_float_declaration ? "FLET" : "LET") + " ");
    for (size_t i = 0; i < node.names.size(); ++i) {
        write(node.names[i]);
        if (i < node.names.size() - 1) write(", ");
    }
    if (!node.initializers.empty()) {
        write(" = ");
        for (size_t i = 0; i < node.initializers.size(); ++i) {
            write(format_expression(node.initializers[i].get()));
            if (i < node.initializers.size() - 1) write(", ");
        }
    }
    write_line();
}

void CodeFormatter::visit(ManifestDeclaration& node) {
    write_indent();
    write_line("MANIFEST " + node.name + " = " + std::to_string(node.value));
}

void CodeFormatter::visit(StaticDeclaration& node) {
    write_indent();
    write("STATIC " + node.name);
    if (node.initializer) {
        write(" = " + format_expression(node.initializer.get()));
    }
    write_line();
}

void CodeFormatter::visit(GlobalDeclaration& node) {
    write_indent();
    write("GLOBAL ");
    for (size_t i = 0; i < node.globals.size(); ++i) {
        write(node.globals[i].first);
        if (node.globals[i].second != 0) {
            write(" = " + std::to_string(node.globals[i].second));
        }
        if (i < node.globals.size() - 1) write(", ");
    }
    write_line();
}

void CodeFormatter::visit(GlobalVariableDeclaration& node) {
    write_indent();
    write(uppercase(node.is_float_declaration ? "FLET" : "LET") + " ");
    for (size_t i = 0; i < node.names.size(); ++i) {
        write(node.names[i]);
        if (i < node.names.size() - 1) write(", ");
    }
    if (!node.initializers.empty()) {
        write(" = ");
        for (size_t i = 0; i < node.initializers.size(); ++i) {
            write(format_expression(node.initializers[i].get()));
            if (i < node.initializers.size() - 1) write(", ");
        }
    }
    write_line();
}

void CodeFormatter::visit(FunctionDeclaration& node) {
    write_indent();
    write(uppercase("LET") + " " + node.name + "(");
    for (size_t i = 0; i < node.parameters.size(); ++i) {
        write(node.parameters[i]);
        if (i < node.parameters.size() - 1) write(", ");
    }
    write(")");
    if (node.body) {
        write_line(" BE");
        indent();
        write_line("$(");
        indent();
        if (node.body) {
            if (node.body->getType() == ASTNode::NodeType::ValofExpr) {
                auto* valof_expr = static_cast<ValofExpression*>(node.body.get());
                if (valof_expr->body) {
                    valof_expr->body->accept(*this);
                }
            } else if (node.body->getType() == ASTNode::NodeType::CompoundStmt) {
                // Removed invalid static_cast as node.body is not a CompoundStatement
            } else {
                node.body->accept(*this);
            }
        }
        dedent();
        write_line("$)");
        dedent();
    } else {
        write_line();
    }
}

void CodeFormatter::visit(RoutineDeclaration& node) {
    write_indent();
    write(uppercase("ROUTINE") + " " + uppercase(node.name) + "(");
    for (size_t i = 0; i < node.parameters.size(); ++i) {
        write(node.parameters[i]);
        if (i < node.parameters.size() - 1) write(", ");
    }
    write(") BE");
    write_line();
    write_line("$(");
    indent();
    if (node.body) node.body->accept(*this);
    dedent();
    write_indent();
    write_line("$)");
}

void CodeFormatter::visit(LabelDeclaration& node) {
    write_indent();
    write_line(node.name + ":");
    if (node.command) {
        indent();
        node.command->accept(*this);
        dedent();
    }
}

// --- Statements ---
void CodeFormatter::visit(BlockStatement& node) {
    write_line("$(");
    indent();
    for (const auto& decl : node.declarations) {
        write_indent();
        decl->accept(*this);
    }
    for (const auto& stmt : node.statements) {
        write_indent();
        stmt->accept(*this);
    }
    dedent();
    write_line("$)");
}

void CodeFormatter::visit(AssignmentStatement& node) {
    std::string lhs_str;
    for (size_t i = 0; i < node.lhs.size(); ++i) {
        lhs_str += format_expression(node.lhs[i].get());
        if (i < node.lhs.size() - 1) lhs_str += ", ";
    }
    std::string rhs_str;
    for (size_t i = 0; i < node.rhs.size(); ++i) {
        rhs_str += format_expression(node.rhs[i].get());
        if (i < node.rhs.size() - 1) rhs_str += ", ";
    }
    write_indent();
    write(lhs_str + " := " + rhs_str);
    write_line();
}

void CodeFormatter::visit(RoutineCallStatement& node) {
    write_indent();
    write(format_expression(node.routine_expr.get()) + "(");
    for (size_t i = 0; i < node.arguments.size(); ++i) {
        write(format_expression(node.arguments[i].get()));
        if (i < node.arguments.size() - 1) write(", ");
    }
    write(")");
    write_line();
}

void CodeFormatter::visit(IfStatement& node) {
    write_indent();
    write(uppercase("IF") + " " + format_expression(node.condition.get()) + " " + uppercase("THEN"));
    write_line();
    indent();
    if (node.then_branch) node.then_branch->accept(*this);
    dedent();
}

void CodeFormatter::visit(UnlessStatement& node) {
    write_indent();
    write(uppercase("UNLESS") + " " + format_expression(node.condition.get()) + " " + uppercase("THEN"));
    write_line();
    indent();
    if (node.then_branch) node.then_branch->accept(*this);
    dedent();
}

void CodeFormatter::visit(TestStatement& node) {
    write_indent();
    write(uppercase("TEST") + " " + format_expression(node.condition.get()));
    write_line();
    indent();
    if (node.then_branch) node.then_branch->accept(*this);
    dedent();
    if (node.else_branch) {
        write_indent();
        write_line(uppercase("ELSE"));
        indent();
        node.else_branch->accept(*this);
        dedent();
    }
}

void CodeFormatter::visit(WhileStatement& node) {
    write_indent();
    write(uppercase("WHILE") + " " + format_expression(node.condition.get()) + " " + uppercase("DO"));
    write_line();
    indent();
    if (node.body) node.body->accept(*this);
    dedent();
}

void CodeFormatter::visit(UntilStatement& node) {
    write_indent();
    write(uppercase("UNTIL") + " " + format_expression(node.condition.get()) + " " + uppercase("DO"));
    write_line();
    indent();
    if (node.body) node.body->accept(*this);
    dedent();
}

void CodeFormatter::visit(RepeatStatement& node) {
    write_indent();
    std::string repeat_kw = "REPEAT";
    if (node.loop_type == RepeatStatement::LoopType::RepeatWhile) repeat_kw = "REPEATWHILE";
    else if (node.loop_type == RepeatStatement::LoopType::RepeatUntil) repeat_kw = "REPEATUNTIL";
    write(repeat_kw);
    write_line();
    indent();
    if (node.body) node.body->accept(*this);
    dedent();
    if (node.condition) {
        write_indent();
        write_line("WHILE " + format_expression(node.condition.get()));
    }
}

void CodeFormatter::visit(ForStatement& node) {
    write_indent();
    write(uppercase("FOR") + " " + node.loop_variable + " = " + format_expression(node.start_expr.get()));
    write(" " + uppercase("TO") + " " + format_expression(node.end_expr.get()));
    if (node.step_expr) {
        write(" " + uppercase("BY") + " " + format_expression(node.step_expr.get()));
    }
    write(" " + uppercase("DO"));
    write_line();
    write_line("$(");
    indent();
    if (node.body) node.body->accept(*this);
    dedent();
    write_line("$)");
}

void CodeFormatter::visit(SwitchonStatement& node) {
    write_indent();
    write_line("SWITCHON " + format_expression(node.expression.get()) + " INTO");
    indent();
    for (const auto& case_stmt : node.cases) {
        case_stmt->accept(*this);
    }
    if (node.default_case) {
        node.default_case->accept(*this);
    }
    dedent();
}

void CodeFormatter::visit(CaseStatement& node) {
    write_indent();
    write("CASE " + format_expression(node.constant_expr.get()) + ": ");
    if (node.command) {
        node.command->accept(*this);
    } else {
        write_line();
    }
}

void CodeFormatter::visit(DefaultStatement& node) {
    write_indent();
    write("DEFAULT: ");
    if (node.command) {
        node.command->accept(*this);
    } else {
        write_line();
    }
}

void CodeFormatter::visit(GotoStatement& node) {
    write_indent();
    write_line("GOTO " + format_expression(node.label_expr.get()));
}

void CodeFormatter::visit(ReturnStatement& node) {
    write_indent();
    write_line("RETURN");
}

void CodeFormatter::visit(FinishStatement& node) {
    write_indent();
    write("FINISH");
    if (node.syscall_number) {
        write(" " + format_expression(node.syscall_number.get()));
    }
    if (!node.arguments.empty()) {
        write(" (");
        for (size_t i = 0; i < node.arguments.size(); ++i) {
            write(format_expression(node.arguments[i].get()));
            if (i < node.arguments.size() - 1) write(", ");
        }
        write(")");
    }
    write_line();
}

void CodeFormatter::visit(BreakStatement&) {
    write_indent();
    write_line("BREAK");
}

void CodeFormatter::visit(BrkStatement&) {
    write_indent();
    write_line("BRK");
}

void CodeFormatter::visit(LoopStatement&) {
    write_indent();
    write_line("LOOP");
}

void CodeFormatter::visit(EndcaseStatement&) {
    write_indent();
    write_line("ENDCASE");
}

void CodeFormatter::visit(ResultisStatement& node) {
    write_indent();
    write(uppercase("RESULTIS") + " " + format_expression(node.expression.get()));
    write_line();
}

void CodeFormatter::visit(CompoundStatement& node) {
    for (const auto& stmt : node.statements) {
        write_indent();
        stmt->accept(*this);
    }
}

void CodeFormatter::visit(StringStatement& node) {
    write_indent();
    write_line("STRING " + format_expression(node.size_expr.get()));
}

void CodeFormatter::visit(FreeStatement& node) {
    write_indent();
    write_line("FREE " + format_expression(node.expression_.get()));
}

void CodeFormatter::visit(LabelTargetStatement& node) {
    write_indent();
    write_line(node.labelName + ":");
}

void CodeFormatter::visit(ConditionalBranchStatement& node) {
    write_indent();
    write_line("IF " + format_expression(node.condition_expr.get()) + " GOTO " + node.targetLabel);
}

// --- Expressions (for completeness, but handled in format_expression) ---
void CodeFormatter::visit(NumberLiteral&) {}
void CodeFormatter::visit(StringLiteral&) {}
void CodeFormatter::visit(CharLiteral&) {}
void CodeFormatter::visit(BooleanLiteral&) {}
void CodeFormatter::visit(VariableAccess&) {}
void CodeFormatter::visit(BinaryOp&) {}
void CodeFormatter::visit(UnaryOp&) {}
void CodeFormatter::visit(VectorAccess&) {}
void CodeFormatter::visit(CharIndirection&) {}
void CodeFormatter::visit(FloatVectorIndirection&) {}
void CodeFormatter::visit(FunctionCall&) {}
void CodeFormatter::visit(SysCall&) {}

void CodeFormatter::visit(ValofExpression& node) {
    write(uppercase("VALOF"));
    write_line(); // VALOF is followed by a new line and an indented block
// Removed duplicate and incomplete method definition
    if (node.body) {
        node.body->accept(*this);
    }
}

void CodeFormatter::visit(ConditionalExpression& node) {
    write(uppercase("IF") + " " + format_expression(node.condition.get()) + " " + uppercase("THEN"));
    write_line();
    indent();
    if (node.true_expr) {
        write(format_expression(node.true_expr.get()));
    }
    dedent();
    if (node.false_expr) {
        write_indent();
        write_line(uppercase("ELSE"));
        indent();
        write(format_expression(node.false_expr.get()));
        dedent();
    }
}
void CodeFormatter::visit(FloatValofExpression& node) {
    write(uppercase("FLOATVALOF"));
    write_line(); // FLOATVALOF is followed by a new line and an indented block
    if (node.body) {
        node.body->accept(*this);
    }
}

void CodeFormatter::visit(VecAllocationExpression&) {}
void CodeFormatter::visit(StringAllocationExpression&) {}
void CodeFormatter::visit(TableExpression&) {}