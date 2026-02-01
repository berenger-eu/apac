#include "ASTUnstackVisitor.hpp"

void ASTUnstackVisitor::retrieveCalls(DeclStmt *declSt) {
  std::vector<CallExpr *> calls;
  if (declSt->isSingleDecl()) {
    VarDecl *varDec = cast<VarDecl>(declSt->getSingleDecl());
    if (varDec->hasInit()) {
      findTopCallsInExpr(varDec->getInit(), calls);
    }
  } else {
    for (auto it = declSt->decl_begin(); it != declSt->decl_end(); it++) {
      if (isa<VarDecl>(*it)) {
        VarDecl *varDec = cast<VarDecl>(*it);
        if (varDec->hasInit()) {
          findTopCallsInExpr(varDec->getInit(), calls);
        }
      }
    }
  }
  addCallsToUnstack(calls, declSt->getBeginLoc());
}

void ASTUnstackVisitor::retrieveCalls(CallExpr *calExpr) {
  addCallToUnstack(calExpr, calExpr->getBeginLoc());
}
void ASTUnstackVisitor::retrieveCalls(Expr *expr) {
  std::vector<CallExpr *> calls;
  findTopCallsInExpr(expr, calls);
  addCallsToUnstack(calls, expr->getBeginLoc());
}