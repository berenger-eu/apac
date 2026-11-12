#include "ASTDepthAddVisitor.hpp"

void ASTDepthAddVisitor::handleStmt(Stmt *statement) {
  if (!statement)
    return;
  // Visit each recursive call, including calls in conditions and nested
  // arguments. Guard the expression itself rather than its enclosing statement:
  // declarations must stay visible, and sibling calls must not inherit depth.
  for (auto *child : statement->children())
    handleStmt(child);
  if (auto *call = dyn_cast<CallExpr>(statement))
    if (auto *callee = call->getDirectCallee())
      if (callee->getCanonicalDecl() ==
          functionsToModify.back()->getCanonicalDecl())
        recursiveCallsStatements.push_back(call);
}
