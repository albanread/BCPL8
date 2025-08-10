#include "ASTAnalyzer.h"

// Implements ASTAnalyzer::visit for ManifestDeclaration nodes.
// Manifest declarations are now handled and removed by ManifestResolutionPass.
// No action needed here.
void ASTAnalyzer::visit(ManifestDeclaration& node) {
    // Intentionally left blank.
}