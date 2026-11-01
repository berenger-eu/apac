#include "ASTConditionUnstackVisitor.hpp"
#include "clang/Lex/Lexer.h"

namespace {
void closeScopeAfter(Rewriter &rewriter, Stmt *statement) {
  auto end = statement->getEndLoc();
  Token token;
  if (!Lexer::getRawToken(end, token, rewriter.getSourceMgr(),
                          rewriter.getLangOpts(), true) &&
      !token.is(tok::r_brace) && !token.is(tok::semi)) {
    auto afterSemi = Lexer::findLocationAfterToken(
        end, tok::semi, rewriter.getSourceMgr(), rewriter.getLangOpts(), false);
    if (afterSemi.isValid()) {
      rewriter.InsertTextBefore(afterSemi, "\n}\n");
      return;
    }
  }
  rewriter.InsertTextAfterToken(end, "\n}\n");
}
} // namespace

bool ASTConditionUnstackVisitor::VisitWhileStmt(WhileStmt *whileSt) {
  if (auto *var = whileSt->getConditionVariable()) {
    std::stringstream prefix;
    prefix << "while (true) {\n"
           << getCompleteVarDeclStr(var) << "if (!(" << var->getNameAsString()
           << ")) break;\n";
    TheRewriter.ReplaceText(
        SourceRange(whileSt->getBeginLoc(), whileSt->getRParenLoc()),
        prefix.str());
    closeScopeAfter(TheRewriter, whileSt->getBody());
  }
  return true;
}
bool ASTConditionUnstackVisitor::VisitIfStmt(IfStmt *ifSt) {
  if (ifSt->hasVarStorage() || ifSt->hasInitStorage()) {

    std::stringstream SSprint;
    SSprint << "{\n";
    if (ifSt->getInit() != NULL && isa<DeclStmt>(ifSt->getInit())) {
      DeclStmt *ifStInit = cast<DeclStmt>(ifSt->getInit());
      for (auto decl : ifStInit->decls()) {
        if (isa<VarDecl>(decl)) {
          auto varDecl = cast<VarDecl>(decl);
          SSprint << getCompleteVarDeclStr(varDecl);
        }
      }
      TheRewriter.RemoveText(
          SourceRange(ifStInit->getBeginLoc(), ifStInit->getEndLoc()));
    }
    if (ifSt->getConditionVariableDeclStmt() != NULL) {
      DeclStmt *ifStCond = ifSt->getConditionVariableDeclStmt();
      for (auto decl : ifStCond->decls()) {
        if (isa<VarDecl>(decl)) {
          auto varDecl = cast<VarDecl>(decl);
          SSprint << getCompleteVarDeclStr(varDecl);
          TheRewriter.ReplaceText(
              SourceRange(ifStCond->getBeginLoc(), ifStCond->getEndLoc()),
              (cast<VarDecl>(ifStCond->getSingleDecl()))->getNameAsString());
        }
      }
    }
    // Adds the built text section before the If statement
    TheRewriter.InsertTextAfter(ifSt->getBeginLoc(), SSprint.str());
    // We remove variables seen in the scope at the end of it
    // First declStmt contains variables,
    // add them to curVarEncoutered and curVarsInScope
    // UNHANDLED, either do it in the if BUT c++17 will be required
    // Or outside of if, but then the same has to be done for else if
    closeScopeAfter(TheRewriter, ifSt);
  }
  return true;
}

bool ASTConditionUnstackVisitor::VisitForStmt(ForStmt *forSt) {
  if (forSt->getInit() && isa<DeclStmt>(forSt->getInit())) {
    std::stringstream SSprint;
    SSprint << "{\n";
    DeclStmt *forStCond = cast<DeclStmt>(forSt->getInit());
    for (auto decl : forStCond->decls()) {
      if (isa<VarDecl>(decl)) {
        auto varDecl = cast<VarDecl>(decl);
        SSprint << getCompleteVarDeclStr(varDecl);
      }
    }
    TheRewriter.ReplaceText(
        SourceRange(forStCond->getBeginLoc(), forStCond->getEndLoc()), ";");
    // We remove variables seen in the scope at the end of it
    TheRewriter.InsertTextAfter(forSt->getBeginLoc(), SSprint.str());
    closeScopeAfter(TheRewriter, forSt);
  }
  return true;
}