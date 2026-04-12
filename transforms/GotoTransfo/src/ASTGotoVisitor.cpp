#include "ASTGotoVisitor.hpp"
#include "clang/Lex/Lexer.h"
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
        lastFuncStmt != returnsList[0].first)) {

    TheRewriter.InsertTextAfterToken(fDecl->getBody()->getBeginLoc(),
                                     SSprint.str());
    SSexit << "\n}\n";
    if (!fDecl->getReturnType().getTypePtr()->isVoidType()) {
      SSexit << "__exit" << functionsCounter << ":;\n return *__result;\n";
    } else {
      SSexit << "__exit" << functionsCounter << ":;\n";
    }
    TheRewriter.InsertTextAfter(fDecl->getBody()->getEndLoc(), SSexit.str());
    for (auto &retStmtInfo : returnsList) {
      if (retStmtInfo.second) {
        auto semiEndLoc = Lexer::findLocationAfterToken(
            retStmtInfo.first->getEndLoc(), tok::semi,
            TheRewriter.getSourceMgr(), TheRewriter.getLangOpts(), true);
        if (semiEndLoc.isValid()) {
          TheRewriter.ReplaceText(
              CharSourceRange::getCharRange(retStmtInfo.first->getBeginLoc(),
                                            semiEndLoc),
              createGotoString(*retStmtInfo.first, TheRewriter,
                               functionsCounter, true));
        } else {
          TheRewriter.ReplaceText(
              SourceRange(retStmtInfo.first->getBeginLoc(),
                          retStmtInfo.first->getEndLoc()),
              createGotoString(*retStmtInfo.first, TheRewriter,
                               functionsCounter, true));
        }
        continue;
      }

      TheRewriter.ReplaceText(
          SourceRange(retStmtInfo.first->getBeginLoc(),
                      retStmtInfo.first->getEndLoc()),
          createGotoString(*retStmtInfo.first, TheRewriter, functionsCounter,
                           false));
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

void ASTGotoVisitor::subVisitReturnStmt(ReturnStmt *retStmt,
                                        bool forceBraces) {
  // Insert a Goto and affect the value of the return to result
  forceBraces = forceBraces && (retStmt->getRetValue() != nullptr);
  returnsList.push_back(std::make_pair(retStmt, forceBraces));
}

std::string createGotoString(const ReturnStmt &retStmt,
                             const Rewriter &TheRewriter,
                             const unsigned int &exitCounter,
                             bool wrapInBraces) {
  std::stringstream SSprint;
  if (wrapInBraces) {
    SSprint << "{\n";
  }
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
  if (wrapInBraces) {
    SSprint << ";\n}";
  }
  return SSprint.str();
}