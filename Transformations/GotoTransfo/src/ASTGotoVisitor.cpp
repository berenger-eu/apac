#include "ASTGotoVisitor.hpp"
using namespace clang;
bool ASTGotoVisitor::VisitFunctionDecl(FunctionDecl *fDecl) {
  if (!fDecl->isThisDeclarationADefinition())
    return true;
  // Declares the result in the transformed source file, only if it's not void
  std::stringstream SSprint;
  if (!fDecl->getReturnType().getTypePtr()->isVoidType()) {
    // Declares the result
    SSprint << "\nwrapper_t<"
            << fDecl->getReturnType().getAsString(TheRewriter.getLangOpts())
            << "> __result;\n";
  }
  SSprint << "{\n";

  // Adds the exit section
  std::stringstream SSexit;
  // TODO:Handle other cases with no returns

  Stmt *fDeclBody = fDecl->getBody();
  assert(fDeclBody && "Function body is null\n");
  if (fDeclBody && isa<CompoundStmt>(fDecl->getBody())) {
    subVisitCompoundStmt(cast<CompoundStmt>(fDeclBody));
  }
  auto returnsCounter = returnsList.size();
  auto funcChildren = fDeclBody->children();
  Stmt *lastFuncStmt = nullptr;
  for (auto &child : funcChildren) {
    lastFuncStmt = child;
  }
  if (returnsCounter > 1 ||
      (returnsCounter == 1 &&
       fDecl->getReturnType().getTypePtr()->isVoidType() &&
       lastFuncStmt != returnsList[0])) {

    TheRewriter.InsertTextAfterToken(fDecl->getBody()->getBeginLoc(),
                                     SSprint.str());
    SSexit << "\n}\n";
    if (!fDecl->getReturnType().getTypePtr()->isVoidType()) {
      SSexit << "__exit" << functionsCounter << ": return *__result;\n";
    } else {
      SSexit << "__exit" << functionsCounter << ":;\n";
    }
    TheRewriter.InsertTextAfter(fDecl->getBody()->getEndLoc(), SSexit.str());
    for (auto &retStmt : returnsList) {
      TheRewriter.ReplaceText(
          SourceRange(retStmt->getBeginLoc(), retStmt->getEndLoc()),
          createGotoString(*retStmt, TheRewriter, functionsCounter));
    }
    functionsCounter++;
  }

  returnsList.clear();
  return true;
}

void ASTGotoVisitor::subVisitCompoundStmt(CompoundStmt *coSt) {
  for (CompoundStmt::body_iterator b = coSt->body_begin(), e = coSt->body_end();
       b != e; ++b) {
    handleSubStmt(*b);
  }
}

void ASTGotoVisitor::handleSubStmt(Stmt *st) {
  if (st == NULL)
    ;
  else if (isa<ReturnStmt>(st)) {
    subVisitReturnStmt(cast<ReturnStmt>(st));
  } else if (isa<CompoundStmt>(st)) {
    subVisitCompoundStmt(cast<CompoundStmt>(st));
  } else if (isa<IfStmt>(st)) {
    subVisitIfStmt(cast<IfStmt>(st));
  } else if (isa<ForStmt>(st)) {
    subVisitForStmt(cast<ForStmt>(st));
  } else if (isa<WhileStmt>(st)) {
    subVisitWhileStmt(cast<WhileStmt>(st));
  }
}

void ASTGotoVisitor::subVisitReturnStmt(ReturnStmt *retStmt) {
  // Insert a Goto and affect the value of the return to result
  returnsList.push_back(retStmt);
}

std::string createGotoString(const ReturnStmt &retStmt,
                             const Rewriter &TheRewriter,
                             const unsigned int &exitCounter) {
  std::stringstream SSprint;
  // Replaces return with __result=build_wrapper<type>(); if there is a
  // return value
  if (retStmt.getRetValue()) {
    SSprint << "__result = build_wrapper<"
            << retStmt.getRetValue()->getType().getAsString(
                   TheRewriter.getLangOpts())
            << ">("
            << getExprAsString(retStmt.getRetValue(), TheRewriter.getLangOpts())
            << ");\n";
  }
  // goto __exitX;\n
  SSprint << "goto __exit" << exitCounter;
  return SSprint.str();
}