#include "SymbolTableBuilder.h"
#include <iostream>



SymbolTableBuilder::SymbolTableBuilder(bool enable_tracing)
    : enable_tracing_(enable_tracing) {
    // Create a new, empty symbol table
    symbol_table_ = std::make_unique<SymbolTable>();
}

std::unique_ptr<SymbolTable> SymbolTableBuilder::build(Program& program) {
    trace("Building symbol table...");
    
    // Visit the AST to populate the symbol table
    visit(program);
    
    if (enable_tracing_) {
        trace("Symbol table construction complete. Symbol table contents:");
        symbol_table_->dumpTable();
    }
    
    // Return the populated symbol table
    return std::move(symbol_table_);
}

void SymbolTableBuilder::visit(Program& node) {
    trace("Entering global scope");
    
    // Process all declarations first to ensure forward references work
    for (auto& decl : node.declarations) {
        if (decl) decl->accept(*this);
    }
    
    // Then process statements
    for (auto& stmt : node.statements) {
        if (stmt) stmt->accept(*this);
    }
}

void SymbolTableBuilder::visit(LetDeclaration& node) {
    trace("Processing let declaration");
    
    // For each variable in the declaration
    for (size_t i = 0; i < node.names.size(); i++) {
        const std::string& name = node.names[i];
        
        // Determine type (let is for integers, flet is for floats)
        VarType type = node.is_float_declaration ? VarType::FLOAT : VarType::INTEGER;
        
        // Add the variable to the current scope
        if (!symbol_table_->addSymbol(name, SymbolKind::LOCAL_VAR, type)) {
            report_duplicate_symbol(name);
        } else {
            trace("Added local variable: " + name + 
                  (node.is_float_declaration ? " (float)" : " (int)"));
        }
    }
}

void SymbolTableBuilder::visit(ManifestDeclaration& node) {
    trace("Processing manifest declaration: " + node.name);
    
    // Add the manifest constant to the symbol table
    if (!symbol_table_->addSymbol(node.name, SymbolKind::MANIFEST, VarType::INTEGER)) {
        report_duplicate_symbol(node.name);
    } else {
        // Set the absolute value for the manifest constant
        symbol_table_->setSymbolAbsoluteValue(node.name, node.value);
        trace("Added manifest constant: " + node.name + " = " + std::to_string(node.value));
    }
}

void SymbolTableBuilder::visit(StaticDeclaration& node) {
    trace("Processing static declaration: " + node.name);
    
    // Add the static variable to the symbol table (at global scope)
    // Even though static variables have local scope visibility, they live in global storage
    
    // Determine the type (default to INTEGER)
    VarType type = VarType::INTEGER;
    
    // Try to infer from the initializer if it's a float literal
    if (node.initializer && node.initializer->getType() == ASTNode::NodeType::NumberLit) {
        auto* num_lit = static_cast<NumberLiteral*>(node.initializer.get());
        if (num_lit->literal_type == NumberLiteral::LiteralType::Float) {
            type = VarType::FLOAT;
        }
    }
    
    if (!symbol_table_->addSymbol(node.name, SymbolKind::STATIC_VAR, type)) {
        report_duplicate_symbol(node.name);
    } else {
        trace("Added static variable: " + node.name);
    }
}

void SymbolTableBuilder::visit(GlobalDeclaration& node) {
    trace("Processing global declaration");
    
    // Add each global to the symbol table
    for (auto& global : node.globals) {
        const std::string& name = global.first;
        VarType type = VarType::INTEGER; // Default to INTEGER type
        
        if (!symbol_table_->addSymbol(name, SymbolKind::GLOBAL_VAR, type)) {
            report_duplicate_symbol(name);
        } else {
            trace("Added global variable: " + name);
        }
    }
}

void SymbolTableBuilder::visit(GlobalVariableDeclaration& node) {
    trace("Processing global variable declaration");
    
    // Process each global variable
    for (size_t i = 0; i < node.names.size(); i++) {
        const std::string& name = node.names[i];
        
        // Determine type based on declaration
        VarType type = node.is_float_declaration ? VarType::FLOAT : VarType::INTEGER;
        
        if (!symbol_table_->addSymbol(name, SymbolKind::GLOBAL_VAR, type)) {
            report_duplicate_symbol(name);
        } else {
            trace("Added global variable: " + name + 
                  (node.is_float_declaration ? " (float)" : " (int)"));
        }
    }
}

