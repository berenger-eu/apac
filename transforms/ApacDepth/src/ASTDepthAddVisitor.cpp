
#include "ASTDepthAddVisitor.hpp"

bool ASTDepthAddVisitor::lookForRecCall(Stmt *st) {
  std::stack<Stmt *> stack;
  bool foundRecCall = false;
  stack.push(st);

  while (!(foundRecCall || stack.empty())) {
    Stmt *current = stack.top();
    stack.pop();
    // If it is a recursive call, then we need to add the depth incrementation
    if (isa<CallExpr>(current)) {
      auto currentCall = cast<CallExpr>(current);
      if (currentCall->getDirectCallee() != nullptr &&
          currentCall->getDirectCallee()->getNameAsString() ==
              functionsToModify.back()->getNameAsString())
        foundRecCall = true;
    } else {
      for (auto child : current->children()) {
        stack.push(child);
      }
    }
  }
  return foundRecCall;
}
void ASTDepthAddVisitor::handleStmt(Stmt *st) {
  if (st == nullptr)
    return;
  // Special cases in the ifs (such as ifs, fors, etc), because the recursive
  // call can be placed at different places in the tree If it's in the if
  // condition, the increment is before the if, if it's in the if itself, then
  // increment is in it too etc
  if (isa<IfStmt>(st)) {
    traverseTree(cast<IfStmt>(st)->getThen());
    traverseTree(cast<IfStmt>(st)->getElse());
    // Look for calls in ifs
    // Then traverse tree of if and else
  } else {
    if (lookForRecCall(st))
      recursiveCallsStatements.push_back(st);
  }
}