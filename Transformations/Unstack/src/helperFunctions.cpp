#include "helperFunctions.hpp"
// Returns all top CallExpr within an Expr
void findTopCallsInExpr(Expr *ex, std::vector<CallExpr *> &callExprVect) {
  if (ex != nullptr) {
    std::stack<Expr *> exprStack;
    exprStack.push(ex);
    while (!exprStack.empty()) {
      Expr *currentExpr = exprStack.top();
      exprStack.pop();
      if (isa<CallExpr>(currentExpr)) {
        callExprVect.push_back(cast<CallExpr>(currentExpr));
      } else {
        for (auto it = currentExpr->child_begin();
             it != currentExpr->child_end(); it++) {
          if (isa<Expr>(*it)) {
            exprStack.push(cast<Expr>(*it));
          }
        }
      }
    }
  }
}

void findAllCallExpr(CallExpr *calExp, std::vector<CallExpr *> &callExprVect) {
  if (calExp != nullptr) {
    std::stack<Expr *> exprStack;
    exprStack.push(calExp);
    while (!exprStack.empty()) {
      Expr *currentExpr = exprStack.top();
      exprStack.pop();
      if (isa<CallExpr>(currentExpr)) {
        callExprVect.push_back(cast<CallExpr>(currentExpr));
      }
      for (auto it = currentExpr->child_begin(); it != currentExpr->child_end();
           it++) {
        if (isa<Expr>(*it)) {
          exprStack.push(cast<Expr>(*it));
        }
      }
    }
  }
}