void SymbolTableBuilder::visit(FunctionDeclaration& node) {
    trace("Processing function declaration: " + node.name);
    
    // Add function to the symbol table
    SymbolKind kind = SymbolKind::FUNCTION;
    
    // --- CORRECTED LOGIC ---
    // Determine the function's return type by inspecting its body.
    VarType return_type = VarType::INTEGER; // Default to INTEGER
    if (node.body && dynamic_cast<FloatValofExpression*>(node.body.get())) {
        // If the body is a FloatValofExpression, it returns a float.
        return_type = VarType::FLOAT;
        kind = SymbolKind::FLOAT_FUNCTION; // Use a more specific kind for clarity
    }
    // --- END CORRECTION ---
    
    // Add the function to the symbol table with the correct return type.
    if (!symbol_table_->addSymbol(node.name, kind, return_type)) {
        report_duplicate_symbol(node.name);
    } else {
        trace("Added function: " + node.name + (return_type == VarType::FLOAT ? " (float)" : " (int)"));
    }
    
    // Enter a new scope for the function body
    symbol_table_->enterScope();
    // Pre-allocate _temp0 through _temp3 as ANY
    for (int i = 0; i < 4; ++i) {
        symbol_table_->addSymbol("_temp" + std::to_string(i), SymbolKind::LOCAL_VAR, VarType::ANY);
    }
    symbol_table_->setCurrentFunction(node.name);
    
    // Add parameters to the function's scope
    for (const auto& param : node.parameters) {
        // Default to INTEGER parameters
        VarType param_type = VarType::INTEGER;
        symbol_table_->addParameter(node.name, param, param_type);
        trace("Added parameter: " + param);
    }
    
    // Visit the function body
    if (node.body) {
        node.body->accept(*this);
    }
    
    // Exit the function scope
    symbol_table_->setCurrentFunction("");
    symbol_table_->exitScope();
}

void SymbolTableBuilder::visit(RoutineDeclaration& node) {
    trace("Processing routine declaration: " + node.name);
    
    // Add routine to the symbol table
    if (!symbol_table_->addSymbol(node.name, SymbolKind::ROUTINE, VarType::INTEGER)) {
        report_duplicate_symbol(node.name);
    } else {
        trace("Added routine: " + node.name);
    }
    
    // Enter a new scope for the routine body
    symbol_table_->enterScope();
    // Pre-allocate _temp0 through _temp3 as ANY
    for (int i = 0; i < 4; ++i) {
        symbol_table_->addSymbol("_temp" + std::to_string(i), SymbolKind::LOCAL_VAR, VarType::ANY);
    }
    symbol_table_->setCurrentFunction(node.name);
    
    // Add parameters to the routine's scope
    for (const auto& param : node.parameters) {
        // Default to INTEGER parameters
        VarType param_type = VarType::INTEGER;
        symbol_table_->addParameter(node.name, param, param_type);
        trace("Added parameter: " + param);
    }
    
    // Visit the routine body
    if (node.body) {
        node.body->accept(*this);
    }
    
    // Exit the routine scope
    symbol_table_->setCurrentFunction("");
    symbol_table_->exitScope();
}

void SymbolTableBuilder::visit(LabelDeclaration& node) {
    trace("Processing label declaration: " + node.name);
    
    // Add label to the symbol table
    if (!symbol_table_->addSymbol(node.name, SymbolKind::LABEL, VarType::INTEGER)) {
        report_duplicate_symbol(node.name);
    } else {
        // Set the label location
        symbol_table_->setSymbolAbsoluteValue(node.name, 0); // Address will be resolved later
        trace("Added label: " + node.name);
    }
    
    // Visit the label's command
    if (node.command) {
        node.command->accept(*this);
    }
}

void SymbolTableBuilder::visit(BlockStatement& node) {
    trace("Entering block scope");
    
    // Enter a new scope for the block
    symbol_table_->enterScope();
    
    // Process declarations in the block
    for (auto& decl : node.declarations) {
        if (decl) decl->accept(*this);
    }
    
    // Process statements in the block
    for (auto& stmt : node.statements) {
        if (stmt) stmt->accept(*this);
    }
    
    // Exit the block scope
    symbol_table_->exitScope();
    
    trace("Exited block scope");
}

void SymbolTableBuilder::visit(ForStatement& node) {
    trace("Processing for statement with loop variable: " + node.loop_variable);
    
    // Add the loop variable to the current scope
    if (!symbol_table_->addSymbol(node.loop_variable, SymbolKind::LOCAL_VAR, VarType::INTEGER)) {
        report_duplicate_symbol(node.loop_variable);
    } else {
        trace("Added for-loop variable: " + node.loop_variable);
    }
    
    // Visit the for-loop body
    if (node.body) {
        node.body->accept(*this);
    }
}

void SymbolTableBuilder::visit(FloatValofExpression& node) {
    trace("Entering FloatValofExpression block");
    
    // Enter a new scope for the valof block
    symbol_table_->enterScope();
    
    // Visit the valof body
    if (node.body) {
        node.body->accept(*this);
    }
    
    // Exit the valof scope
    symbol_table_->exitScope();
    
    trace("Exited FloatValofExpression block");
}

void SymbolTableBuilder::visit(ValofExpression& node) {
    trace("Entering ValofExpression block");
    
    // Enter a new scope for the valof block
    symbol_table_->enterScope();
    
    // Visit the valof body
    if (node.body) {
        node.body->accept(*this);
    }
    
    // Exit the valof scope
    symbol_table_->exitScope();
    
    trace("Exited ValofExpression block");
}

void SymbolTableBuilder::trace(const std::string& message) const {
    if (enable_tracing_) {
        std::cout << "[SymbolTableBuilder] " << message << std::endl;
    }
}

void SymbolTableBuilder::report_duplicate_symbol(const std::string& name) const {
    std::cerr << "Warning: Duplicate symbol declaration: " << name << std::endl;
}

