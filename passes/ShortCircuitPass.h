#ifndef SHORT_CIRCUIT_PASS_H
#define SHORT_CIRCUIT_PASS_H

#include "../Optimizer.h"
#include "../AST.h"

class ShortCircuitPass : public Optimizer {
public:
    // Inherit the constructor from the Optimizer base class.
    using Optimizer::Optimizer;

    // The main entry point for applying the transformation.
    ProgramPtr apply(ProgramPtr program) override;
    
    // Implement the required getName method from the Optimizer base class
    std::string getName() const override { return "ShortCircuitPass"; }

private:
    // Override the visitor for binary operations, which is where the
    // logical AND and OR operators will be transformed.
    void visit(BinaryOp& node) override;
};

#endif // SHORT_CIRCUIT_PASS_H