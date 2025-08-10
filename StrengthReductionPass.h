#ifndef STRENGTH_REDUCTION_PASS_H
#define STRENGTH_REDUCTION_PASS_H

#include "ASTVisitor.h"
#include "AST.h"
#include "DataTypes.h"
#include <cmath>
#include <iostream>
#include <memory> // Required for std::unique_ptr

class StrengthReductionPass : public ASTVisitor {
public:
    explicit StrengthReductionPass(bool trace = false) : trace_enabled_(trace) {}

    void run(Program& ast) {
        ast.accept(*this);
    }

    // Main visitor method for binary operations. It now delegates to helper functions.
    void visit(BinaryOp& node) override {
        // First, visit children to optimize nested expressions from the bottom up.
        if (node.left) node.left->accept(*this);
        if (node.right) node.right->accept(*this);

        // Attempt to apply optimizations one by one.
        if (tryOptimizeIntegerMultiply(node)) return;
        if (tryOptimizeFloatMultiply(node)) return;
        if (tryOptimizeIntegerDivision(node)) return;
        if (tryOptimizeFloatDivision(node)) return;
    }

private:
    bool trace_enabled_;

    // --- Optimization Helpers ---

    /**
     * @brief Optimizes integer multiplication by a power of two into a left shift.
     * e.g., `x * 8` becomes `x << 3`. This is a commutative operation.
     */
    bool tryOptimizeIntegerMultiply(BinaryOp& node) {
        if (node.op != BinaryOp::Operator::Multiply) return false;

        NumberLiteral* literal = nullptr;
        std::unique_ptr<Expression>* other_operand_ptr = nullptr;

        // Find which operand is the constant power of two.
        if (auto* r_lit = dynamic_cast<NumberLiteral*>(node.right.get())) {
            literal = r_lit;
            other_operand_ptr = &node.left;
        } else if (auto* l_lit = dynamic_cast<NumberLiteral*>(node.left.get())) {
            literal = l_lit;
            other_operand_ptr = &node.right;
        } else {
            return false; // No literal operand.
        }

        if (literal->literal_type != NumberLiteral::LiteralType::Integer) return false;

        long long value = literal->int_value;

        // FIX: Efficiently check for power of two and get the shift amount using bitwise operations,
        // which avoids floating-point math (`log2`) on integers.
        if (value > 0 && (value & (value - 1)) == 0) { // isPowerOfTwo
            long long shift_amount = 0;
            if (value > 0) {
                unsigned long long temp_val = value;
                while ((temp_val & 1) == 0) {
                    temp_val >>= 1;
                    shift_amount++;
                }
            }

            // Transform the node.
            node.op = BinaryOp::Operator::LeftShift;
            // FIX: Safely move the non-literal operand to the left side for the shift operation.
            node.left = std::move(*other_operand_ptr);
            node.right = std::make_unique<NumberLiteral>(shift_amount);

            if (trace_enabled_) {
                std::cout << "Optimized: Reduced integer multiplication by " << value << " to left shift by " << shift_amount << "." << std::endl;
            }
            return true;
        }
        return false;
    }

    /**
     * @brief Optimizes floating-point multiplication by 2.0 into an addition.
     * e.g., `expr * 2.0` becomes `expr + expr`. This is a commutative operation.
     */
    bool tryOptimizeFloatMultiply(BinaryOp& node) {
        if (node.op != BinaryOp::Operator::FloatMultiply) return false;

        NumberLiteral* literal = nullptr;
        std::unique_ptr<Expression>* other_operand_ptr = nullptr;

        if (auto* r_lit = dynamic_cast<NumberLiteral*>(node.right.get())) {
            literal = r_lit;
            other_operand_ptr = &node.left;
        } else if (auto* l_lit = dynamic_cast<NumberLiteral*>(node.left.get())) {
            literal = l_lit;
            other_operand_ptr = &node.right;
        } else {
            return false; // No literal operand.
        }

        if (literal->literal_type != NumberLiteral::LiteralType::Float || literal->float_value != 2.0) {
            return false;
        }

        // FIX: Generalized the optimization. It now works for ANY expression, not just a fragile
        // and incorrect check for a specific AST structure like `FLOAT(...)`.
        node.op = BinaryOp::Operator::FloatAdd;
        node.left = std::move(*other_operand_ptr);
        // FIX: Use static_cast to convert the pointer to the correct unique_ptr type.
        node.right = std::unique_ptr<Expression>(static_cast<Expression*>(node.left->clone().release()));

        if (trace_enabled_) {
            std::cout << "Optimized: Reduced float multiplication by 2.0 to a self-addition." << std::endl;
        }
        return true;
    }

    /**
     * @brief Optimizes integer division by a power of two into a right shift.
     * e.g., `x / 4` becomes `x >> 2`. This is NOT commutative.
     */
    bool tryOptimizeIntegerDivision(BinaryOp& node) {
        if (node.op != BinaryOp::Operator::Divide) return false;

        // FIX: Division is not commutative. The optimization is only valid if the literal
        // is the divisor (on the right-hand side).
        auto* literal = dynamic_cast<NumberLiteral*>(node.right.get());
        if (!literal || literal->literal_type != NumberLiteral::LiteralType::Integer) {
            return false;
        }

        long long value = literal->int_value;
        if (value > 0 && (value & (value - 1)) == 0) { // isPowerOfTwo
            long long shift_amount = 0;
            if (value > 0) {
                unsigned long long temp_val = value;
                while ((temp_val & 1) == 0) {
                    temp_val >>= 1;
                    shift_amount++;
                }
            }

            // Transform the node.
            node.op = BinaryOp::Operator::RightShift;
            // The left operand (the dividend) is already in the correct place.
            node.right = std::make_unique<NumberLiteral>(shift_amount);

            if (trace_enabled_) {
                std::cout << "Optimized: Reduced integer division by " << value << " to right shift by " << shift_amount << "." << std::endl;
            }
            return true;
        }
        return false;
    }

    /**
     * @brief Optimizes floating-point division by a constant into multiplication by its reciprocal.
     * e.g., `x / 2.0` becomes `x * 0.5`. This is NOT commutative.
     */
    bool tryOptimizeFloatDivision(BinaryOp& node) {
        if (node.op != BinaryOp::Operator::FloatDivide) return false;

        // FIX: Division is not commutative. The optimization is only valid if the literal
        // is the divisor (on the right-hand side).
        auto* literal = dynamic_cast<NumberLiteral*>(node.right.get());
        if (!literal || literal->literal_type != NumberLiteral::LiteralType::Float) {
            return false;
        }

        double value = literal->float_value;
        if (value != 0.0) { // Avoid division by zero.
            // Transform the node.
            node.op = BinaryOp::Operator::FloatMultiply;
            // The left operand (the dividend) is already in the correct place.
            node.right = std::make_unique<NumberLiteral>(1.0 / value);

            if (trace_enabled_) {
                std::cout << "Optimized: Reduced float division to multiplication by reciprocal." << std::endl;
            }
            return true;
        }
        return false;
    }
};

#endif // STRENGTH_REDUCTION_PASS_H
