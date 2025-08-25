#include "Symbol.h"
#include <sstream>



std::string Symbol::to_string() const {
    std::ostringstream oss;
    
    // Symbol name and kind
    oss << "Symbol '" << name << "' (";
    
    switch (kind) {
        case SymbolKind::LOCAL_VAR:
            oss << "LOCAL_VAR";
            break;
        case SymbolKind::STATIC_VAR:
            oss << "STATIC_VAR";
            break;
        case SymbolKind::GLOBAL_VAR:
            oss << "GLOBAL_VAR";
            break;
        case SymbolKind::PARAMETER:
            oss << "PARAMETER";
            break;
        case SymbolKind::FUNCTION:
            oss << "FUNCTION";
            break;
        case SymbolKind::FLOAT_FUNCTION:
            oss << "FLOAT_FUNCTION";
            break;
        case SymbolKind::ROUTINE:
            oss << "ROUTINE";
            break;
        case SymbolKind::LABEL:
            oss << "LABEL";
            break;
        case SymbolKind::MANIFEST:
            oss << "MANIFEST";
            break;
        case SymbolKind::RUNTIME_FUNCTION:
            oss << "RUNTIME_FUNCTION";
            break;
        case SymbolKind::RUNTIME_FLOAT_FUNCTION:
            oss << "RUNTIME_FLOAT_FUNCTION";
            break;
        case SymbolKind::RUNTIME_ROUTINE:
            oss << "RUNTIME_ROUTINE";
            break;
        case SymbolKind::RUNTIME_FLOAT_ROUTINE:
            oss << "RUNTIME_FLOAT_ROUTINE";
            break;
    }
    
    // Add type information
    oss << ", ";
    switch (type) {
        case VarType::INTEGER:
            oss << "INTEGER";
            break;
        case VarType::FLOAT:
            oss << "FLOAT";
            break;
        case VarType::UNKNOWN:
            oss << "UNKNOWN";
            break;
        case VarType::ANY:
            oss << "ANY";
            break;
        case VarType::POINTER_TO_INT:
            oss << "POINTER_TO_INT";
            break;
        case VarType::POINTER_TO_FLOAT:
            oss << "POINTER_TO_FLOAT";
            break;
        case VarType::POINTER_TO_FLOAT_VEC:
            oss << "POINTER_TO_FLOAT_VEC";
            break;
        case VarType::POINTER_TO_INT_VEC:
            oss << "POINTER_TO_INT_VEC";
            break;
        case VarType::POINTER_TO_STRING:
            oss << "POINTER_TO_STRING";
            break;
        case VarType::POINTER_TO_TABLE:
            oss << "POINTER_TO_TABLE";
            break;
        case VarType::POINTER_TO_ANY_LIST:
            oss << "POINTER_TO_ANY_LIST";
            break;
        case VarType::POINTER_TO_INT_LIST:
            oss << "POINTER_TO_INT_LIST";
            break;
        case VarType::POINTER_TO_FLOAT_LIST:
            oss << "POINTER_TO_FLOAT_LIST";
            break;
        case VarType::POINTER_TO_LIST_NODE:
            oss << "POINTER_TO_LIST_NODE";
            break;
        case VarType::CONST_POINTER_TO_INT_LIST:
            oss << "CONST_POINTER_TO_INT_LIST";
            break;
        case VarType::CONST_POINTER_TO_FLOAT_LIST:
            oss << "CONST_POINTER_TO_FLOAT_LIST";
            break;
        case VarType::CONST_POINTER_TO_ANY_LIST:
            oss << "CONST_POINTER_TO_ANY_LIST";
            break;
        default:
            oss << "UNKNOWN_TYPE";
            break;
    }
    
    // Add scope level
    oss << ", scope=" << scope_level;
    
    // Add location information
    if (location.type != SymbolLocation::LocationType::UNKNOWN) {
        oss << ", location=";
        switch (location.type) {
            case SymbolLocation::LocationType::STACK:
                oss << "STACK[FP+" << location.stack_offset << "]";
                break;
            case SymbolLocation::LocationType::DATA:
                oss << "DATA[" << location.data_offset << "]";
                break;
            case SymbolLocation::LocationType::ABSOLUTE:
                oss << "ABSOLUTE[" << location.absolute_value << "]";
                break;
            case SymbolLocation::LocationType::LABEL:
                oss << "LABEL";
                break;
            case SymbolLocation::LocationType::UNKNOWN:
                oss << "UNKNOWN";
                break;
        }
    }
    
    // Add size information for arrays if available
    if (has_size) {
        oss << ", size=" << size;
    }
    
    oss << ")";
    return oss.str();
}

